/* The WebUI can also be executed on a local webserver for development purposes, e.g. XAMPP.
* Configure the physical device IP which shall be used for communication and call http://localhost
* NOTE: And you also might have to disable CORS in your webbrowser.
* IMPORTANT: For regular WebUI operation this IP parameter is not needed at all!
*/
let DUTDeviceIP = "192.168.2.66";      // Set the IP of physical device under test
let TestEnvironmentActive = false;


/* Returns the domainname with prepended protocol.
* E.g. http://watermeter.fritz.box or http://192.168.1.5
*/
function getDomainname()
{
    let domainname;

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
    let zw = location.href;
    zw = zw.substring(0, zw.indexOf("?"));
    if (_dosession) {
        window.location = zw + '?' + Math.floor((Math.random() * 1000000) + 1);
    }
    else {
        window.location = zw;
    }
}


// Used in overview.html + sys_info.html
function formatUptime(uptime)
{
    let uptime_days = Math.floor(uptime / (3600*24));
    let uptime_hours = Math.floor(Math.floor((uptime - uptime_days * 3600*24) / (3600)));
    let uptime_minutes = Math.floor((uptime - uptime_days * 3600*24 - uptime_hours * 3600) / (60));
    let uptime_seconds = uptime - uptime_days * 3600*24 - uptime_hours * 3600 - uptime_minutes * 60;

    return uptime_days + "d " + uptime_hours + "h " +uptime_minutes + "m " +uptime_seconds + "s";
}


/* Remove hidden class (show element) when condition is TRUE */
function setClassVisibility(classname, condition)
{
    let el = document.getElementsByClassName(classname);

    if (el.length == 0)
        return;

    for (j = 0; j < el.length; ++j) {
        if (condition === 'true' || condition === true) {
            el[j].classList.remove("hidden");
        }
        else {
            el[j].classList.add("hidden");
        }
    }
}


/* Set attribute (enable element) when condition is TRUE */
function setElVisibility(el, condition, fade = false)
{
    $(el).prop('hidden', !condition);

    if (!fade)
        return;

    condition ? $(el).find("*").fadeIn(250) :$(el).find("*").fadeOut(250);
}


/* Set attribute (enable element) when condition is TRUE */
function setTreeEnabled(el, condition, fade = false)
{
    $(el).find("input,button,select").attr("disabled", condition == false || condition == "false");
    $(el).find("*").fadeTo(400, condition ? 1.0 : 0.85);

    if (!fade)
        return;

    condition ? $(el).find("*").fadeIn(400) :$(el).find("*").fadeOut(400);
}


/* Remove disabled class or attribute (enable element) when condition is TRUE
   Only class elements + two underlaying children
*/
function setClassEnabled(classname, condition)
{
    let el = document.getElementsByClassName(classname);

    if (el.length == 0)
        return;

    for (j = 0; j < el.length; ++j) {
        if (condition === 'true' || condition === true) {
            el[j].classList.remove("disabled");
            el[j].disabled = false;
            el[j].children[0].removeAttribute("disabled");
            el[j].children[0].children[0].removeAttribute("disabled");
        }
        else {
            el[j].classList.add("disabled");
            el[j].disabled = true;
            el[j].children[0].setAttribute("disabled", "disabled");
            el[j].children[0].children[0].setAttribute("disabled", "disabled");
        }
    }
}


function triggerClassElementOnChange(classname)
{
    let el = document.getElementsByClassName(classname);

    if (el.length == 0)
        return;

    for (i = el.length - 1; i >= 0; i--) {
        el[i].onchange();
    }
}
