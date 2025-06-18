[Overview](_OVERVIEW.md) 

## REST API endpoint: fileserver

`http://IP-ADDRESS/fileserver/`

This endpoint provides access to the file system of the SD card. Depending on the request path, 
it either returns the contents of a file or renders the contents of a folder as an HTML table.

Payload:
1. Get file content (URL that ends with a filename) 
    - Example: `/fileserver/log/message/log_2024-01-11.txt`
    - Response:
      - Content type: `HTML`
      - Content: Content of file

2. Show folder content as HTML table (URL that ends with a trailing slash `/`)
    1. Show all files in folder
      - Example: `/fileserver/config/` or `/fileserver/config/?full=true`
      - Response:
        - Content type: `HTML`
        - Content: Content of folder (HTML file table)
    2. Show only first 100 items in folder
      - Example: `/fileserver/config/?full=false`
      - Response:
        - Content type: `HTML`
        - Content: Content of folder (HTML file table with first 100 items only)
    3. Show all files in folder (read-only)
      - Example: `/fileserver/config/?readonly=true`
      - Response:
        - Content type: `HTML`
        - Content: Content of folder (HTML file table without `Action` column)
