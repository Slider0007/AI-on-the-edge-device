#include "openmetrics.h"
#include "../../include/defines.h"

#include <esp_log.h>
#include "esp_private/esp_clk.h"

#include "MainFlowControl.h"
#include "system.h"
#include "connect_wlan.h"

extern std::string getFwVersion(void);


static const char *TAG = "OPENMETRICS";


/**
 * Create a hardware info metric
 **/
std::string createHardwareInfoMetric(const std::string &metricNamePrefix)
{
    return "# HELP " + metricNamePrefix + "hardware_info Hardware info\n" +
           "# TYPE " + metricNamePrefix + "hardware_info gauge\n" +
           metricNamePrefix + "hardware_info{board_type=\"" + getBoardType() +
                                          "\",chip_model=\"" + getChipModel() +
                                          "\",chip_cores=\"" + std::to_string(getChipCoreCount()) +
                                          "\",chip_revision=\"" + getChipRevision() +
                                          "\",chip_frequency=\"" + std::to_string(esp_clk_cpu_freq()/1000000) +
                                          "\",camera_type=\"" + Camera.getCamType() +
                                          "\",camera_frequency=\"" + std::to_string(Camera.getCamFrequencyMhz()) +
                                          "\",sdcard_capacity=\"" + std::to_string(getSDCardCapacity()) +
                                          "\",sdcard_partition_size=\"" + std::to_string(getSDCardPartitionSize()) + "\"} 1\n";
}


/**
 * Create a network info metric
 **/
std::string createNetworkInfoMetric(const std::string &metricNamePrefix)
{
    return "# HELP " + metricNamePrefix + "network_info Network info\n" +
           "# TYPE " + metricNamePrefix + "network_info gauge\n" +
           metricNamePrefix + "network_info{hostname=\"" + getHostname() +
                                          "\",ipv4_address=\"" + getIPAddress() +
                                          "\",mac_address=\"" + getMac() + "\"} 1\n";
}


/**
 * Create a firmware info metric
 **/
std::string createFirmwareInfoMetric(const std::string &metricNamePrefix)
{
    return "# HELP " + metricNamePrefix + "firmware_info Firmware info\n" +
           "# TYPE " + metricNamePrefix + "firmware_info gauge\n" +
           metricNamePrefix + "firmware_info{firmware_version=\"" + getFwVersion() + "\"} 1\n";
}


/**
 * Create heap data metrics
 **/
std::string createHeapDataMetric(const std::string &metricNamePrefix)
{
    return "# HELP " + metricNamePrefix + "heap_info_bytes Heap info\n" +
           "# UNIT " + metricNamePrefix + "heap_info_bytes bytes\n" +
           "# TYPE " + metricNamePrefix + "heap_info_bytes gauge\n" +
           metricNamePrefix + "heap_info_bytes{type=\"heap_total_free\"} " + std::to_string(getESPHeapSizeTotalFree()) + "\n" +
           metricNamePrefix + "heap_info_bytes{type=\"heap_internal_free\"} " + std::to_string(getESPHeapSizeInternalFree()) + "\n" +
           metricNamePrefix + "heap_info_bytes{type=\"heap_internal_largest_free\"} " + std::to_string(getESPHeapSizeInternalLargestFree()) + "\n" +
           metricNamePrefix + "heap_info_bytes{type=\"heap_internal_min_free\"} " + std::to_string(getESPHeapSizeInternalMinFree()) + "\n" +
           metricNamePrefix + "heap_info_bytes{type=\"heap_spiram_free\"} " + std::to_string(getESPHeapSizeSPIRAMFree()) + "\n" +
           metricNamePrefix + "heap_info_bytes{type=\"heap_spiram_largest_free\"} " +  std::to_string(getESPHeapSizeSPIRAMLargestFree()) + "\n" +
           metricNamePrefix + "heap_info_bytes{type=\"heap_spiram_min_free\"} " + std::to_string(getESPHeapSizeSPIRAMMinFree()) + "\n";
}


/**
 * Create a generic single metric
 **/
std::string createMetric(const std::string &metricName, const std::string &help, const std::string &type, const std::string &value)
{
    return "# HELP " + metricName + " " + help + "\n" +
           "# TYPE " + metricName + " " + type + "\n" +
           metricName + " " + value + "\n";
}


/**
 * Create a generic single metric with unit
 **/
std::string createMetricWithUnit(const std::string &metricName, const std::string &help, const std::string &type,
                                 const std::string &unit, const std::string &value)
{
    return "# HELP " + metricName + "_" + unit + " " + help + "\n" +
           "# UNIT " + metricName + "_" + unit + " " + unit + "\n" +
           "# TYPE " + metricName + "_" + unit + " " + type + "\n" +
           metricName + "_" + unit + " " + value + "\n";
}


