let jsonConfig = {}; // Config: Object in JSON notation
let jsonConfigModifiedDelta = {}; // Config: Object in JSON notation (modified parameter only)


async function fileCopyOnServer(_source, _target, _domainname = "", async = false)
{
    return new Promise(function (resolve, reject) {
        let url = _domainname + "/editflow?task=copy&in=" + _source + "&out=" + _target;

        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4) {
                if (this.status >= 200 && this.status < 300) {
                        return resolve("Copy file request successful");
                }
                if (this.status > 400) {
                        firework.launch("Copy file request failed (Response status: " + this.status +
                                            "). Repeat action or check logs.", 'danger', 30000);
                        console.error("Copy file request failed. Response status: " + this.status);
                        return reject("Copy file request failed");
                }
            }
        };

        if (async)
            xhttp.timeout = 10000;  // 10 seconds

        xhttp.open("GET", url, async);
        xhttp.send();
    });
}


function fileDeleteOnServer(_filename, _domainname = "")
{
    let okay = false;
    let url = _domainname + "/delete" + _filename;

    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (xhttp.readyState == 4) {
            if (xhttp.status >= 200 && xhttp.status < 300) {
                okay = true;
            }
            else {
                firework.launch("File delete request failed (Response status: " + this.status +
                                    "). Repeat action or check logs.", 'danger', 30000);
                console.error("File delete request failed. Response status: " + this.status);
                okay = false;
            }
        }
    };

    xhttp.open("POST", url, false);
    xhttp.send();

    return okay;
}


function uploadContent(_content, _filename, _domainname = "")
{
    let okay = false;
    let url = _domainname + "/upload" + _filename;

    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (xhttp.readyState == 4) {
            if (xhttp.status >= 200 && xhttp.status < 300) {
                okay = true;
            }
            else if (xhttp.status == 0) {
            firework.launch('File send request failed. Server closed the connection abruptly!', 'danger', 30000);
            }
            else {
                firework.launch("File send request failed (Response status: " + this.status +
                                    "). Repeat action or check logs.", 'danger', 30000);
                console.error("File send request failed. Response status: " + this.status);
                okay = false;
            }
        }
    };

    xhttp.open("POST", url, false);
    xhttp.send(_content);

    return okay;
}


async function getDataFileList()
{
    return new Promise(function (resolve, reject) {
        let url = getDomainname() + '/editflow?task=data';

        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4) {
                    if (this.status >= 200 && this.status < 300) {
                        return resolve(JSON.parse(xhttp.responseText));
                    }
                    else {
                        firework.launch("Data files request failed (Response status: " + this.status +
                                "). Repeat action or check logs.", 'danger', 30000);
                        console.error("Data files request failed. Response status: " + this.status);
                        return reject("Data files request failed");
                    }
            }
        };

        xhttp.timeout = 10000; // 10 seconds
        xhttp.open("GET", url, true);
        xhttp.send();
    });
}


async function getTfliteFileList()
{
    return new Promise(function (resolve, reject) {
        let url = getDomainname() + '/editflow?task=tflite';

        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4) {
                if (this.status >= 200 && this.status < 300) {
                        return resolve(JSON.parse(xhttp.responseText));
                }
                else {
                        firework.launch("TFLite files request failed (Response status: " + this.status +
                                "). Repeat action or check logs.", 'danger', 30000);
                        console.error("TFLite files request failed. Response status: " + this.status);
                        return reject("TFLite files request failed");
                }
            }
        };

        xhttp.timeout = 10000; // 10 seconds
        xhttp.open("GET", url, true);
        xhttp.send();
    });
}


async function getCertFileList()
{
    return new Promise(function (resolve, reject) {
        let url = getDomainname() + '/editflow?task=certs';

        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4) {
                if (this.status >= 200 && this.status < 300) {
                        return resolve(JSON.parse(xhttp.responseText));
                }
                else {
                        firework.launch("Cert files request failed (Response status: " + this.status +
                                "). Repeat action or check logs.", 'danger', 30000);
                        console.error("Cert files request failed. Response status: " + this.status);
                        return reject("Cert files request failed");
                }
            }
        };

        xhttp.timeout = 10000; // 10 seconds
        xhttp.open("GET", url, true);
        xhttp.send();
    });
}


function reloadConfig()
{
    let url = getDomainname() + '/config?task=reload';

    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4) {
            if (this.status >= 200 && this.status < 300) {
                if (xhttp.responseText.substring(0,3) == "001" || xhttp.responseText.substring(0,3) == "002" ||
                        xhttp.responseText.substring(0,3) == "003")
                {
                        firework.launch('Configuration updated and applied', 'success', 2000);
                }
                else if (xhttp.responseText.substring(0,3) == "004") {
                        firework.launch('Configuration updated and get applied after actual cycle is completed', 'success', 3000);
                }
                else if (xhttp.responseText.substring(0,3) == "099") {
                        firework.launch('Configuration updated, but cannot get applied (flow not initialized)', 'danger', 30000);
                }
            }
            else {
                firework.launch("Configuration update failed (Response status: " + this.status +
                                "). Repeat action or check logs.", 'danger', 30000);
                console.error("Configuration update failed. Response status: " + this.status);
            }
        }
    };

    xhttp.timeout = 10000;  // 10 seconds
    xhttp.open("GET", url, true);
    xhttp.send();
}



