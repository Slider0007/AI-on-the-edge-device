[Overview](_OVERVIEW.md) 

## REST API endpoint: metrics

`http://IP-ADDRESS/metrics`


Provides a set of metrics that can be scraped by prometheus or any OpenMetrics compatilble software. 
The metrics are provided in text wire format based on [OpenMetrics specification](https://github.com/OpenObservability/OpenMetrics/blob/main/specification/OpenMetrics.md) 
which is backward-compatible with [Prometheus text-based exposition format](https://github.com/prometheus/docs/blob/main/content/docs/instrumenting/exposition_formats.md).


### Metrics
Metrics description > see Prometheus API description (docs/API/Prometheus-OpenMetrics)

### Example output

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
ai_on_the_edge_device_process_interval_minutes 1.0
# TYPE ai_on_the_edge_device_process_time_seconds gauge
# UNIT ai_on_the_edge_device_process_time_seconds seconds
# HELP ai_on_the_edge_device_process_time_seconds Processing time of one cycle
ai_on_the_edge_device_process_time_seconds 21
# TYPE ai_on_the_edge_device_cycle_counter counter
# HELP ai_on_the_edge_device_cycle_counter Process cycles since device startup
ai_on_the_edge_device_cycle_counter_total 2
# TYPE ai_on_the_edge_device_actual_value gauge
# HELP ai_on_the_edge_device_actual_value Actual value of meter
ai_on_the_edge_device_actual_value{sequence="main"} 530.01083
ai_on_the_edge_device_actual_value{sequence="name"} 3
# TYPE ai_on_the_edge_device_rate_per_minute gauge
# HELP ai_on_the_edge_device_rate_per_minute Rate per minute of meter
ai_on_the_edge_device_rate_per_minute{sequence="main"} 0.000000
ai_on_the_edge_device_rate_per_minute{sequence="name"} 0.0
```
