[Overview](_OVERVIEW.md) 

## REST API endpoint: coredump

`http://IP-ADDRESS/coredump`


Handle core dumps for debugging purpose (software exception)<br>

1. Get API name and version:
    - Payload:
      - `/coredump?task=api_name`
    - Response:
      - Content type: `Plain Text`
      - Content: Query response, e.g. `coredump:v1`

2. Get core dump summary:
    - Payload:
      - No payload needed
    - Response:
      - Content type: `Plain Text` (easy to copy)
      - Content: Query response
    - Example:<br>
        ```
        Backtrace:  0x4008391D 0x40090B9D 0x40097AF5 0x400D9E6F 0x400FA185 0x400F91AC 0x400F973A 0x400F8220 0x401E1AB2 0x400F8430 0x4009145D
        Depth: 11
        Corrupted: 0
        PC: 1074280733
        Firmware version: Develop: coredump (Commit: b23f061)
        ```

3. Save core dump file:
    - Payload:
      - `/coredump?task=save`
    - Response:
      - Content type: `Attachment`
      - Content: Core dump file (as download)

4. Clear core dump partition:
    - Payload:
      - `/coredump?task=clear`
    - Response:
      - Content type: `Plain Text`
      - Content: Query response

5. Force a software exception (Device reboots instantly):
    - Payload:
      - `/coredump?task=force_exception`
    - Response:
      - Content type: `Plain Text`
      - Content: None
