/* ========================================================================== */
/* Environment                                                                */
/* ========================================================================== */

/* The WebUI can also be executed on a local webserver for development purposes, e.g. XAMPP.
 * Configure the physical device IP which shall be used for communication and call http://localhost
 * NOTE: And you also might have to disable CORS in your webbrowser.
 * IMPORTANT: For regular WebUI operation this IP parameter is not needed at all!
 */
const dutDeviceIP = '192.168.2.20'; // Set the IP of physical device under test
let testEnvironmentActive = false;

/* Returns the domainname with prepended protocol.
 * E.g. http://watermeter.fritz.box or http://192.168.1.5
 */
function getDomainname() {
  // NOTE: Do not special-case 127.0.0.1/empty host here, this breaks access through a forwarded port:
  //       https://github.com/jomjol/AI-on-the-edge-device/issues/2681
  if (window.location.hostname === 'localhost') {
    console.log('Test environment active! Device IP: ' + dutDeviceIP);
    testEnvironmentActive = true;
    return 'http://' + dutDeviceIP;
  }

  return window.location.origin; // protocol + hostname + optional port
}

function getTestEnvironmentActive() {
  return testEnvironmentActive;
}

function isDarkTheme() {
  const cl = document.documentElement.classList;
  if (cl.contains('dark')) return true;
  if (cl.contains('light')) return false;

  return window.matchMedia('(prefers-color-scheme: dark)').matches;
}


/* ========================================================================== */
/* Formatting + DOM helpers                                                   */
/* ========================================================================== */

const isObj = (v) => v !== null && typeof v === 'object';
const resolveEl = (el) => (typeof el === 'string' ? document.querySelector(el) : el);

function addZero(i) {
  return i < 10 ? '0' + i : i;
}

function formatUptime(uptime) {
  const s = Math.floor(uptime);
  const days = Math.floor(s / 86400);
  const hours = Math.floor((s % 86400) / 3600);
  const minutes = Math.floor((s % 3600) / 60);

  return `${days}d ${addZero(hours)}h ${addZero(minutes)}m ${addZero(s % 60)}s`;
}

function renderRssi(value) {
  const output = document.getElementById('wlan_rssi');
  if (!output) return;

  const bars = document.querySelectorAll('#rssi_svg .bar');
  const setBars = (count, dimOpacity) =>
    bars.forEach((bar, i) => {
      const active = i < count;
      bar.style.opacity = active ? '1' : dimOpacity;
      bar.style.fill = active ? 'var(--primary)' : 'currentColor';
    });

  if (value === null || isNaN(value)) {
    output.textContent = 'Offline';
    setBars(0, '0.1');
  } else if (value <= -255) {
    output.textContent = 'Disconnected';
    setBars(0, '0.1');
  } else {
    const label = value >= -55 ? 'Excellent' : value >= -67 ? 'Good' : value >= -78 ? 'Fair' : value >= -85 ? 'Weak' : 'Unreliable';
    output.textContent = `${label} (${value}dBm)`;
    const RSSI_THRESHOLDS = [-85, -78, -67, -55]; // Weak, Fair, Good, Excellent
    setBars(RSSI_THRESHOLDS.filter((t) => value >= t).length, '0.2');
  }
}

// Remove hidden class (show elements) when condition is TRUE
function setClassVisibility(classname, condition) {
  const show = condition === 'true' || condition === true;
  for (const el of document.getElementsByClassName(classname)) {
    el.classList.toggle('hidden', !show);
  }
}

// Show element (clear hidden attribute) when condition is TRUE
function setElVisibility(el, condition, fade = false) {
  const root = resolveEl(el);
  if (!root) return;

  root.hidden = !condition;

  if (fade) {
    root.querySelectorAll('*').forEach((node) => {
      node.style.transition = 'opacity 250ms';
      node.style.opacity = condition ? '1' : '0';
    });
  }
}

// Enable all inputs/buttons/selects in tree when condition is not false/'false'
function setTreeEnabled(el, condition, fade = false) {
  const root = resolveEl(el);
  if (!root) return;

  const enable = !(condition == false || condition == 'false');
  root.querySelectorAll('input,button,select').forEach((node) => {
    node.disabled = !enable;
  });

  root.querySelectorAll('*').forEach((node) => {
    node.style.transition = 'opacity 400ms';
    node.style.opacity = enable ? '1' : fade ? '0' : '0.85';
  });
}

