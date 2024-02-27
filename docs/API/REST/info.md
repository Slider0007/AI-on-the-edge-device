[Overview](_OVERVIEW.md) 

### REST API endpoint: info

`http://IP-ADDRESS/info`


Get device info

Payload:
- `type`: Request type
  - Available options:
    | Content                 | Query `type=`          
    |:------------------------|:----------------------------
    | Cycle Counter           | CycleCounter
    | WLAN SSID               | SSID   
    | IPv4 Address            | IP    
    | Hostname                | Hostname
    | SD Card Name            | SDCardName            
    | SD Card Manufacturer    | SDCardManufacturer    
    | SD Card Capacity        | SDCardCapacity        
    | SD Card Sector Size     | SDCardSectorSize      
    | SD Card Partition Allocation Size| SDCardPartitionAllocationSize
    | SD Card Partition Size  | SDCardPartitionSize
    | SD Card Free Partition Space | SDCardFreePartitionSpace
    | Git Branch              | GitBranch
    | Git Tag                 | GitTag
    | Git Revision            | GitRevision
    | Firmware Version        | FirmwareVersion
    | HTML Version            | HTMLVersion
    | Build Time              | BuildTime
  - Example: `/info?type=GitBranch` 

Response:
- Content type: `HTML`
- Content: Query response
