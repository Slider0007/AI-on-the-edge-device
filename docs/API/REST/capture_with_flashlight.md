[Overview](_OVERVIEW.md) 

### REST API endpoint: capture_with_flashlight

`http://IP-ADDRESS/capture_with_flashlight`


Instantly capture actual image using flashlight (flash duration: default)

Payload:
- No payload needed

Response:
- Content type: `HTML`
- Content: Raw image (JPG file)<br>
  (Response delayed by flash duration)
