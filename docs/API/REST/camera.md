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
        - `exposurecontrolmode` Exposure Control Mode [0 .. 2]
        - `autoexposurelevel` Auto Exposure Level [-2 .. 2]
        - `manualexposurevalue` Manual Exposure Value [0 .. 1200]
        - `gaincontrolmode` Gain Control Mode [0 .. 1]
        - `manualgainvalue` Manual Gain Value [0 .. 30]
        - `specialeffect` Special Effect [0 .. 2, 7] (0: None, 1: Negative, 2: Grayscale, 7: Grayscale + Negative)
        - `mirror` Image Mirror [true, false]
        - `flip` Image Flip [true, false]
        - `zoommode` Zoom Mode [0 .. 2] (0: Off, 1: Crop only, 2: Scale & Crop)
        - `zoomx` Zoom Offset X [0 .. 960]
        - `zommy` Zoom Offset Y [0 .. 720]
      - Example: `/camera?task=set_parameter&flashtime=0.1&flashintensity=1&brightness=-2&contrast=0&saturation=0 &sharpness=0&exposurecontrolmode=1&autoexposurelevel=0&manualexposurevalue=300&gaincontrolmode=1 &manualgainvalue=0&specialeffect=0&mirror=false&flip=false&zoommode=0&zoomx=0&zoomy=0`
      - Response:
        - Content type: `HTML`
        - Content: `001: Parameter set`
