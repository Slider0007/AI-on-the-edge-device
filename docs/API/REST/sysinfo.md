[Overview](_OVERVIEW.md) 

### REST API endpoint: sysinfo

`http://IP-ADDRESS/sysinfo`


Get device info

The following data are available:

| JSON Property               | Description                                      | Output
|:----------------------------|:-------------------------------------------------|:-----------------------
| `firmware`                  | Firmware version                                 | `Develop: develop (Commit: 4c1a1c9`
| `buildtime`                 | Firmware build time (UTC)                        | `2024-01-27 15:05`
| `gitbranch`                 | Git Branch Name                                  | `develop`
| `gittag`                    | Git Tag Name (only for releases, otherwise `N/A`)| `N/A`
| `gitrevision`               | Git Revision                                     | `4c1a1c9`
| `html`                      | Webinterface version                             | `Develop: develop (Commit: 4c1a1c9`
| `cputemp`                   | Device Temperature (degree celcius)              | `38`
| `hostname`                  | Device Hostname                                  | `WaterMeter`
| `IPv4`                      | Device IPv4 Address                              | `192.168.1.x`
| `freeHeapMem`               | Total heap (Int + Ext.) free (byte)              | `2656663`


Payload:
- No payload needed

Response:
- Content type: `JSON`
- Content: Query response
- Example: 
```
[
  {
    "firmware": "Develop: develop (Commit: 4c1a1c9)",
    "buildtime": "2024-01-27 15:05",
    "gitbranch": "develop",
    "gittag": "N/A",
    "gitrevision": "4c1a1c9",
    "html": "Develop: develop (Commit: 4c1a1c9)",
    "cputemp": "38",
    "hostname": "MainWaterMeter",
    "IPv4": "192.168.2.18",
    "freeHeapMem": "2656663"
}
]
```
