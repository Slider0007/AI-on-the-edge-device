# Parameter: Server Certificate Verification

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Server Certificate Verification | servercertverification
| Default Value     | `Enabled`            | `2`
| Input Options     | `Disabled`<br>`Enabled (No Name Validation)`<br>`Enabled`| `0`<br>`1`<br>`2`


## Description

Set TLS server certificate verification / validation method


### Input Options

| Input Option              | Description
|:---                       |:---
| `Disabled`                | Server certificate verification / validation disabled (High security risk. Use only for testing purpose.)
| `Enabled (No Name Validation)` | Server certificate is validated, except certificate identity (common name (CN) / subject alternative names (SAN))
| `Enabled`                 | Server certificate is fully validated


!!! Warning
    Disabled certificate verification or verification without name validation comes with a potential risk 
    of establishing a TLS connection with a server that has a fake identity. Disabled verification shall be 
    used only for testing purposes.


!!! Tipp
    TLS error code reference:<br>
    - [ESP-IDF Error Codes](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/error-codes.html)<br>
    - [MbedTLS Error Codes](https://github.com/Mbed-TLS/mbedtls/blob/development/include/mbedtls/x509.h)<br>
    - [Cert Verify Codes](https://github.com/Mbed-TLS/mbedtls/blob/development/include/mbedtls/x509.h)

