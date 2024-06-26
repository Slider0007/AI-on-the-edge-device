## Overview: Prometheus API
### Prometheus / OpenMetrics telemetry data

A set of metrics is exported via the `/metrics` REST API endpoint (see also REST API description). 
The metrics can be scraped by Prometheus or any OpenMetrics specification compatilble software.<br>

The metrics are provided in text wire format based on [OpenMetrics specification](https://github.com/OpenObservability/OpenMetrics/blob/main/specification/OpenMetrics.md) 
which is backward-compatible with [Prometheus text-based exposition format](https://github.com/prometheus/docs/blob/main/content/docs/instrumenting/exposition_formats.md).

### Metric Name Design Approach

The MetricPrefix is hard-coded: `ai_on_the_edge_device`

Generic metric name: `metricPrefix` + `metricName` + `_unit` (and/or `_total` for counter metric)<br>
Example: `ai_on_the_edge_device_uptime_seconds`


### Metrics
#### 1. Hardware Info Metric `ai_on_the_edge_device_hardware_info`

All information are static and provided by labels. The metric value is set to `1`

| Metric label        | Description                 | Output
|:--------------------|:----------------------------|:--------------
| `board_type`        | Board Type                  | `ESP32CAM`  
| `chip_model`        | Device SOC Model            | `ESP32`
| `chip_cores`        | Device SOC Cores            | `2`
| `chip_revision`     | Device SOC Silicon Revision | `1.00`
| `chip_frequency`    | Device SOC CPU Frequency    | `160`
| `camera_type`       | Camera Type                 | `OV2640`
| `camera_frequency`  | Camera Frequency [Mhz]      | `20`
| `sdcard_capacity`   | SD card capacity [MB]       | `29580`
| `sdcard_partition_size` | SD card partition size [MB] | `29560`


#### 2. Network Info Metric `ai_on_the_edge_device_network_info`

All information are static and provided by labels. The metric value is set to `1`

| Metric label        | Description                 | Output
|:--------------------|:----------------------------|:--------------     
| `hostname`          | Device Hostname             | `watermetter`
| `ipv4_address`      | Device IPv4 Address         | `192.168.1.x`
| `mac_address`       | Device MAC Address          | `44:21:D8:04:DF:A8`


#### 3. Firmware Info Metric `ai_on_the_edge_device_firmware_info`

All information are static and provided by labels. The metric value is set to `1`

| Metric Label        | Description                 | Output
|:--------------------|:----------------------------|:--------------     
| `firmware_version`  | Firmware Version (MCU)      | `v17.0.0 (1234567)`


#### 4. Heap Data Metric `ai_on_the_edge_device_heap_data_bytes`

All data are provided by labels. The metric label is called `heap_data`.

Example: `ai_on_the_edge_device_heap_data_bytes{heap_data="heap_total_free"}`

| Metric Label Values          | Description                 | Output
|:-----------------------------|:----------------------------|:--------------     
| `heap_total_free`            | Memory: Total Free (Int. + Ext.) [kB] | `3058639`
| `heap_internal_free`         | Memory: Internal Free [kB]  | `75079`
| `heap_internal_largest_free` | Memory: Internal Largest Free Block [kB] | `65536`
| `heap_internal_min_free`     | Memory: Internal Minimum Free [kB] | `57647`
| `heap_spiram_free`           | Memory: External Free [kB]  | `2409076`
| `heap_spiram_largest_free`   | Memory: External Largest Free Block [kB] | `2359296`
| `heap_spiram_min_free`       | Memory: External Minimum Free [kB] | `1359460`


#### 5. Further Device Status Metrics

| Metric Name                                      | Description                 | Output
|:-------------------------------------------------|:----------------------------|:--------------     
| `ai_on_the_edge_device_device_uptime_seconds `   | Device Uptime [s]           | `147`
| `ai_on_the_edge_device_wlan_rssi_dBm`            | WLAN Signal Strength [dBm]  | `-54`
| `ai_on_the_edge_device_chip_temp_celsius`        | Device CPU Temperature (°C) | `45`
| `ai_on_the_edge_device_sd_partition_free_megabytes`| SD Card: Free Partition Space | `29016`


#### 6. Process Status Metrics

| Metric Name                                      | Description                 | Output
|:-------------------------------------------------|:----------------------------|:--------------     
| `ai_on_the_edge_device_process_interval_minutes` | Automatic Process Interval [min] | `2.0`
| `ai_on_the_edge_device_process_time_seconds`     | Process Time [sec]          | `25`
| `ai_on_the_edge_device_process_error`            | Process Error State<br>- Error definition: Process error with cycle abortion, e.g. alignment failed<br>- Deviation definition: Process deviation with cycle continuation, e.g. rate limit exceeded<br><br>Possible States:<br>- `0`: No error/deviation<br>- `-1`: One error occured<br>- `-2`: Multiple process errors in a row<br>- `1`: One process deviation occured<br>- `2`: Multiple process deviations in a row | `0`
| `ai_on_the_edge_device_cycle_counter_total`      | Process Cycle Counter       | `64`


#### 7. Process Data Metrics

Muliple sequence data is provided separately by label `sequence`.

| Topic                     | Description                 | Output
|:--------------------------|:----------------------------|:--------------   
| `ai_on_the_edge_device_actual_value{sequence="[sequenceName]"}` | Actual value of [sequenceName] | `146.540`
| `ai_on_the_edge_device_rate_per_minute{sequence="[sequenceName]"}`| Rate per minute<br>(Delta of actual and last valid processed cycle + normalized to minute) | `0.000`


### Prometheus Scrape Config

The following scrape config (add to `prometheus.yml`) can be used as an example to ingest available metrics with prometheus:
```
scrape_configs:
  - job_name: watermeter
    scrape_interval: 300s
    metrics_path: /metrics
    static_configs:
      - targets: ['192.168.1.4']
```

Example response of REST API `/metrics`:
```
# TYPE ai_on_the_edge_device_hardware_info gauge
# HELP ai_on_the_edge_device_hardware_info Hardware info
ai_on_the_edge_device_hardware_info{board_type="ESP32CAM",chip_model="ESP32",chip_cores="2",chip_revision="1.0",chip_frequency="160",camera_type="OV2640",camera_frequency="20",sdcard_capacity="29580",sdcard_partition_size="29560"} 1
# TYPE ai_on_the_edge_device_network_info gauge
# HELP ai_on_the_edge_device_network_info Network info
ai_on_the_edge_device_network_info{hostname="watermeter",ipv4_address="192.168.2.68",mac_address="40:22:D8:03:5F:AC"} 1
# TYPE ai_on_the_edge_device_firmware_info gauge
# HELP ai_on_the_edge_device_firmware_info Firmware info
ai_on_the_edge_device_firmware_info{firmware_version="Develop: openmetrics-exporter (Commit: 432bb72)"} 1
# TYPE ai_on_the_edge_device_device_uptime_seconds gauge
# UNIT ai_on_the_edge_device_device_uptime_seconds seconds
# HELP ai_on_the_edge_device_device_uptime_seconds Device uptime in seconds
ai_on_the_edge_device_device_uptime_seconds 109
# TYPE ai_on_the_edge_device_wlan_rssi_dBm gauge
# UNIT ai_on_the_edge_device_wlan_rssi_dBm dBm
# HELP ai_on_the_edge_device_wlan_rssi_dBm WLAN signal strength in dBm
ai_on_the_edge_device_wlan_rssi_dBm -60
# TYPE ai_on_the_edge_device_chip_temp_celsius gauge
# UNIT ai_on_the_edge_device_chip_temp_celsius celsius
# HELP ai_on_the_edge_device_chip_temp_celsius CPU temperature in celsius
ai_on_the_edge_device_chip_temp_celsius 40
# TYPE ai_on_the_edge_device_heap_info_bytes gauge
# UNIT ai_on_the_edge_device_heap_info_bytes bytes
# HELP ai_on_the_edge_device_heap_info_bytes Heap info
ai_on_the_edge_device_heap_info_bytes{heap_type="heap_total_free"} 2381099
ai_on_the_edge_device_heap_info_bytes{heap_type="heap_internal_free"} 69159
ai_on_the_edge_device_heap_info_bytes{heap_type="heap_internal_largest_free"} 65536
ai_on_the_edge_device_heap_info_bytes{heap_type="heap_internal_min_free"} 57971
ai_on_the_edge_device_heap_info_bytes{heap_type="heap_spiram_free"} 2311700
ai_on_the_edge_device_heap_info_bytes{heap_type="heap_spiram_largest_free"} 2293760
ai_on_the_edge_device_heap_info_bytes{heap_type="heap_spiram_min_free"} 1261940
# TYPE ai_on_the_edge_device_sd_partition_free_megabytes gauge
# UNIT ai_on_the_edge_device_sd_partition_free_megabytes megabytes
# HELP ai_on_the_edge_device_sd_partition_free_megabytes Free SD partition space in MB
ai_on_the_edge_device_sd_partition_free_megabytes 28410
# TYPE ai_on_the_edge_device_process_error gauge
# HELP ai_on_the_edge_device_process_error Process error state
ai_on_the_edge_device_process_error 0
# TYPE ai_on_the_edge_device_process_interval_minutes gauge
# UNIT ai_on_the_edge_device_process_interval_minutes minutes
# HELP ai_on_the_edge_device_process_interval_minutes Processing interval
ai_on_the_edge_device_process_interval_minutes 2.0
# TYPE ai_on_the_edge_device_process_time_seconds gauge
# UNIT ai_on_the_edge_device_process_time_seconds seconds
# HELP ai_on_the_edge_device_process_time_seconds Processing time of one cycle
ai_on_the_edge_device_process_time_seconds 24
# TYPE ai_on_the_edge_device_cycle_counter counter
# HELP ai_on_the_edge_device_cycle_counter Process cycles since device startup
ai_on_the_edge_device_cycle_counter_total 2
# TYPE ai_on_the_edge_device_actual_value gauge
# HELP ai_on_the_edge_device_actual_value Actual value of meter
ai_on_the_edge_device_actual_value{sequence="main"} 530.01083
ai_on_the_edge_device_actual_value{sequence="test"} 3
# TYPE ai_on_the_edge_device_rate_per_minute gauge
# HELP ai_on_the_edge_device_rate_per_minute Rate per minute of meter
ai_on_the_edge_device_rate_per_minute{sequence="main"} 0.000000
ai_on_the_edge_device_rate_per_minute{sequence="test"} 0.0
```
