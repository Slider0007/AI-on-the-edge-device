# Parameter: URI

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | URI                 | Uri
| Default Value     | empty               | empty


## Description

URL of the webhook (HTTP URL endpoint which shall be receiving the message)<br>
e.g. `http://ServerAddress/1234567890`


!!! Note
    Using TLS for the webhook, adaptions of webhook `URI` parameter needs to be done. Please ensure 
    protocol `https://` is configured, e.g. `https://ServerAddress/1234567890`
