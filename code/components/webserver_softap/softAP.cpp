#include "softAP.h"
#include "../../include/defines.h"

#include <string>
#include <unistd.h>

#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_log.h>

#include "network_main.h"
#include "connect_wlan.h"
#include "configClass.h"
#include "webserver.h"
#include "helper.h"
#include "statusled.h"
#include "server_ota.h"


static const char *TAG = "WLAN_AP";

static volatile bool credentialsSet = false;
static volatile bool SDCardContentExisting = false;
static volatile bool apModeActive = false;

static const char PAGE_HEAD[] = R"HTML(<!DOCTYPE html><html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Device Provisioning</title><style>
:root{color-scheme:light;--card-radius:.6rem;--input-radius:.3rem;--transition-speed:.2s;
--surface-header:#0f172a;--surface-header-gradient:#1e3a5f;--text-on-dark:#fff;
--primary:#0366d6;--success:#10b981;--error:#ef4444;
--surface:rgba(211,211,211,.667);--bg:#fff;--bg-light:#e2e8f0;--border:#cbd5e1;
--shadow:0 4px 12px rgba(20,30,50,.3);--text-main:#1b1f23;--text-secondary:#64748b;
--info-box-bg:#e7f3ff;--info-box-border:#007bff}
@media(prefers-color-scheme:dark){:root{color-scheme:dark;
--surface:#334155;--bg:#0f172a;--bg-light:#1e293b;--border:rgba(51,65,85,.31);
--shadow:0 4px 12px rgba(70,70,70,.3);--text-main:#f9fafb;--text-secondary:#9ca3af;
--info-box-bg:rgba(131,166,206,.15);--info-box-border:#38bdf8}}
*{box-sizing:border-box}
html,body{max-width:1024px;margin:0;padding:0;font-family:Arial,sans-serif;font-size:16px;background:var(--bg);color:var(--text-main);
-webkit-text-size-adjust:100%;text-size-adjust:100%}
@media(min-width:721px){html,body{font-size:14px}}
.header{background:linear-gradient(0deg,var(--surface-header) 0%,var(--surface-header-gradient) 100%);
color:var(--text-on-dark);padding:10px 15px;line-height:1.3}
.brand-font{font-size:1.5rem;font-weight:bold;margin:0;letter-spacing:.01em}
.device-name{font-size:1rem;color:#c7d2fe;margin:0}
main{max-width:640px;margin:0 auto;padding:16px}
h2{font-size:1.2rem;margin:0 0 .6em}
.steps{display:flex;gap:8px;margin:0 0 16px;padding:0;list-style:none}
.steps li{flex:1;padding:10px 8px;background:var(--surface);color:var(--text-secondary);
border-radius:var(--input-radius);text-align:center;font-weight:bold}
body[data-step="1"] .s1,body[data-step="2"] .s2{background:var(--primary);color:#fff}
body[data-step="2"] .s1{background:var(--success);color:#fff}
body[data-step="2"] .s1::before{content:"\2713 "}
.card{background:var(--bg-light);border:1px solid var(--border);border-radius:var(--card-radius);
box-shadow:var(--shadow);padding:16px}
label{display:block;font-weight:bold;margin:12px 0 4px}
label.chk{font-weight:normal;margin:8px 0 0}
input[type=text],input[type=password]{width:100%;padding:10px;border:1px solid var(--surface);
border-radius:var(--input-radius);background:var(--bg);color:var(--text-main);font:inherit}
input[type=file]{width:100%;margin:8px 0 0;color:var(--text-main)}
.hint{margin:10px 0 0;padding:8px 10px;font-size:.9rem;background:var(--info-box-bg);
border-left:4px solid var(--info-box-border);border-radius:var(--input-radius)}
.button{color:var(--text-main);font-family:inherit;font-size:inherit;background:var(--surface);
border:1px solid var(--border);width:160px;padding:5px;border-radius:4px;cursor:pointer;text-align:center;
transition:opacity var(--transition-speed),filter var(--transition-speed)}
.btn-wide{width:100%;padding:10px;font-size:1.1rem;margin-top:16px}
.btn-action{background:var(--primary);text-transform:uppercase;color:#fff;font-weight:bold;border:1px solid transparent}
.button:hover:not(:disabled){filter:brightness(.9)}
.button.btn-action:hover:not(:disabled){filter:brightness(1.1)}
.button:disabled{cursor:not-allowed;filter:grayscale(1);opacity:.25}
progress{width:100%;height:10px;margin-top:16px;accent-color:var(--primary)}
#msg{margin:12px 0 0;min-height:1.5em;font-weight:bold}
.ok{color:var(--success)}.err{color:var(--error)}
</style></head>)HTML";

static const char PAGE_COMMON[] = R"HTML(<div class="header"><p class="brand-font">AI-on-the-Edge Device [SLFork]</p>
<p class="device-name">Device Provisioning</p></div>
<main><ol class="steps"><li class="s1"> 1 &middot; NETWORK</li><li class="s2"> 2 &middot; FIRMWARE</li></ol>)HTML";

static const char PAGE_STEP1[] = R"HTML(<section class="card"><h2>WLAN Network</h2>
<p class="hint">The password is sent unencrypted over this setup network (Device Access Point)</p>
<label for="ssid">Network Name (SSID)</label>
<input id="ssid" type="text" maxlength="32" autocapitalize="none" autocomplete="off" spellcheck="false">
<label for="pwd">Password</label>
<input id="pwd" type="password" maxlength="63" autocomplete="off">
<label class="chk"><input type="checkbox" onchange="$('pwd').type=this.checked?'text':'password'"> Show password</label>
<button id="save" class="button btn-action btn-wide" onclick="save()">Save</button>
<progress id="prog" max="100" value="0" hidden></progress><p id="msg" role="status"></p></section>
<script>
async function save(){
  const ssid=$('ssid').value;
  if(!ssid){msg('Enter the network name','err');return}
  $('save').disabled=true;msg('Saving...');
  try{
    const r=await fetch('/config?ssid='+encodeURIComponent(ssid)+'&pwd='+encodeURIComponent($('pwd').value));
    const t=await r.text();
    if(!r.ok)throw new Error(t);
    if(t==='reboot'){finish('Settings saved');return}
    location.reload();
  }catch(e){$('save').disabled=false;msg('Save failed: '+e.message,'err')}
}</script>)HTML";

static const char PAGE_STEP2[] = R"HTML(<section class="card"><h2>Install Firmware Package (MCU + SD-CARD)</h2>
<p class="hint">Select <code>AI-on-the-edge-device__{Board Type}__*.zip</code>. The device reboots and installs it automatically (up to 2 min). Do not power off.</p>
<input id="file" type="file" accept=".zip">
<button id="up" class="button btn-action btn-wide" onclick="upload()">Upload &amp; Install</button>
<progress id="prog" max="100" value="0" hidden></progress><p id="msg" role="status"></p></section>
<script>
function upload(){
  const f=$('file').files[0];
  if(!f){msg('Select a file first','err');return}
  if(!f.name.includes('AI-on-the-edge-device__')&&!confirm('File name should contain "AI-on-the-edge-device__". Continue anyway?'))return;
  const fail=t=>{$('up').disabled=false;$('prog').hidden=true;msg('Upload failed: '+t,'err')};
  $('up').disabled=true;$('prog').hidden=false;msg('Uploading...');
  const x=new XMLHttpRequest();
  x.upload.onprogress=e=>{if(e.lengthComputable)$('prog').value=100*e.loaded/e.total};
  x.onload=()=>x.status==200?finish('Upload complete. Installing (up to 2 min).'):fail(x.responseText||x.status);  x.onerror=()=>fail('connection lost');
  x.open('POST','/ota/'+encodeURIComponent(f.name));x.send(f);
}</script>)HTML";

static const char PAGE_TAIL[] = R"HTML(</main><script>
const $=i=>document.getElementById(i),msg=(t,c)=>{const m=$('msg');m.textContent=t;m.className=c||''};
const AP=document.body.dataset.ap==='1';

// Device is rebooting. AP mode: AP vanishes, nothing to poll. Otherwise wait for the normal UI.
function finish(what){
  $('prog').hidden=true;
  if(AP){   // AP vanishes on reboot: nothing to poll
    msg(what+' Device is restarting. Reconnect to your default WLAN and open the device page manually (http://watermeter, http://watermeter.local or IP-Address)','ok');
    return;
  }
  msg(what+' Device is restarting. This page redirects automatically...','ok');
  $('prog').value=0;$('prog').hidden=false;
  checkStatus();
}

async function checkStatus(){
  const sleep=ms=>new Promise(r=>setTimeout(r,ms));
  const start=Date.now();
  while(Date.now()-start<120000){
    await sleep(2500);
    const elapsed=Date.now()-start;
    $('prog').value=Math.min(Math.round(elapsed/1250),100);
    if(elapsed<8000)continue;   // device still shutting down
    try{
      const ctl=new AbortController(),t=setTimeout(()=>ctl.abort(),2000);
      const r=await fetch('/index.html',{cache:'no-store',signal:ctl.signal});
      clearTimeout(t);
      // provisioning server answers every GET with 200 -> require the marker to be absent
      if(r.ok&&!r.headers.get('X-Provisioning')){
        $('prog').value=100;
        document.cookie='page=dashboard.html;path=/;SameSite=Lax';
        location.href='/index.html';
        return;
      }
    }catch(e){}
  }
  msg('Reboot takes unusually long. Reload this page or power cycle the device.','err');
}
</script></body></html>)HTML";

static esp_err_t ap_handler_main(httpd_req_t *req)
{
    char body[48];
    snprintf(body, sizeof(body), "<body data-step=\"%d\" data-ap=\"%d\">", credentialsSet ? 2 : 1, apModeActive ? 1 : 0);

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "X-Provisioning", "1");

    httpd_resp_send_chunk(req, PAGE_HEAD, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, body, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, PAGE_COMMON, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, credentialsSet ? PAGE_STEP2 : PAGE_STEP1, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, PAGE_TAIL, HTTPD_RESP_USE_STRLEN);
    return httpd_resp_send_chunk(req, NULL, 0);
}

esp_err_t ap_handler_config(httpd_req_t *req)
{
    char query[512];
    char ssidEnc[100]; // 32 B SSID, worst case %XX = 96
    char pwdEnc[200];  // 63 B passphrase, worst case %XX = 189

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "ssid", ssidEnc, sizeof(ssidEnc)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or oversized SSID");
        return ESP_FAIL;
    }
    const std::string ssid = urlDecode(std::string(ssidEnc));
    if (ssid.empty() || ssid.size() > 32) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID must be 1-32 bytes");
        return ESP_FAIL;
    }

    const esp_err_t pwdRet = httpd_query_key_value(query, "pwd", pwdEnc, sizeof(pwdEnc));
    if (pwdRet == ESP_ERR_HTTPD_RESULT_TRUNC) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Password too long");
        return ESP_FAIL;
    }

    auto &wlan = ConfigClass::getInstance()->cfgTmp()->sectionNetwork.wlan;
    wlan.ssid = ssid;
    if (pwdRet == ESP_OK) {
        wlan.password = urlDecode(std::string(pwdEnc));
        ConfigClass::getInstance()->saveMigDataToNVS("wlan_pw", wlan.password);
    }
    ConfigClass::getInstance()->persistConfig();
    credentialsSet = true;

    if (SDCardContentExisting) { // nothing to upload -> restart into normal mode
        httpd_resp_sendstr(req, "reboot");
        vTaskDelay(pdMS_TO_TICKS(500));
        doReboot();
        return ESP_OK;
    }

    httpd_resp_sendstr(req, "ok");
    return ESP_OK;
}

