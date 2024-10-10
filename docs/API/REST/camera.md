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
        - Content: `001: Camer parameter set`
    - `capture` Capture image without flashlight
      - Example: `/camera?task=capture`
      - Response:
        - Content type: `HTML`
        - Content: Raw image (JPG file)
    - `capture_with_flashlight` Capture with flashlight
      - Parameter:
        - `flashtime` Flashlight time in ms
      - Example: `/camera?task=capture_with_flashlight&flashtime=1000`
      - Response:
        - Content type: `HTML`
        - Content: Raw image (JPG file)<br>
          (Response delayed by flash duration)
    - `capture_to_file` Capture image with flashlight and save onto SD card
      - Parameter:
        - `flashtime` Flashlight time in ms
        - `filename` Filename incl. path on SD card
      - Example: `/camera?task=capture_to_file&flashtime=1000&filename=/img_tmp/filename.jpg`
      - Response:
        - Content type: `HTML`
        - Content: `/img_tmp/test.jpg`<br>
          (Response delayed by flash duration)
    - `flashlight_on` Flashlight on
      - Example: `/camera?task=flashlight_on`
      - Response:
        - Content type: `HTML`
        - Content: `005: Flashlight on`
    - `flashlight_off` Flashlight off
      - Example: `/camera?task=flashlight_off`
      - Response:
        - Content type: `HTML`
        - Content: `006: Flashlight off`
    - `stream` Camera livestream without flashlight<br>
      __IMPORTANT__: A running stream is blocking the entire web interface (to limit memory usage for 
                     this function). Please ensure to close stream before continue with WebUI.
      - Example: `/camera?task=stream`
      - Response:
        - Content type: `HTML`
        - Content: `007: Camera livestream`
    - `stream_flashlight` Camera livestream with flashlight<br>
      __IMPORTANT__: A running stream is blocking the entire web interface (to limit memory usage for 
                     this function). Please ensure to close stream before continue with WebUI.
      - Example: `/camera?task=stream_flashlight`
      - Response:
        - Content type: `HTML`
        - Content: `008: Camera livestream with flashlight`
