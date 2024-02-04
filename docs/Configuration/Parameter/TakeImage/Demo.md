# Parameter: Demo Mode

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Demo Mode           | Demo
| Default Value     | `false`             | `false`
| Input Options     | `false`<br>`true`   | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

Enable this feature to use demo images instead of the real camera images.
This can be helpful for device testing. <br>
Make sure to have a `/demo` folder on your SD-Card and it contains the expected files.
Check [documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Demo-Mode) for more details. 


!!! Note
    Even demo files are used the camera module is still needed. After image is taken
    by camera the framebuffer content will be replaced by the demo image content.
