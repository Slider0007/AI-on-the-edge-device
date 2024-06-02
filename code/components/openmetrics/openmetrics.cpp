#include "openmetrics.h"
#include "../../include/defines.h"

#include <esp_log.h>

#include "MainFlowControl.h"
#include "system.h"
#include "connect_wlan.h"


static const char *TAG = "OPENMETRICS";


/**
 * create a singe metric from the given input
 **/
std::string createMetric(const std::string &metricName, const std::string &help, const std::string &type, const std::string &value)
{
    return "# HELP " + metricName + " " + help + "\n" +
           "# TYPE " + metricName + " " + type + "\n" +
           metricName + " " + value + "\n";
}


/**
 * Generate the MetricFamily from all available sequences
 * @returns the string containing the text wire format of the MetricFamily
 **/
std::string createSequenceMetrics(std::string prefix, const std::vector<NumberPost *> &sequences)
{
    std::string res;

    for (const auto &sequence : sequences) {
        std::string sequenceName = sequence->name;

        // except newline, double quote, and backslash (https://github.com/OpenObservability/OpenMetrics/blob/main/specification/OpenMetrics.md#abnf)
        // to keep it simple, these characters are just removed from the label
        replaceAll(sequenceName, "\\", "");
        replaceAll(sequenceName, "\"", "");
        replaceAll(sequenceName, "\n", "");
        res += prefix + "_flow_value{sequence=\"" + sequenceName + "\"} " + sequence->sActualValue + "\n";
    }

    // prepend metadata if a valid metric was created
    if (res.length() > 0) {
        res = "# HELP " + prefix + "_flow_value current value of meter readout\n# TYPE " + prefix + "_flow_value gauge\n" + res;
    }

    return res;
}


/**
 * Generates a http response containing the OpenMetrics (https://openmetrics.io/) text wire format
 * according to https://github.com/OpenObservability/OpenMetrics/blob/main/specification/OpenMetrics.md#text-format.
 *
 * A MetricFamily with a Metric for each Sequence is provided. If no valid value is available, the metric is not provided.
 * MetricPoints are provided without a timestamp. Additional metrics with some device information is also provided.
 *
 * The metric name prefix is 'ai_on_the_edge_device_'.
 *
 * example configuration for Prometheus (`prometheus.yml`):
 *
 *    - job_name: watermeter
 *      static_configs:
 *        - targets: ['watermeter.fritz.box']
 *
*/
esp_err_t handler_openmetrics(httpd_req_t *req)
{
    if (getTaskAutoFlowState() <= FLOW_TASK_STATE_INIT) {
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send_err(req, HTTPD_403_FORBIDDEN, "E95: Request rejected, flow not initialized");
        return ESP_FAIL;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "text/plain"); // application/openmetrics-text is not yet supported by prometheus so we use text/plain for now

    const std::string metricNamePrefix = "ai_on_the_edge_device";

    // get current measurement values
    std::string response = createSequenceMetrics(metricNamePrefix, flowctrl.getNumbers());

    // CPU temperature
    response += createMetric(metricNamePrefix + "_cpu_temperature_celsius", "current cpu temperature in celsius", "gauge", std::to_string((int)getSOCTemperature()));

    // WiFi signal strength
    response += createMetric(metricNamePrefix + "_rssi_dbm", "current WiFi signal strength in dBm", "gauge", std::to_string(get_WIFI_RSSI()));

    // memory info
    response += createMetric(metricNamePrefix + "_memory_heap_free_bytes", "available heap memory", "gauge", std::to_string(getESPHeapSizeTotalFree()));

    // device uptime
    response += createMetric(metricNamePrefix + "_uptime_seconds", "device uptime in seconds", "gauge", std::to_string((long)getUptime()));

    // data aquisition round
    response += createMetric(metricNamePrefix + "_cycles_total", "data aquisition cycles since device startup", "counter", std::to_string(getFlowCycleCounter()));

    // the response always contains at least the metadata (HELP, TYPE) for the MetricFamily so no length check is needed
    httpd_resp_send(req, response.c_str(), response.length());

    return ESP_OK;
}


void register_openmetrics_uri(httpd_handle_t server)
{
    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_uri_t camuri = { };
    camuri.method    = HTTP_GET;

    camuri.uri = "/metrics";
    camuri.handler = handler_openmetrics;
    camuri.user_ctx = NULL;
    httpd_register_uri_handler(server, &camuri);
}
