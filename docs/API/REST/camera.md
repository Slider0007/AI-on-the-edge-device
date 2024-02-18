[Overview](_OVERVIEW.md) 

### REST API endpoint: camera

`http://IP-ADDRESS/camera`


Camera related tasks

Payload:
  - `task` Task to perform
  - Available options:
    - `set_parameter` Set camera parameter
      - Full or delta parameter update is possible
      - Possible parameter:
        - `flashtime` Flash Time [0.1 .. &infin; seconds]
        - `flashintensity` Flash Intensity [0 .. 100 %]
        - `brightness` Image Brightness [-2 .. 2]
        - `contrast` Image Contrast [-2 .. 2]
        - `saturation` Image Saturation [-2 .. 2]
        - `sharpness` Image Sharpness [-4(Auto), -3 .. 3]
        - `autoexposurelevel` Auto Exposure Level [-2 .. 2]
        - `aec2` Alternative Auto Exposure Control Algorithm [true, false]
        - `specialeffect` Special Effect [0 .. 2] (0: None, 1: Negative, 2: Grayscale)
        - `mirror` Image Mirror Horizontally [true, false]
        - `flip` Image Flip Vertically [true, false]
        - `zoom` Enable Zoom [true, false]
        - `zoommode` Zoom Mode [0 .. 1] (0: Crop only, 1: Scale & Crop)
        - `zoomx` Zoom Offset X [0 .. 960]
        - `zommy` Zoom Offset Y [0 .. 720]
      - Example: `/camera?task=set_parameter&flashtime=2.0&flashintensity=1&brightness=0&contrast=0&saturation=0&sharpness=0&autoexposurelevel=0&aec2=false&specialeffect=0&mirror=false&flip=false&zoom=true&zoommode=0&zoomx=0&zoomy=0`
      - Response:
        - Content type: `HTML`
        - Content: `001: Parameter set`