async function saveConfig(data)
{
    return new Promise(function (resolve, reject) {
        let url = getDomainname() + "/config";

        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (xhttp.readyState == 4) {
                if (xhttp.status >= 200 && xhttp.status < 300) {
                        sessionStorage.setItem("jsonConfigData", xhttp.responseText); // Store the object into storage
                        return resolve();
                }
                else if (xhttp.status == 0) {
                        firework.launch('Save config failed failed. Server closed the connection abruptly!', 'danger', 30000);
                }
                else {
                        firework.launch("Save config failed (Response status: " + this.status +
                                            "). Repeat action or check logs.", 'danger', 30000);
                        console.error("Save config failed. Response status: " + this.status);
                        return reject("Save config failed");
                }
            }
        };

        xhttp.open("POST", url, true);
        xhttp.send(data);
    });
}


async function loadConfig()
{
    return new Promise(function (resolve, reject) {
        let url = getDomainname() + '/config';

        let xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4) {
                if (this.status >= 200 && this.status < 300) {
                        sessionStorage.setItem("jsonConfigData", xhttp.responseText); // Store the object into storage
                        return resolve();
                }
                else {
                        firework.launch("Loading config failed (Response status: " + this.status +
                                    "). Repeat action or check logs.", 'danger', 30000);
                        console.error("Loading config failed. Response status: " + this.status);
                        return reject("Loading config failed");
                }
            }
        };

        xhttp.timeout = 10000;  // 10 seconds
        xhttp.open("GET", url, true);
        xhttp.send();
    });
}


function getConfigFromStorage(type = 0)
{
    jsonConfig = JSON.parse(sessionStorage.getItem("jsonConfigData"));
    jsonConfigKeyified = jsonObjectKeyify(jsonConfig);

    switch (type) {
        case 0:
            return jsonConfig;
        case 1:
            return jsonConfigKeyified;
        case 2:
            return [jsonConfig, jsonConfigKeyified]
    }
}


function noConfigInStorage()
{
    return sessionStorage.getItem("jsonConfigData") === null;
}


function jsonObjectKeyify(jsonObject)
{
    let values = [];

    const keyify = (t, pre = []) =>
        Object(t) === t ? Object.entries(t).flatMap(([k, v]) =>
            keyify(v, [...pre, k])) : pre.join("_")


    function traverse_it(jsonObj)
    {
        for(let prop in jsonObj) {
            if(typeof jsonObj[prop] == 'object') { // object
                traverse_it(jsonObj[prop]);
            }
            else { // property
                values.push(jsonObj[prop]);
            }
        }
    }
    traverse_it(jsonObject);

    return keyify(jsonObject).map((x, i) => [x, values[i]]);
}


function deepMergeObjects(obj1, obj2)
{
    const result = {};

    for (const key in obj2) {
        if (obj2.hasOwnProperty(key)) {
            if (typeof obj2[key] === "object" && obj1.hasOwnProperty(key) && typeof obj1[key] === "object") {
                result[key] = deepMergeObjects(obj1[key], obj2[key]);
            }
            else {
                result[key] = obj2[key];
            }
        }
    }

    for (const key in obj1) {
        if (obj1.hasOwnProperty(key) && !result.hasOwnProperty(key)) {
            if (typeof obj1[key] === "object") {
                result[key] = deepMergeObjects(obj1[key], {});
            }
            else {
                result[key] = obj1[key];
            }
        }
    }

    return result;
}


// This function is taken from https://stackoverflow.com/a/54733755/5459839
function deepSet(obj, path, value, integer)
{
    if (Object(obj) !== obj)
        return obj; // When obj is not an object

    // If not yet an array, get the keys from the string-path
    if (!Array.isArray(path))
        path = path.toString().match(/[^.[\]]+/g) || [];

    path.slice(0,-1).reduce((a, c, i) => // Iterate all of them except the last one
        Object(a[c]) === a[c] // Does the key exist and is its value an object?
            // Yes: then follow that path
            ? a[c]
            // No: create the key. Is the next key a potential array-index?
            : a[c] = Math.abs(path[i+1])>>0 === +path[i+1]
                ? [] // Yes: assign a new array object
                : {}, // No: assign a new plain object
        // Finally assign the value to the last key
        obj)[path[path.length-1]] = integer ? parseInt(value) : (value === "true" || value === "false") ?
                                            (value === "true") ? true : false : value;

    return obj; // Return the top-level object to allow chaining
}


function gatherChangedParameter(form, integer = false)
{
    const formData = new FormData(form);
    const data = {};
    for (const [path, value] of formData) {
        deepSet(data, path, value, integer);
    }
    jsonConfigModifiedDelta = deepMergeObjects(jsonConfigModifiedDelta, data);
    console.log(jsonConfigModifiedDelta) //@TODO: Remove log
}


