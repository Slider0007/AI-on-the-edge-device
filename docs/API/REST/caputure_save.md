[Overview](_OVERVIEW.md) 

### REST API endpoint: save

`http://IP-ADDRESS/save`


Save a new image to SD card

Payload:
- `filename` Filename incl. path, e.g. `img_tmp/test.jpg` (folder: `img_tmp`, filename: `test.jpg`)
- `delay` Flashlight time im milliseconds
- Example: `/save?filename=img_tmp/test.jpg&delay=1000`

Response:
- Content type: `HTML`
- Content: `/sdcard/img_tmp/test.jpg`<br>
  (Response delayed by flash duration)
