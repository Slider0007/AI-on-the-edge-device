# Parameter: Network Configuration

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Network Configuration | networkconfig
| Default Value     | `DHCP`              | `0`
| Input Options     | `DHCP`<br>`Static`  | `0`<br>`1`


## Description

Select the network configuration principle


| Input Option               | Description
|:---                        |:---
| `DHCP`                     | Automatically assign IP related parameter provided by a server (router, AP)
| `Static`                   | Manual confiuuration of network related parameter like, IP, subnet, gateway, DNS server


!!! Note
    To apply this parameter a device reboot is required.