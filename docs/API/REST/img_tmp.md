[Overview](_OVERVIEW.md) 

### REST API endpoint: img_tmp

`http://IP-ADDRESS/img_tmp`


Get prcoess relevant images directly from memory.

Be aware: Defining the trigger time keep in mind that the images are prcoessed by the device on the fly all during processing. Loading them the 'wrong' time they can be fragmented or distroted for a short time.


Payload:
- Actual raw image: `/img_tmp/raw.jpg`
- Last image after alignment: `/img_tmp/alg.jpg`
- Last image after alignment incl. ROI overlay: `/img_tmp/alg_roi.jpg`

Response:
  - Content type: `image/jpeg`
  - Content: JPG file
