# Parameter: Retain Discovery Topics

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Retain Discovery Topics | HARetainDiscovery
| Default Value     | `false`             | `false`
| Input Options     | `false`<br>`true`   | `false`<br>`true` 


## Description

Enable or disable [message retain flag](https://www.hivemq.com/blog/mqtt-essentials-part-8-retained-messages/) 
for Home Assistant discovery topics.

!!! Warning
    It is recommended to use Home Assistant status topic to trigger automatic re-publish Home Assistant Discovery topics.
    A disadvantage of using retained messages is that these messages retain at the broker, even when the device or service 
    stops working. They are retained even after the system or broker has been restarted. Retained messages can create ghost 
    entities that keep coming back.<br>
    Especially when you have many entities, (unneeded) discovery messages can cause excessive system load. For this reason, 
    use discovery messages with caution.