/**
 * Generate the MetricFamily from all available sequences
 * @returns the string containing the text wire format of the MetricFamily
 **/
std::string createSequenceMetrics(const std::string &metricNamePrefix, const std::vector<NumberPost *> &sequences)
{
    std::string response;

    for (const auto &sequence : sequences) {
        std::string sequenceName = sequence->name;

        // except newline, double quote, and backslash (https://github.com/OpenObservability/OpenMetrics/blob/main/specification/OpenMetrics.md#abnf)
        // to keep it simple, these characters are just removed from the label
        replaceAll(sequenceName, "\\", "");
        replaceAll(sequenceName, "\"", "");
        replaceAll(sequenceName, "\n", "");

        if (!sequence->sActualValue.empty())
            response += metricNamePrefix + "actual_value{sequence=\"" + sequenceName + "\"} " + sequence->sActualValue + "\n";
    }

    // Return if no valid value is available
    if (response.empty()) {
        return response;
    }

    response += "# HELP " + metricNamePrefix + "actual_value Actual value of meter\n" +
                "# TYPE " + metricNamePrefix + "actual_value gauge\n" + response;

    response += "# HELP " + metricNamePrefix + "rate_per_minute Rate per minute of meter\n" +
                "# TYPE " + metricNamePrefix + "rate_per_minute gauge\n";

    for (const auto &sequence : sequences) {
        std::string sequenceName = sequence->name;

        // except newline, double quote, and backslash (https://github.com/OpenObservability/OpenMetrics/blob/main/specification/OpenMetrics.md#abnf)
        // to keep it simple, these characters are just removed from the label
        replaceAll(sequenceName, "\\", "");
        replaceAll(sequenceName, "\"", "");
        replaceAll(sequenceName, "\n", "");
        response += metricNamePrefix + "rate_per_minute{sequence=\"" + sequenceName + "\"} " + sequence->sRatePerMin + "\n";
    }

    return response;
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
 * Example configuration for Prometheus (`prometheus.yml`):
 *
 *    - job_name: watermeter
 *      static_configs:
 *        - targets: ['192.168.1.4']
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

    // Metric name prefix
    const std::string metricNamePrefix = "ai_on_the_edge_device_";

    // Hardware (board, camera, sd-card) info
    std::string response = createHardwareInfoMetric(metricNamePrefix);

    // Network info
    response += createNetworkInfoMetric(metricNamePrefix);

    // Firmware info
    response += createFirmwareInfoMetric(metricNamePrefix);

    // Device uptime
    response += createMetricWithUnit(metricNamePrefix + "device_uptime", "Device uptime in seconds",
                                    "gauge", "seconds", std::to_string((long)getUptime()));

    // WLAN signal strength
    response += createMetricWithUnit(metricNamePrefix + "wlan_rssi", "WLAN signal strength in dBm",
                                    "gauge", "dBm", std::to_string(get_WIFI_RSSI()));

    // CPU temperature
    response += createMetricWithUnit(metricNamePrefix + "chip_temp", "CPU temperature in celsius",
                                    "gauge", "celsius", std::to_string((int)getSOCTemperature()));

    // Heap data
    response += createHeapDataMetric(metricNamePrefix);

    // SD card partition free space
    response += createMetricWithUnit(metricNamePrefix + "sd_partition_free", "Free SD partition space in MB",
                                    "gauge", "MB", std::to_string(getSDCardFreePartitionSpace()));

    // Process error state
    response += createMetric(metricNamePrefix + "process_error", "Process error state",
                                    "gauge", std::to_string(flowctrl.getFlowStateErrorOrDeviation()));

    // Processing interval
    response += createMetricWithUnit(metricNamePrefix + "process_interval", "Processing interval",
                                    "gauge", "minutes", to_stringWithPrecision(flowctrl.getProcessInterval(), 1));

    // Processing time
    response += createMetricWithUnit(metricNamePrefix + "process_time", "Processing time of one cycle",
                                    "gauge", "seconds", std::to_string(getFlowProcessingTime()));

    // Process cycles
    response += createMetric(metricNamePrefix + "cycle_counter", "Process cycles since device startup",
                                    "counter", std::to_string(getFlowCycleCounter()));

    // Actual measurement values
    response += createSequenceMetrics(metricNamePrefix, flowctrl.getNumbers());

    // The response always contains at least the metadata (HELP, TYPE) for the MetricFamily so no length check is needed
    httpd_resp_sendstr(req, response.c_str());

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