// Remove disabled class + disabled attribute on all interactive children when condition is TRUE
function setClassEnabled(classname, condition) {
  const enable = condition === 'true' || condition === true;

  for (const container of document.getElementsByClassName(classname)) {
    container.classList.toggle('disabled', !enable);
    container.querySelectorAll('input, select, textarea, button, output').forEach((input) => {
      input.disabled = !enable; // reflects to the attribute
    });
  }
}

function triggerClassElementOnChange(classname) {
  Array.from(document.getElementsByClassName(classname))
    .reverse()
    .forEach((el) => {
      if (typeof el.onchange === 'function') el.onchange();
    });
}


/* ========================================================================== */
/* Input masks                                                                */
/* ========================================================================== */

// Helper: Backspace right behind a separator removes the separator itself
function eraseSeparatorOnBackspace(input, separators) {
  input.addEventListener('keydown', (e) => {
    if (e.key !== 'Backspace') return;

    const { selectionStart: pos, selectionEnd, value } = e.target;
    if (pos === selectionEnd && pos > 0 && separators.includes(value[pos - 1])) {
      e.preventDefault();
      e.target.value = value.slice(0, pos - 1) + value.slice(pos);
      e.target.setSelectionRange(pos - 1, pos - 1);
    }
  });
}

function validateIpMasks(ipInputs) {
  ipInputs.forEach((input) => {
    input.setAttribute('maxlength', '15');
    input.setAttribute('autocomplete', 'off');

    // Digits and dots only, max 4 octets, each 0-255 / 3 digits
    input.addEventListener('input', (e) => {
      e.target.value = e.target.value
        .replace(/[^0-9.]/g, '')
        .split('.')
        .slice(0, 4)
        .map((octet) => {
          const o = octet.slice(0, 3);
          return parseInt(o, 10) > 255 ? '255' : o;
        })
        .join('.');
    });

    eraseSeparatorOnBackspace(input, ['.']);
  });
}

// YYYY-MM-DD HH:mm:ss
const DATETIME_SEGMENTS = [
  { len: 4, sep: '' },
  { len: 2, sep: '-', min: 1, max: 12 },
  { len: 2, sep: '-', min: 1, max: 31 },
  { len: 2, sep: ' ', min: 0, max: 23 },
  { len: 2, sep: ':', min: 0, max: 59 },
  { len: 2, sep: ':', min: 0, max: 59 },
];

function formatDateTimeDigits(digits) {
  let pos = 0;
  let out = '';

  for (const { len, sep, min, max } of DATETIME_SEGMENTS) {
    const part = digits.slice(pos, pos + len);
    if (!part) break;

    // Clamp only completed segments that have a range
    const clamped = part.length === len && max !== undefined ? String(Math.min(Math.max(+part, min), max)).padStart(len, '0') : part;
    out += sep + clamped;
    pos += len;
  }

  return out;
}

function validateDateTimeMasks(datetimeInputs) {
  datetimeInputs.forEach((input) => {
    input.setAttribute('maxlength', '19');
    input.setAttribute('placeholder', input.getAttribute('placeholder') || 'YYYY-MM-DD HH:mm:ss');
    input.setAttribute('autocomplete', 'off');

    input.addEventListener('input', (e) => {
      e.target.value = formatDateTimeDigits(e.target.value.replace(/\D/g, ''));
    });

    eraseSeparatorOnBackspace(input, ['-', ' ', ':']);
  });
}


/* ========================================================================== */
/* API core                                                                   */
/* ========================================================================== */

/**
 * @typedef {Object} ApiResult
 * @property {boolean} ok
 * @property {number|null} status
 * @property {Headers|null} headers
 * @property {*} data
 * @property {'timeout'|'network'|null} error
 */

function resolveApiUrl(endpoint) {
  if (typeof endpoint !== 'string') {
    return String(endpoint);
  }
  if (/^https?:\/\//i.test(endpoint)) {
    return endpoint;
  }
  const origin = getDomainname();
  if (endpoint.startsWith(origin)) {
    return endpoint;
  }
  if (!endpoint.startsWith('/')) {
    return `${origin}/${endpoint}`;
  }
  return `${origin}${endpoint}`;
}

function classifyFetchError(error, signal) {
  if (error?.name === 'TimeoutError') return 'timeout'; // AbortSignal.timeout()

  if (signal?.aborted) {
    return signal.reason?.name === 'TimeoutError' ? 'timeout' : 'aborted';
  }

  return error?.name === 'AbortError' ? 'aborted' : 'network';
}

function plainTextSnippet(data) {
  if (typeof data !== 'string') {
    return '';
  }
  const trimmed = data.trim();
  if (!trimmed || trimmed.startsWith('<')) {
    return '';
  }
  return trimmed.length > 180 ? trimmed.slice(0, 180) + '…' : trimmed;
}