function parseCurrentParametrization(paramsKeyified)
{
    for (let i in paramsKeyified) {
        let el = document.getElementById((paramsKeyified[i][0] + "_value").toLowerCase());

        if (el == null)
            continue;

        if (el.tagName.toLowerCase() == "select") {
            for(let j, k = 0; j = el.options[k]; k++) {
                if(j.value.toLowerCase() == paramsKeyified[i][1].toString() || j.value.toLowerCase() == paramsKeyified[i][1]) {
                        el.selectedIndex = k;
                        break;
                }
            }
        }
        else {
            el.value = paramsKeyified[i][1];
        }
    }
}


function loadImage(url, altUrl)
{
    let timer;
    function clearTimer() {
        if (timer) {
            clearTimeout(timer);
            timer = null;
        }
    }

    function handleFailed() {
        // kill previous error handlers
        this.onload = this.onabort = this.onerror = function() {};
        // stop existing timer
        clearTimer();
        // switch to alternate url
        if (this.src === url) { // Alternative image --> img_not_available.png as BASE64
            this.src = altUrl;
        }
    }

    let img = new Image();
    img.onerror = img.onabort = handleFailed;
    img.onload = function() {
        clearTimer();
    };
    img.src = url;
    timer = setTimeout(function(theImg) {
        return function() {
            handleFailed.call(theImg);
        };
    }(img), 2000);
    return(img);
}


function dataURLtoBlob(dataurl)
{
    let arr = dataurl.split(','), mime = arr[0].match(/:(.*?);/)[1],
        bstr = atob(arr[1]), n = bstr.length, u8arr = new Uint8Array(n);
    while(n--){
        u8arr[n] = bstr.charCodeAt(n);
    }
    return new Blob([u8arr], {type:mime});
}


function SaveCanvasToImage(_canvas, _filename, _delete = true, _domainname = "")
{
    let JPEG_QUALITY = 0.8;
    let dataUrl = _canvas.toDataURL('image/jpeg', JPEG_QUALITY);
    let rtn = dataURLtoBlob(dataUrl);

    if (_delete) {
        fileDeleteOnServer(_filename, _domainname);
    }

    uploadContent(rtn, _filename, _domainname);
}


function CreateNumberSequence(type, sequenceName)
{
    if (sequenceName.length == 0) {
        return "Number sequence name must not be empty";
    }

    if ((sequenceName.indexOf(".") >= 0) || (sequenceName.indexOf(",") >= 0) ||
    (sequenceName.indexOf(" ") >= 0) || (sequenceName.indexOf("\"") >= 0)) {
        return "Number sequence name must not contain , . \" or a space";
    }

    for (i = 0; i < jsonConfigModifiedDelta[type]["sequence"].length; ++i) {
        if (jsonConfigModifiedDelta[type]["sequence"][i]["sequencename"] == sequenceName)
            return "Number sequence name is already existing";
    }

    jsonConfigModifiedDelta[type]["sequence"].push({"sequenceid": -1, "sequencename": sequenceName, "roi": []});
    jsonConfigModifiedDelta.numbersequences.sequence.push({"sequenceid": -1, "sequencename": sequenceName});
    return "";
}


function RenameNumberSequence(type, idx, sequenceNameNew)
{
    if (sequenceNameNew.length == 0) {
        return "Number sequence name must not be empty";
    }

    if ((sequenceNameNew.indexOf(".") >= 0) || (sequenceNameNew.indexOf(",") >= 0) ||
        (sequenceNameNew.indexOf(" ") >= 0) || (sequenceNameNew.indexOf("\"") >= 0)) {
        return "Number sequence name must not contain , . \" or a space";
    }

    for (i = 0; i < jsonConfigModifiedDelta[type]["sequence"].length; ++i) {
        if (jsonConfigModifiedDelta[type]["sequence"][i]["sequencename"] == sequenceNameNew)
            return "Number sequence name is already existing";
    }

    if (idx == -1)
        return "Invalid number sequence index";

    jsonConfigModifiedDelta[type]["sequence"][idx]["sequencename"] = sequenceNameNew;
    jsonConfigModifiedDelta.numbersequences.sequence[idx].sequencename = sequenceNameNew;
    return "";
}


function DeleteNumberSequence(type, idx)
{
    if (jsonConfigModifiedDelta[type]["sequence"].length == 1)
        return "One number sequence is mandatory. Therefore this cannot be deleted"

    jsonConfigModifiedDelta[type]["sequence"].splice(idx, 1);
    jsonConfigModifiedDelta.numbersequences.sequence.splice(idx, 1);

    return "";
}


function CreateROI(type, sequenceName, x, y, dx, dy)
{
    for (i = 0; i < jsonConfigModifiedDelta[type]["sequence"].length; ++i) {
        if (jsonConfigModifiedDelta[type]["sequence"][i]["sequencename"] == sequenceName) {
            jsonConfigModifiedDelta[type]["sequence"][i]["roi"].push({"x": parseInt(x),
                "y": parseInt(y), "dx": parseInt(dx), "dy": parseInt(dy), "ccw": false, "ar": dx/dy});

            return "";
        }
    }

    return "Number sequence not existing. ROI cannot be created"
}