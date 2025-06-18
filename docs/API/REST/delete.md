[Overview](_OVERVIEW.md) 

## REST API endpoint: delete

`http://IP-ADDRESS/delete`


This endpoint deletes files or directories from the SD card via an HTTP POST request.


Payload:
1. Delete file
    - Example: `/delete/log/message/log_2024-01-11.txt`
    - Response:
      - Content type: `HTML`
      - Content: Redirects to parent folder and responds with HTML table of parent folder (only first 100 items)

2. Delete a folder recusively (Even if not empty --> Be careful!)
    - Example: `/delete/config/?task=deldir`
    - Response:
      - Content type: `HTML`
      - Content: Redirects to parent folder and responds with HTML table of parent folder (only fist 100 items)