function launchNotifyEntry(entry, fallbackType, fallbackDuration) {
  if (!entry) {
    return;
  }
  if (typeof entry === 'string') {
    firework.launch(entry, fallbackType, fallbackDuration);
    return;
  }
  firework.launch(entry.msg, entry.type || fallbackType, entry.time || fallbackDuration);
}

/**
 * Thin fetch: device URL, credentials, cache no-store, 10s timeout.
 * Pass native RequestInit only. Use signal: null to disable the default timeout.
 * @param {string} endpoint
 * @param {RequestInit} [init]
 * @returns {Promise<Response>}
 */
async function fetchWrapper(endpoint, init = {}) {
  const { timeout = 10000, ...rest } = init;
  const options = { credentials: 'include', cache: 'no-store', ...rest };

  if (options.signal === undefined) {
    options.signal = AbortSignal.timeout(timeout);
  } else if (options.signal === null) {
    delete options.signal;
  } else {
    options.signal = AbortSignal.any([options.signal, AbortSignal.timeout(timeout)]);
  }
  return fetch(resolveApiUrl(endpoint), options);
}

/**
 * API call without toast message
 * Never throws. Reads the body even when !ok.
 * @param {string} endpoint
 * @param {RequestInit} [init]
 * @param {{ parseType?: 'text'|'json'|'blob' }} [extras]
 * @returns {Promise<ApiResult>}
 */
async function apiCall(endpoint, init = {}, extras = {}) {
  const parseType = extras.parseType || 'text';
  try {
    const response = await fetchWrapper(endpoint, init);
    if (parseType === 'blob') {
      const data = await response.blob();
      return { ok: response.ok, status: response.status, headers: response.headers, data, error: null };
    }

    const text = await response.text();
    if (parseType === 'json' && response.ok) {
      try {
        const data = text ? JSON.parse(text) : null;
        return { ok: true, status: response.status, headers: response.headers, data, error: null };
      } catch (error) {
        return { ok: false, status: response.status, headers: response.headers, data: error.message, error: null };
      }
    }

    return { ok: response.ok, status: response.status, headers: response.headers, data: text, error: null };
  } catch (error) {
    return { ok: false, status: null, headers: null, data: error.message, error: classifyFetchError(error, init.signal) };
  }
}

/**
 * API call with toast message for user-initiated actions
 * @param {string} endpoint
 * @param {RequestInit} [init]
 * @param {{ action?: string, resultMsg?: string|Object, loadingMsg?: string, parseType?: 'text'|'json'|'blob', httpObj?: Object }} [notify]
 * @returns {Promise<ApiResult>}
 */
async function apiCallNotify(endpoint, init = {}, notify = {}) {
  const { action = 'Request', resultMsg, loadingMsg, parseType, httpObj } = notify;
  const loadingId = loadingMsg ? firework.launch(loadingMsg, 'success', 120000) : null;
  const res = await apiCall(endpoint, init, { parseType });
  firework.dismiss(loadingId);

  if (res.error === 'aborted') // manual abort, do nothing
    return res;

  if (res.error === 'timeout') {
    firework.launch(`${action} timed out`, 'danger', 30000);
    console.error(`${action} timed out`, res.error, res.data);
    return res;
  }

  if (res.error === 'network') {
    firework.launch(`${action} failed. Server closed the connection abruptly`, 'danger', 30000);
    console.error(`${action} failed`, res.error, res.data);
    return res;
  }

  if (!res.ok) {
    if (httpObj && httpObj[res.status]) {
      const entry = httpObj[res.status];
      launchNotifyEntry(typeof entry === 'function' ? entry(res) : entry, 'danger', 30000);
    } else if (res.status === 403) {
      firework.launch(`${action} rejected. Process not (yet) initialized. Repeat action or check logs.`, 'warning', 5000);
    } else {
      const snippet = plainTextSnippet(res.data);
      let msg = `${action} failed (Status: ${res.status})`;
      if (snippet) {
        msg += ` | ${snippet}`;
      }
      firework.launch(msg, 'danger', 30000);
    }
    console.error(`${action} rejected`, res.status, res.data);
    return res;
  }

  if (typeof resultMsg === 'string') {
    firework.launch(resultMsg, 'success', 3000);
  } else if (resultMsg && typeof resultMsg === 'object') {
    const code = typeof res.data === 'string' ? res.data.substring(0, 3) : '';
    if (resultMsg[code]) {
      launchNotifyEntry(resultMsg[code], 'success', 3000);
    } else {
      firework.launch(`${action} rejected. Reason: ${res.data || 'Unknown'}`, 'danger', 30000);
    }
  }

  return res;
}


