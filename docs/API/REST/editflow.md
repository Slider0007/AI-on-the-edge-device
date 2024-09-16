[Overview](_OVERVIEW.md) 

## REST API endpoint: editflow

`http://IP-ADDRESS/editflow`


Helper API to interact and setup the process

Payload:
- `task` Task to perform
  Available options:
  - `data` Get JSON array of all available data files
    - No additional parameter needed
    - Response:
      - Content type: `JSON`
      - Content: `{"files": [filename 1, filename 2, ...]}`
  - `tflite` Get JSON array of all available CNN model files
    - No additional parameter needed
    - Response:
      - Content type: `JSON`
      - Content: `{"files": [filename 1, filename 2, ...]}`
  - `copy`
    - Copy a file from source to destination
    - Mandatory parameter: `in`, `out`
      - `in` Filename with path of source file
        - Example: `/config/marker1.jpg`
      - `out` Filename with path of destination file
        - Example: `/img_tmp/marker1.jpg`
    - Example: `/editflow?task=copy&in=/config/marker1.jpg&out=/img_tmp/marker1.jpg`
    - Response:
      - Content type: `HTML`
      - Content: Query response status text
  - `cutref`
    - Cut a marked area out of an image and save to file
    - Mandatory parameter: `in`, `out`, `x`, `y`, `dx`, `dy`
      - `in` Filename with path of source file
        - Example: `/config/reference.jpg`
      - `out` Filename with path of destination file
        - Example: `/img_tmp/marker1.jpg`
      - `x` Start x-coordinate of image
        - Example: `152`
      - `y` Start y-coordinate of image
        - Example: `138`
      - `dx` Dx of desired cut out
        - Example: `44`
      - `dy` Dy of desired cut out
        - Example: `17`
    - Example: `/editflow?task=cutref&in=/config/reference.jpg&out=/img_tmp/marker1_org.jpg&x=152&y=138&dx=44&dy=17`
    - Response:
      - Content type: `HTML`
      - Content: Query response status text

