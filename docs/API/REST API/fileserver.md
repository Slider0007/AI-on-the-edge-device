[Overview](_overview.md) 

### REST API endpoint: fileserver

`http://IP-ADDRESS/fileserver/`

Endpoint to show folder content or get file content from sd card file system

Payload:
- Get file content (request ends with a specific filename) 
Example: `http://IP-ADDRESS/fileserver/log/message/log_2024-01-11.txt`

  Response:
    - Content type: `HTML`
    - Content: Content of file

- Show folder content (request ends with a traling slash)
Example: `http://IP-ADDRESS/fileserver/config/`

  Response:
    - Content type: `HTML`
    - Content: Content of folder (HTML table)