/* ========================================================================== */
/* API calls                                                                  */
/* ========================================================================== */

const withDomain = (domainname, endpoint) => (domainname ? domainname + endpoint : endpoint);
const apiError = (message, res) => Object.assign(new Error(message), { status: res.status, error: res.error });

// GET + parse JSON. Toasts and throws on failure
async function fetchJson(endpoint, action) {
  const res = await apiCallNotify(endpoint, {}, { action, parseType: 'json' });
  if (!res.ok) throw new Error(`${action} failed`);
  return res.data;
}

const getWlanNetworks = () => fetchJson('/wlan?task=scan', 'WLAN scan request');
const getLogFileList = () => fetchJson('/log?type=filelist', 'Log files request');
const getDataFileList = () => fetchJson('/data?type=filelist', 'Data files request');
const getTfliteFileList = () => fetchJson('/fileserver/config/models/?filelist=true', 'TFLite files request');
const getCertFileList = () => fetchJson('/fileserver/config/certs/?filelist=true', 'Cert files request');

async function apiCopy(_source, _target, _domainname = '', async = false) {
  const endpoint = withDomain(_domainname, `/editflow?task=copy&in=${_source}&out=${_target}`);
  const res = await apiCallNotify(endpoint, { signal: async ? undefined : null }, { action: 'Copy file request' });
  if (!res.ok) throw apiError('Copy file request failed', res);
  return 'Copy file request successful';
}

async function apiDelete(_filename, _domainname = '') {
  const res = await apiCallNotify(withDomain(_domainname, '/delete' + _filename), { method: 'POST' }, { action: 'File delete request' });
  return res.ok;
}

async function apiUpload(_content, _filename, _domainname = '') {
  const res = await apiCallNotify(withDomain(_domainname, '/upload' + _filename), { method: 'POST', body: _content }, { action: 'File send request' });
  return res.ok;
}

/* ========================================================================== */
/* Config                                                                     */
/* ========================================================================== */

let jsonConfig = {}; // Config: Object in JSON notation
let jsonConfigModifiedDelta = {}; // Config: Object in JSON notation (modified parameter only)

// Reload code -> notify entry
const APPLIED = { msg: 'Configuration saved and applied successfully', type: 'success', time: 3000 };
const DEFERRED = { msg: 'Configuration saved. Will apply after current cycle', type: 'success', time: 3000 };
const NOT_APPLIED = { msg: 'Configuration saved, but failed to apply: Process uninitialized', type: 'danger', time: 10000 };
const RELOAD_NOTIFY = { '001': APPLIED, '002': APPLIED, '003': APPLIED, '004': DEFERRED, '099': NOT_APPLIED, E90: NOT_APPLIED };

function processReloadFeedback(code) {
  launchNotifyEntry(RELOAD_NOTIFY[code], 'success', 3000);
}

async function reloadConfig() {
  await apiCallNotify('/config?task=reload', {}, { action: 'Configuration update', resultMsg: RELOAD_NOTIFY });
}

async function saveConfig(data, reload = false) {
  const res = await apiCallNotify(
    reload ? '/config?reload=true' : '/config',
    { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: data, timeout: 15000 },
    {
      action: 'Save config',
      loadingMsg: 'Save and apply configuration...',
      resultMsg: reload ? undefined : 'Configuration saved successfully',
    },
  );

  if (!res.ok) throw apiError(`Save config failed (${res.error ?? `Status: ${res.status}`})`, res);

  sessionStorage.setItem('jsonConfigData', res.data);
  if (reload) processReloadFeedback(res.headers.get('X-Reload-Code'));
}

async function loadConfig() {
  const res = await apiCallNotify('/config', { timeout: 15000 }, { action: 'Loading config', parseType: 'json' });
  if (!res.ok || !res.data) throw apiError(`Loading config failed (${res.error ?? `Status: ${res.status}`})`, res);

  try {
    sessionStorage.setItem('jsonConfigData', JSON.stringify(res.data));
  } catch (e) {
    console.warn('Config cache not updated:', e);
  }
}

// type: 0 = config, 1 = keyified [path, value] pairs, 2 = [config, keyified]
function getConfigFromStorage(type = 0) {
  jsonConfig = JSON.parse(sessionStorage.getItem('jsonConfigData'));
  if (type === 0) return jsonConfig;

  const keyified = jsonObjectKeyify(jsonConfig);
  return type === 1 ? keyified : type === 2 ? [jsonConfig, keyified] : undefined;
}

