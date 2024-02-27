[Overview](_OVERVIEW.md) 

## REST API endpoint: editflow

`http://IP-ADDRESS/editflow`


Helper API to interact and setup the process

Payload:
- `task` Task to perform
  Available options:
  - `data` Get list of data files
    - No additional parameter needed
  - `tflite` Get list of tflite files
    - No additional parameter needed
  - `copy`
    - Copy a file from source to destination
    - Mandatory parameter: `in`, `out`
      - `in` Filename with path of source file
        - Example: `/config.ini/ref0.jpg`
      - `out` Filename with path of destination file
        - Example: `/img_tmp/ref0.jpg`
    - Example: `/editflow?task=copy&in=/config.ini/ref0.jpg&out=/img_tmp/ref0.jpg`
  - `cutref`
    - Cut a marked area out of an image and save to file
    - Mandatory parameter: `in`, `out`, `x`, `y`, `dx`, `dy`
      - `in` Filename with path of source file
        - Example: `/config/reference.jpg`
      - `out` Filename with path of destination file
        - Example: `/img_tmp/ref0.jpg`
      - `x` Start x-coordinate of image
        - Example: `152`
      - `y` Start y-coordinate of image
        - Example: `138`
      - `dx` Dx of desired cut out
        - Example: `44`
      - `dy` Dy of desired cut out
        - Example: `17`
    - Example: `/editflow?task=cutref&in=/config/reference.jpg&out=/img_tmp/ref0_org.jpg&x=152&y=138&dx=44&dy=17`


Response:
- Content type: `HTML`
- Content: Query response
