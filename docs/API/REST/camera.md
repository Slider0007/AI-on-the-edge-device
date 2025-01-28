[Overview](_OVERVIEW.md) 

## REST API endpoint: camera

`http://IP-ADDRESS/camera`


Camera related tasks

Payload:
  - `task` Task to perform
  - Available options:
    - `api_name` API name + version
      - Example: `/camera?task=api_name`
      - Response:
        - Content type: `HTML`
        - Content: `camera:vx`
    - `capture` Capture image (single shot) with given parameter set and response either to REST API or save to SD card
      - Full or delta parameter update is possible
      - Possible parameter:
        - `flashtime` Flash Time [100 .. &infin; milliseconds]
        - `flashintensity` Flash Intensity [0 .. 100 %]
        - `brightness` Image Brightness [-2 .. 2]
        - `contrast` Image Contrast [-2 .. 2]
        - `saturation` Image Saturation [-2 .. 2]
        - `sharpness` Image Sharpness [-3 .. 3]
        - `exposurecontrolmode` Exposure Control Mode [0 .. 2]
        - `autoexposurelevel` Auto Exposure Level [-2 .. 2]
        - `manualexposurevalue` Manual Exposure Value [0 .. 1200]
        - `gaincontrolmode` Gain Control Mode [0 .. 1]
        - `manualgainvalue` Manual Gain Value [0 .. 30]
        - `specialeffect` Special Effect [0 .. 2, 7] (0: None, 1: Negative, 2: Grayscale, 7: Grayscale + Negative)
        - `mirror` Image Mirror [true, false]
        - `flip` Image Flip [true, false]
        - `zoomfactor` Zoom Factor [1000 .. 4000] (1000: 1.0x, 4000: 4.0x | Max. zoom factor is depending on hardware capabilities)
        - `zoomx` Zoom Offset X [0 .. max. 960] (Max. Offset is limited in firmware depending on actual zoom factor | Lower zoom --> lower limits)
        - `zommy` Zoom Offset Y [0 .. max. 720] (Max. Offset is limited in firmware depending on actual zoom factor | Lower zoom --> lower limits)
        - `filename` Filename incl. path on SD card (e.g. `/foldername/filename.jpg`)
      - Example 1 (Reponse image via REST API): `/camera?task=capture&flashtime=2000&flashintensity=50&brightness=0&contrast=0&saturation=0&sharpness=1&exposurecontrolmode=1&autoexposurelevel=0&manualexposurevalue=300&gaincontrolmode=1&manualgainvalue=0&specialeffect=0&mirror=false&flip=false&zoomfactor=1000&zoomx=0&zoomy=0`
      - Response:
        - Content type: `JPEG`
        - Content: Image (JPG file)
      - Example 2 (Save image to SD card): `/camera?task=capture&flashtime=2000&flashintensity=50&brightness=0&contrast=0&saturation=0&sharpness=1&exposurecontrolmode=1&autoexposurelevel=0&manualexposurevalue=300&gaincontrolmode=1&manualgainvalue=0&specialeffect=0&mirror=false&flip=false&zoomfactor=1000&zoomx=0&zoomy=0&filename=/img_tmp/filename.jpg`
      - Response:
        - Content type: `HTML`
        - Content: `001: Capture to file successful`
    - `flashlight_on` Flashlight on
      - Example: `/camera?task=flashlight_on`
      - Response:
        - Content type: `HTML`
        - Content: `002: Flashlight on`
    - `flashlight_off` Flashlight off
      - Example: `/camera?task=flashlight_off`
      - Response:
        - Content type: `HTML`
        - Content: `003: Flashlight off`
    - `stream` Camera livestream without flashlight<br>
      __IMPORTANT__: A running stream is blocking the entire web interface (to limit memory usage for 
                     this function). Please ensure to close stream before continue with WebUI.
      - Example: `/camera?task=stream`
      - Response:
        - Content type: `HTML`
        - Content: `004: Camera livestream`
    - `stream_flashlight` Camera livestream with flashlight<br>
      __IMPORTANT__: A running stream is blocking the entire web interface (to limit memory usage for 
                     this function). Please ensure to close stream before continue with WebUI.
      - Example: `/camera?task=stream_flashlight`
      - Response:
        - Content type: `HTML`
        - Content: `005: Camera livestream with flashlight`