function noConfigInStorage() {
  return sessionStorage.getItem('jsonConfigData') === null;
}

// { a: { b: 1 } } -> [['a_b', 1]]
function jsonObjectKeyify(obj) {
  const flatten = (v, path = []) => (Object(v) === v ? Object.entries(v).flatMap(([k, child]) => flatten(child, [...path, k])) : [[path.join('_'), v]]);
  return flatten(obj);
}

function deepMergeObjects(obj1, obj2) {
  const result = {};

  for (const key of Object.keys(obj2)) {
    result[key] = isObj(obj2[key]) && isObj(obj1[key]) ? deepMergeObjects(obj1[key], obj2[key]) : obj2[key];
  }
  for (const key of Object.keys(obj1)) {
    if (!(key in result)) result[key] = isObj(obj1[key]) ? deepMergeObjects(obj1[key], {}) : obj1[key];
  }

  return result;
}

// Based on https://stackoverflow.com/a/54733755/5459839
function deepSet(obj, path, value, integer) {
  if (Object(obj) !== obj) return obj;

  const keys = Array.isArray(path) ? [...path] : path.toString().match(/[^.[\]]+/g) || [];
  const last = keys.pop();

  const target = keys.reduce((acc, key, i) => {
    if (Object(acc[key]) !== acc[key]) {
      // next key is an array index -> create array, else object
      const next = keys[i + 1] ?? last;
      acc[key] = Math.abs(next) >> 0 === +next ? [] : {};
    }
    return acc[key];
  }, obj);

  target[last] = integer ? parseInt(value) : value === 'true' ? true : value === 'false' ? false : value;
  return obj;
}

function gatherChangedParameter(form, integer = false) {
  const data = {};
  for (const [path, value] of new FormData(form)) {
    deepSet(data, path, value, integer);
  }
  jsonConfigModifiedDelta = deepMergeObjects(jsonConfigModifiedDelta, data);
}

function parseCurrentParametrization(paramsKeyified) {
  for (const [key, value] of paramsKeyified) {
    const el = document.getElementById(`${key}_value`.toLowerCase());
    if (!el) continue;

    if (el.tagName === 'SELECT') {
      const valueLowerCase = String(value).toLowerCase();
      const idx = Array.from(el.options).findIndex((o) => o.value.toLowerCase() === valueLowerCase);
      if (idx >= 0) el.selectedIndex = idx;
    } else {
      el.value = value;
    }
  }
}


/* ========================================================================== */
/* Images                                                                     */
/* ========================================================================== */
const loaderSpinner = `data:image/svg+xml;base64,${btoa(`
    <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none"
        stroke="#007bff" stroke-width="1" stroke-linecap="round">
        <path d="M12 3a9 9 0 1 1-6.36 2.64">
            <animateTransform attributeName="transform" type="rotate" from="0 12 12"
                to="360 12 12" dur="0.8s" repeatCount="indefinite" />
        </path>
    </svg>
`)}`;

const noImageAvailable = `data:image/svg+xml;utf8,${encodeURIComponent(`
  <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 400 300">
    <rect width="400" height="300" fill="#f0f0f0" rx="10"/>
    <g stroke="#999" stroke-width="6" fill="none" stroke-linecap="round" stroke-linejoin="round">
      <rect x="150" y="80" width="100" height="80" rx="4" />
      <path d="M150 140 L180 110 L210 140 L230 120 L250 150" />
      <circle cx="220" cy="105" r="5" fill="#999" stroke="none" />
    </g>
    <text x="200" y="220" font-family="sans-serif" font-size="22" fill="#666" text-anchor="middle" font-weight="bold">
      No Image Available
    </text>
  </svg>
`)}`;

// Loads url; falls back to altUrl on error/abort/10s timeout
function loadImage(url, altUrl) {
  const img = new Image();
  let timer;

  const fallback = () => {
    clearTimeout(timer);
    img.onload = img.onabort = img.onerror = null;
    img.src = altUrl;
  };

  img.onerror = img.onabort = fallback;
  img.onload = () => clearTimeout(timer);
  img.src = url;
  timer = setTimeout(fallback, 10000);
  return img;
}

async function saveCanvasToImage(_canvas, _filename, _delete = true, _domainname = '') {
  const blob = await new Promise((resolve) => _canvas.toBlob(resolve, 'image/jpeg', 0.8));
  if (!blob) return false;

  if (_delete) {
    await apiCall(withDomain(_domainname, '/delete' + _filename), { method: 'POST' });
  }

  return apiUpload(blob, _filename, _domainname);
}