httpd_handle_t start_webserverAP(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 8192;
    config.lru_purge_enable = true; // captive-portal probes exhaust sockets otherwise
    config.recv_wait_timeout = 15;  // seconds; large upload over weak AP link

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start webserver (access point). Power cycle to retry...");
        doRebootOTA();
        return NULL;
    }

    httpd_uri_t apHandlerConfig = {
        .uri = "/config",
        .method = HTTP_GET,
        .handler = ap_handler_config,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &apHandlerConfig);

    httpd_uri_t apHandlerOta = {
        .uri = "/ota/*",
        .method = HTTP_POST,
        .handler = handler_ota,     // Reuse existing OTA handler (server_ota.cpp)
        .user_ctx = httpServerData, // Reuse existing httpServerData (webserver.cpp)
    };
    httpd_register_uri_handler(server, &apHandlerOta);

    httpd_uri_t apHandlerMain = {
        .uri = "*",
        .method = HTTP_GET,
        .handler = ap_handler_main,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &apHandlerMain);

    return server;
}

void startAPForDeviceProvisioning()
{
    SDCardContentExisting = fileExists("/sdcard/html/index.html");
    if (!SDCardContentExisting) {
        ESP_LOGW(TAG, "Unable to load HTML content: 'html/index.html' not found on SD card");
    }

    bool wlanSsidEmpty = (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_WLAN &&
                          ConfigClass::getInstance()->get()->sectionNetwork.wlan.ssid.empty());
    if (wlanSsidEmpty) {
        ESP_LOGW(TAG, "WLAN SSID empty");
    }

    if (!SDCardContentExisting || wlanSsidEmpty) {
        ESP_LOGI(TAG, "Init device provisioning");
        setStatusLed(AP_OR_OTA, 2, true);

#ifdef BOARD_FEATURE_ETHERNET
        if (getNetworkOpmodeType() == NETWORK_OPMODE_TYPE_ETHERNET) {
            initNetwork(); // Init network and wait for network connection
            while (!getNetworkConnectionState()) {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            credentialsSet = true; // Skip config of WLAN credentials
        }
        else {
            initWifiAp(true);
            apModeActive = true;
        }
#else
        initWifiAp(true);
        apModeActive = true;
#endif // BOARD_FEATURE_ETHERNET

        start_webserverAP();

        while (1) { // Idle until reboot
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
}

/** This function is only used to switch the Improv provisioning service where
 * step 1, WLAN configuration, is provisioned via serial / USB console,
 * continued with step 2 defined in softAP.cpp -> function 'ap_handler_main'
 */
void stopAPForDeviceProvisioning(void)
{
    forceStatusLedOff();
    deinitNetwork();
    apModeActive = false;
    credentialsSet = true; // Skip step 1
}

bool getDeviceProvisioningByApStarted()
{
    return apModeActive;
}
