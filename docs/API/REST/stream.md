[Overview](_OVERVIEW.md) 

### REST API endpoint: stream

`http://IP-ADDRESS/stream`


Get live stream of camera

__IMPORTANT__: A running stream is blocking the entire web interface (to limit memory usage for this function). Please ensure to close stream before continue with WebUI.


Payload:
- `flashlight=true` Activate onboard flashlight
- Flashlight off: `/stream` or `/stream?flashlight=false`
- Flashlight on:`/stream?flashlight=true`

Response:
  - Content type: `multipart/x-mixed-replace`
  - Content: Camera live stream
