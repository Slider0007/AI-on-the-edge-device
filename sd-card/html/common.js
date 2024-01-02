/* The WebUI can also be executed on a local webserver for development purposes, e.g. XAMPP.
* Configure the physical device IP which shall be used for communication and call http://localhost
* NOTE: And you also might have to disable CORS in your webbrowser.
* IMPORTANT: For regular WebUI operation this IP parameter is not needed at all!
*/
var DUTDeviceIP = "192.168.2.68";      // Set the IP of physical device under test
var TestEnvironmentActive = false;
 

/* Returns the domainname with prepended protocol.
* E.g. http://watermeter.fritz.box or http://192.168.1.5
*/
function getDomainname()
{
    var domainname;

    // NOTE: The if condition cannot be used in this way: if (((host == "127.0.0.1") || (host == "localhost") || (host == ""))
    //       This breaks access through a forwarded port: https://github.com/jomjol/AI-on-the-edge-device/issues/2681 
    if (window.location.hostname == "localhost") {
         console.log("Test environment active! Device IP: " + DUTDeviceIP);
         domainname = "http://" + DUTDeviceIP
         TestEnvironmentActive = true;
    }
    else {
        domainname = window.location.protocol + "//" + window.location.hostname;
        if (window.location.port != "") {
            domainname = domainname + ":" + window.location.port;
        }
    }

    return domainname;
}


function getTestEnvironmentActive()
{
    return TestEnvironmentActive;
}


function UpdatePage(_dosession = true){
    var zw = location.href;
    zw = zw.substring(0, zw.indexOf("?"));
    if (_dosession) {
        window.location = zw + '?' + Math.floor((Math.random() * 1000000) + 1); 
    }
    else {
        window.location = zw; 
    }
}

        
function LoadHostname()
{
    var url = getDomainname() + '/info?type=Hostname';   

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status >= 200 && this.status < 300) {
                hostname = xhttp.responseText;
                document.title = hostname + " | AI on the Edge";
                document.getElementById("id_title").innerHTML  += hostname;
            }
        }
    };

    xhttp.timeout = 10000;  // 10 seconds
    xhttp.open("GET", url, true);
    xhttp.send();
}


var fwVersion = "";
var webUiVersion = "";

function LoadFwVersion()
{
    var url = getDomainname() + '/info?type=FirmwareVersion';
    
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status >= 200 && this.status < 300) {
                fwVersion = xhttp.responseText;
                document.getElementById("Version").innerHTML  = "Slider0007 Fork | " + fwVersion;
                //console.log(fwVersion);
                compareVersions();
            }
            else {
                fwVersion = "";
            }
        }
    };

    xhttp.timeout = 10000;  // 10 seconds
    xhttp.open("GET", url, true);
    xhttp.send();
}


function LoadWebUiVersion()
{
    var url = getDomainname() + '/info?type=HTMLVersion';

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status >= 200 && this.status < 300) {
                webUiVersion = xhttp.responseText;
                //console.log("Web UI Version: " + webUiVersion);
                compareVersions();
            }
            else {
                webUiVersion = "";
            }
        }
    };

    xhttp.timeout = 10000;  // 10 seconds
    xhttp.open("GET", url, true);
    xhttp.send();
}


function compareVersions()
{
    if (fwVersion == "" || webUiVersion == "") {
        return;
    }

    arr = fwVersion.split(" ");
    fWGitHash = arr[arr.length - 1].substring(0, 7);
    arr = webUiVersion.split(" ");
    webUiHash = arr[arr.length - 1].substring(0, 7);
    //console.log("FW Hash: " + fWGitHash + ", Web UI Hash: " + webUiHash);
    
    if (fWGitHash != webUiHash) {
        firework.launch("Web interface version does not match the firmware version. " + 
                        "It's strongly advised to use matching version. Check logs for more details.", 
                        'warning', 10000);
    }
}
