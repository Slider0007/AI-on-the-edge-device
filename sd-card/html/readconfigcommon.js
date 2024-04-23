function reload_config()
{
     var url = getDomainname() + '/reload_config';

     var xhttp = new XMLHttpRequest();
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


function dataURLtoBlob(dataurl)
{
     var arr = dataurl.split(','), mime = arr[0].match(/:(.*?);/)[1],
          bstr = atob(arr[1]), n = bstr.length, u8arr = new Uint8Array(n);
     while(n--){
          u8arr[n] = bstr.charCodeAt(n);
     }
     return new Blob([u8arr], {type:mime});
}


async function FileCopyOnServer(_source, _target, _domainname = "", async = false)
{
     return new Promise(function (resolve, reject) {
          var url = _domainname + "/editflow?task=copy&in=" + _source + "&out=" + _target;

          var xhttp = new XMLHttpRequest();
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


function FileDeleteOnServer(_filename, _domainname = "")
{
     var okay = false;
     var url = _domainname + "/delete" + _filename;

     var xhttp = new XMLHttpRequest();
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


function FileSendContent(_content, _filename, _domainname = "")
{
     var okay = false;
     var url = _domainname + "/upload" + _filename;

     var xhttp = new XMLHttpRequest();
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


function SaveCanvasToImage(_canvas, _filename, _delete = true, _domainname = "")
{
     var JPEG_QUALITY=0.8;
     var dataUrl = _canvas.toDataURL('image/jpeg', JPEG_QUALITY);
     var rtn = dataURLtoBlob(dataUrl);

     if (_delete) {
          FileDeleteOnServer(_filename, _domainname);
     }

     FileSendContent(rtn, _filename, _domainname);
}


function SaveConfigToServer(_domainname)
{
     // leere Zeilen am Ende löschen
     var zw = config_split.length - 1;
     while (config_split[zw] == "") {
          config_split.pop();
     }

     var _config_gesamt = "";
     for (var i = 0; i < config_split.length; ++i)
     {
          _config_gesamt = _config_gesamt + config_split[i] + "\n";
     }

     // Save to temporary file and then promote to config.ini on firmware level (server_file.cpp)
     // -> Reduce data loss risk (e.g. network connection got interrupted during data transfer)
     FileSendContent(_config_gesamt, "/config/config.tmp", _domainname);
}



function loadImage(url, altUrl)
{
    var timer;
    function clearTimer() {
        if (timer) {
            clearTimeout(timer);
            timer = null;
        }
    }

    function handleFail() {
        // kill previous error handlers
        this.onload = this.onabort = this.onerror = function() {};
        // stop existing timer
        clearTimer();
        // switch to alternate url
        if (this.src === url) {
            this.src = altUrl;
        }
    }

    var img = new Image();
    img.onerror = img.onabort = handleFail;
    img.onload = function() {
        clearTimer();
    };
    img.src = url;
    timer = setTimeout(function(theImg) {
        return function() {
            handleFail.call(theImg);
        };
    }(img), 10000);
    return(img);
}


function ZerlegeZeile(input, delimiter = " =\t\r")
{
     var Output = Array(0);
//          delimiter = " =,\t";


     /* The input can have multiple formats:
          *  - key = value
          *  - key = value1 value2 value3 ...
          *  - key value1 value2 value3 ...
          *
          * Examples:
          *  - ImageSize = VGA
          *  - IO0 = input disabled 10 false false
          *  - main.dig1 28 144 55 100 false
          *
          * This causes issues eg. if a password key has a whitespace or equal sign in its value.
          * As a workaround and to not break any legacy usage, we enforce to only use the
          * equal sign, if the key is "password"
          */
     if (input.includes("password") || input.includes("Token")) { // Line contains a password, use the equal sign as the only delimiter and only split on first occurrence
          var pos = input.indexOf("=");
          delimiter = " \t\r"
          Output.push(trim(input.substr(0, pos), delimiter));
          Output.push(trim(input.substr(pos +1, input.length), delimiter));
     }
     else { // Legacy Mode
          input = trim(input, delimiter);
          var pos = findDelimiterPos(input, delimiter);
          var token;
          while (pos > -1) {
               token = input.substr(0, pos);
               token = trim(token, delimiter);
               Output.push(token);
               input = input.substr(pos+1, input.length);
               input = trim(input, delimiter);
               pos = findDelimiterPos(input, delimiter);
          }
          Output.push(input);
     }

     return Output;

}


function findDelimiterPos(input, delimiter)
{
     var pos = -1;
     var zw;
     var akt_del;

     for (var anz = 0; anz < delimiter.length; ++anz)
     {
          akt_del = delimiter[anz];
          zw = input.indexOf(akt_del);
          if (zw > -1)
          {
               if (pos > -1)
               {
                    if (zw < pos)
                         pos = zw;
               }
               else
                    pos = zw;
          }
     }
     return pos;
}



function trim(istring, adddelimiter)
{
     while ((istring.length > 0) && (adddelimiter.indexOf(istring[0]) >= 0)) {
          istring = istring.substr(1, istring.length-1);
     }

     while ((istring.length > 0) && (adddelimiter.indexOf(istring[istring.length-1]) >= 0)) {
          istring = istring.substr(0, istring.length-1);
     }

     return istring;
}