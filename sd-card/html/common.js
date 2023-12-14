/* The WebUI can also be executed on a local webserver, e.g. XAMPP.
* Switch on the test environment and configure the physical device IP which shall be used for communication
* (NOTE: And you also might have to disable CORS in your webbrowser!)
*
* !!! Don't forget so switch this off again for regular operation !!!
*/
var activateTestEnvironment = false;    // Activate, if webserver shall be operated offloaded from ESP device (e.g. local webserver for testing purpose)
var DeviceIP = "192.168.2.68";          // Set the IP of physical device (only needed if 'activateTestEnvironment=true')
 

/* Returns the domainname with prepended protocol.
* Eg. http://watermeter.fritz.box or http://192.168.1.5
*/
function getDomainname()
{
    var domainname;

    // NOTE: The if condition cannot be used in this way: if (((host == "127.0.0.1") || (host == "localhost") || (host == ""))
    //       This breaks access through a forwarded port: https://github.com/jomjol/AI-on-the-edge-device/issues/2681 
     if (activateTestEnvironment) {
         console.log("Test environment active! Device IP: " + DeviceIP);
         domainname = "http://" + DeviceIP
    }
    else {
        domainname = window.location.protocol + "//" + window.location.hostname;
        if (window.location.port != "") {
            domainname = domainname + ":" + window.location.port;
        }
    }

    return domainname;
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
    var xhttp = new XMLHttpRequest();
    xhttp.addEventListener('load', function(event) {
        if (xhttp.status >= 200 && xhttp.status < 300) {
            hostname = xhttp.responseText;
            document.title = hostname + " | AI on the Edge";
            document.getElementById("id_title").innerHTML  += hostname;
        } 
        else {
            console.warn(request.statusText, request.responseText);
        }
    });

    try {
            url = getDomainname() + '/info?type=hostname';     
            xhttp.open("GET", url, true);
            xhttp.send();

    }
    catch (error)
    {
        //alert("Loading Hostname failed");
    }
}


var fwVersion = "";
var webUiVersion = "";

function LoadFwVersion()
{
    var xhttp = new XMLHttpRequest();
    xhttp.addEventListener('load', function(event) {
        if (xhttp.status >= 200 && xhttp.status < 300) {
            fwVersion = xhttp.responseText;
            document.getElementById("Version").innerHTML  = "Slider0007 Fork | " + fwVersion;
            //console.log(fwVersion);
            compareVersions();
        } 
        else {
            console.warn(request.statusText, request.responseText);
            fwVersion = "NaN";
        }
    });

    try {
        url = getDomainname() + '/info?type=firmware_version';     
        xhttp.open("GET", url, true);
        xhttp.send();
    }
    catch (error) {
        fwVersion = "NaN";
    }
}


function LoadWebUiVersion()
{
    var xhttp = new XMLHttpRequest();
    xhttp.addEventListener('load', function(event) {
        if (xhttp.status >= 200 && xhttp.status < 300) {
            webUiVersion = xhttp.responseText;
            //console.log("Web UI Version: " + webUiVersion);
            compareVersions();
        } 
        else {
            console.warn(request.statusText, request.responseText);
            webUiVersion = "NaN";
        }
    });

    try {
        url = getDomainname() + '/info?type=html_version';     
        //console.log("url: " + url);
        xhttp.open("GET", url, true);
        xhttp.send();
    }
    catch (error) {
        webUiVersion = "NaN";
    }
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
        firework.launch("The version of the web interface (" + webUiHash + 
            ") does not match the firmware version (" + 
            fWGitHash + ")! It is suggested to keep them on the same version!", 'warning', 30000);
    }
}
