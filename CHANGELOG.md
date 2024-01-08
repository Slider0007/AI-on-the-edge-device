# Changelog

## [16.1.1-SLFork](https://github.com/Slider0007/AI-on-the-edge-device/compare/v16.1.0-SLFork...v16.1.1-SLFork) (2024-01-08)


### Bug Fixes

* **gpio:** Fix init issue & wrong mode comparison ([#115](https://github.com/Slider0007/AI-on-the-edge-device/issues/115)) ([8da5982](https://github.com/Slider0007/AI-on-the-edge-device/commit/8da5982de5566f50355c6ad69dd9d5db83603e89))


### Other Changes

* **unity-test:** Fix compiler issue & code cleanup ([#118](https://github.com/Slider0007/AI-on-the-edge-device/issues/118)) ([9862696](https://github.com/Slider0007/AI-on-the-edge-device/commit/98626966a8973d20fdb3d06c3fd0aa7d8b429f00))

## [16.1.0-SLFork](https://github.com/Slider0007/AI-on-the-edge-device/compare/v16.0.2-SLFork...v16.1.0-SLFork) (2024-01-05)


### Features

* **influxdb1+2:** Implement TLS encryption using certificates ([#105](https://github.com/Slider0007/AI-on-the-edge-device/issues/105)) ([4364427](https://github.com/Slider0007/AI-on-the-edge-device/commit/436442738c0d8b987130edfabd986b14c062ede6))
* Make camera frequency adjustable ([#79](https://github.com/Slider0007/AI-on-the-edge-device/issues/79)) ([47f19c8](https://github.com/Slider0007/AI-on-the-edge-device/commit/47f19c887cdce38ef68872cac0a69baa0be3a599))
* **mqtt:** Implement MQTT TLS encryption using certificates ([#102](https://github.com/Slider0007/AI-on-the-edge-device/issues/102)) ([7d53c1e](https://github.com/Slider0007/AI-on-the-edge-device/commit/7d53c1e5ad9f97aea834545af6cdab18d7de2c4f))
* **webui:** 'Overview'+'Data Graph': Save auto page refresh config persistently ([#113](https://github.com/Slider0007/AI-on-the-edge-device/issues/113)) ([6a36d5a](https://github.com/Slider0007/AI-on-the-edge-device/commit/6a36d5a6e18e14f684763af1bdc1794821a25658))
* **webui:** Automatic ROI naming + ROI/Alignment marker position validation ([#99](https://github.com/Slider0007/AI-on-the-edge-device/issues/99)) ([cd5b979](https://github.com/Slider0007/AI-on-the-edge-device/commit/cd5b979e8c76c3a1abe76bdf68200453285a30cd))


### Bug Fixes

* **alignment marker:** Fix usage of preallocated RAM ([#95](https://github.com/Slider0007/AI-on-the-edge-device/issues/95)) ([5676d59](https://github.com/Slider0007/AI-on-the-edge-device/commit/5676d591dee9c2c53a918f829d2cbc059a25c9e7))
* Fix 'MaxRateType' parameter migration ([#91](https://github.com/Slider0007/AI-on-the-edge-device/issues/91)) ([5926bf8](https://github.com/Slider0007/AI-on-the-edge-device/commit/5926bf890668136f465ad594735942dd77ffc336))
* **influxdb:** Consider DST for UTC time converison ([#89](https://github.com/Slider0007/AI-on-the-edge-device/issues/89)) ([d1f77ac](https://github.com/Slider0007/AI-on-the-edge-device/commit/d1f77acb540c679132e176bd020f429855ca723c))
* **influxdbv2:** Rename parameter 'database' to 'bucket' ([#100](https://github.com/Slider0007/AI-on-the-edge-device/issues/100)) ([b2f9e11](https://github.com/Slider0007/AI-on-the-edge-device/commit/b2f9e1134f1fe1b8331a055bbeaa1679cf94b64e))
* **parameter migration:** Fix 'MaxRateType' parameter (Handling 'RateOff') ([#97](https://github.com/Slider0007/AI-on-the-edge-device/issues/97)) ([6901c01](https://github.com/Slider0007/AI-on-the-edge-device/commit/6901c0164f5aa793ac7aea78087b1cd011596198))
* **REST API:** Fix misleading REST API send_file error ([#107](https://github.com/Slider0007/AI-on-the-edge-device/issues/107)) ([5112ce4](https://github.com/Slider0007/AI-on-the-edge-device/commit/5112ce487c97c5fc8298aecc5c363f2f350cc848))
* **REST API:** Fix rare exception for log file handling ([#112](https://github.com/Slider0007/AI-on-the-edge-device/issues/112)) ([21e61d1](https://github.com/Slider0007/AI-on-the-edge-device/commit/21e61d136f49cd5230c7cfed773c74857438a45a))
* **webui:** Increase file handling robustness -&gt; handle config.ini update in firmware ([#90](https://github.com/Slider0007/AI-on-the-edge-device/issues/90)) ([c348cb3](https://github.com/Slider0007/AI-on-the-edge-device/commit/c348cb34502462f8cece141a23f9d77954e3942e))
* **webui:** Make REST API calls more robust ([#109](https://github.com/Slider0007/AI-on-the-edge-device/issues/109)) ([2909471](https://github.com/Slider0007/AI-on-the-edge-device/commit/2909471294558a4053bfcf5096e5674fe45a5bbe))
* **wlan.ini:** ChangeRSSIThreshold: Fix logging issue ([#111](https://github.com/Slider0007/AI-on-the-edge-device/issues/111)) ([4c75e4e](https://github.com/Slider0007/AI-on-the-edge-device/commit/4c75e4ec4dad2b4a291698d74283c42d27188d36))
* **wlan:** Fix deletion of array object ([#110](https://github.com/Slider0007/AI-on-the-edge-device/issues/110)) ([2f0934e](https://github.com/Slider0007/AI-on-the-edge-device/commit/2f0934ea6a566228fcc50056ab4eeb25b1f16aeb))


### Refactoring / Style Changes

* **helper:** Move functions to system_info ([#82](https://github.com/Slider0007/AI-on-the-edge-device/issues/82)) ([40f95f8](https://github.com/Slider0007/AI-on-the-edge-device/commit/40f95f8f160f9935f1cc4cf9d31c7c5b3b0af309))
* Relocate setCPUFrequency ([#88](https://github.com/Slider0007/AI-on-the-edge-device/issues/88)) ([d95c677](https://github.com/Slider0007/AI-on-the-edge-device/commit/d95c677ac78ebc4ddaca8e2b30b3cd6a74930807))
* Rename 'round' to 'cycle' ([#84](https://github.com/Slider0007/AI-on-the-edge-device/issues/84)) ([ce61169](https://github.com/Slider0007/AI-on-the-edge-device/commit/ce611697e8f6260ecd1ff68f78f72a14776011a5))
* **REST API:** Relocate REST API functions ([#83](https://github.com/Slider0007/AI-on-the-edge-device/issues/83)) ([285d8d7](https://github.com/Slider0007/AI-on-the-edge-device/commit/285d8d73e9396438ee2a082d387b02b5c53e2a41))
* **webserver:** Adapt/Align some debug log level ([#98](https://github.com/Slider0007/AI-on-the-edge-device/issues/98)) ([7e1ee91](https://github.com/Slider0007/AI-on-the-edge-device/commit/7e1ee91b6e900b00178c9570fb0b3435716a52ce))
* **webui:** Optimized mobile presenation ([#96](https://github.com/Slider0007/AI-on-the-edge-device/issues/96)) ([8cd041b](https://github.com/Slider0007/AI-on-the-edge-device/commit/8cd041b2461997652ce031537abc97a41456db97))
* **webui:** Overlay notification box e.g. after user interaction ([#108](https://github.com/Slider0007/AI-on-the-edge-device/issues/108)) ([0902669](https://github.com/Slider0007/AI-on-the-edge-device/commit/09026696210c1c089303daeacc8bece9c9a3e558))


### Other Changes

* **build:** Create release-please PR as draft ([#87](https://github.com/Slider0007/AI-on-the-edge-device/issues/87)) ([b0e039c](https://github.com/Slider0007/AI-on-the-edge-device/commit/b0e039ce49a72aafe91607dd0c4319b7de273862))
* **build:** Ensure GIT_TAG is never empty ([#86](https://github.com/Slider0007/AI-on-the-edge-device/issues/86)) ([be07db0](https://github.com/Slider0007/AI-on-the-edge-device/commit/be07db066ca28adc38a55d6e5942a31db2521a32))
* **build:** Fix git tag in device log ([#92](https://github.com/Slider0007/AI-on-the-edge-device/issues/92)) ([fb83eb0](https://github.com/Slider0007/AI-on-the-edge-device/commit/fb83eb07ea6442496b44080df6b83186aa4fa60c))
* **build:** Remove folder creation during build ([#93](https://github.com/Slider0007/AI-on-the-edge-device/issues/93)) ([2d3a3ec](https://github.com/Slider0007/AI-on-the-edge-device/commit/2d3a3ec175c067260644cb7779fbdade42861539))
* **dependency:** Update esp-tflite-micro, esp-nn, esp32-camera, stb, miniz ([#94](https://github.com/Slider0007/AI-on-the-edge-device/issues/94)) ([7949c5d](https://github.com/Slider0007/AI-on-the-edge-device/commit/7949c5dbb8555b91ba2af4ccf67743fa9bba6d9e))
* Modify test environment detection ([#101](https://github.com/Slider0007/AI-on-the-edge-device/issues/101)) ([2509ffe](https://github.com/Slider0007/AI-on-the-edge-device/commit/2509ffe10f235d1a38124c28ed1595be0102a597))
* **repo:** Update readme ([#103](https://github.com/Slider0007/AI-on-the-edge-device/issues/103)) ([16697ad](https://github.com/Slider0007/AI-on-the-edge-device/commit/16697adb3273d09814e586d406db900010a313db))
* **tflite:** Updated digit/analog models ([#114](https://github.com/Slider0007/AI-on-the-edge-device/issues/114)) ([9671208](https://github.com/Slider0007/AI-on-the-edge-device/commit/9671208dfa6af7ad6a836595b0b561a51b67a0b8))
* **unity-test:** Fix some test cases ([#104](https://github.com/Slider0007/AI-on-the-edge-device/issues/104)) ([a83348a](https://github.com/Slider0007/AI-on-the-edge-device/commit/a83348a1fd19c092af3c293888c1ed5aa87124f5))
* Update static part of firmware version string ([#80](https://github.com/Slider0007/AI-on-the-edge-device/issues/80)) ([c283977](https://github.com/Slider0007/AI-on-the-edge-device/commit/c283977beb5aab74f8240564d92ded2eeb6365d2))

## [16.0.2-SLFork](https://github.com/Slider0007/AI-on-the-edge-device/compare/v16.0.1-SLFork...v16.0.2-SLFork) (2023-10-18)


### Other Changes

* **build:** Fix build workflow (commit hash) ([#77](https://github.com/Slider0007/AI-on-the-edge-device/issues/77)) ([d7f9991](https://github.com/Slider0007/AI-on-the-edge-device/commit/d7f9991f763c202e63672dcdb6f651d1973bad4a))

## [16.0.1-SLFork](https://github.com/Slider0007/AI-on-the-edge-device/compare/v16.0.0-SLFork...v16.0.1-SLFork) (2023-10-18)


### Other Changes

* **build:** Fix build workflow (correct artifacts & release version string) ([#75](https://github.com/Slider0007/AI-on-the-edge-device/issues/75)) ([e8c8c00](https://github.com/Slider0007/AI-on-the-edge-device/commit/e8c8c00e6bc362e24ece924b51a4b6025f93d9d6))

## [16.0.0-SLFork](https://github.com/Slider0007/AI-on-the-edge-device/compare/v15.1.1...v16.0.0-SLFork) (2023-10-13)

The following changes are implemented on the the base of jomjol's v15.1.1 release.
--> https://github.com/jomjol/AI-on-the-edge-device/tree/2a7f3b33a30985b43a8db123a1f5f6e5f264f486


### ⚠ BREAKING CHANGES

* Changed MQTT topic names
* Renamed REST API /value option: error -> status
* Adapted data log structure (value status only status number)
* Rename of `preValue` name variants to `fallbackValue` in code and WebUI
* Rework result post-processing and publishing (REST, MQTT, InfluxDB)
* Rework ROI CNN result processing and related functions
* Remove legacy REST APIs
* Update REST API handler_value + Refactor WebUI recognition page
* Save prevalue (fallbackvalue) to NVS instead of file
* Fully reworked main flow state machine (new state names)
* Reload configuration without device reboot

### Features

* Add a post process event handler to perform error/debug handling) ([932d538](https://github.com/Slider0007/AI-on-the-edge-device/commit/932d538d7f0aca67268832d379f7cf987b482868))
* Dedicated state names for digit/analog processing (inspired by caco3) ([47b37eb](https://github.com/Slider0007/AI-on-the-edge-device/commit/47b37eb31e52280f158a55bc9ac450cba716a2d6))
* Enhance alignment algo + Add error handling ([de17472](https://github.com/Slider0007/AI-on-the-edge-device/commit/de174727ced04461fcac849a39b4302a0f9ddc8a))
* **fileserver:** Enhanced file deletion routine (cherry picked, implemented by caco3) ([783b7ea](https://github.com/Slider0007/AI-on-the-edge-device/commit/783b7eaba6fcbe7c6cf65419039785cd5a34f975))
* Extend InfluxDBv1 with individual topic names (implemented by jomjol) (#2319)
* Fully reworked main flow state machine (new state names) ([751e42d](https://github.com/Slider0007/AI-on-the-edge-device/commit/751e42d083ddc6089e3c4320d5069ab85e457cd2))
* Implement a camera livestream handler (#2286)
* Keep alignment marker in RAM to reduce SD read cycles ([3f68cc4](https://github.com/Slider0007/AI-on-the-edge-device/commit/3f68cc40f0d6bd2537796701fb9d2f799c68e0c6))
* Keep tflite models file loaded after cycle init ([6a3d1df](https://github.com/Slider0007/AI-on-the-edge-device/commit/6a3d1df0bfac1cb0d222c4c861a503625587256c))
* Move bss segment to SPIRAM -&gt; more free internal RAM ([7c7a63c](https://github.com/Slider0007/AI-on-the-edge-device/commit/7c7a63c9245e0f324628d65677468d836c6ec22a))
* Provide cycle process error -&gt; MQTT / REST API ([fe6231e](https://github.com/Slider0007/AI-on-the-edge-device/commit/fe6231eb9f8bdc67def2af63ddeeb79e61894c48))
* Reload configuration without device reboot ([751e42d](https://github.com/Slider0007/AI-on-the-edge-device/commit/751e42d083ddc6089e3c4320d5069ab85e457cd2))
* Remove legacy REST APIs ([020d9ec](https://github.com/Slider0007/AI-on-the-edge-device/commit/020d9ec32d3cf321e94ff9252cf558e7dd76ec6f))
* Rework result post-processing and publishing (REST, MQTT, InfluxDB) ([d07b86f](https://github.com/Slider0007/AI-on-the-edge-device/commit/d07b86fa76a07b6ee278a3ea0a7bebc2cdaa3c48))
* Rework ROI CNN result processing and related functions ([bd6ab71](https://github.com/Slider0007/AI-on-the-edge-device/commit/bd6ab71113d48b9401b2a82f890391ea555e7a17))
* ROI images not saving to sd card by default to reduce write cycles ([be023b4](https://github.com/Slider0007/AI-on-the-edge-device/commit/be023b47cebbfc04aa1ecd52a1ba9b2ae41d29ed))
* Save prevalue (fallbackvalue) to NVS instead of file ([a39bc52](https://github.com/Slider0007/AI-on-the-edge-device/commit/a39bc5226023ffa79e5c6620b15929665ded5c23))
* Update REST API handler_value + Refactor WebUI recognition page ([020d9ec](https://github.com/Slider0007/AI-on-the-edge-device/commit/020d9ec32d3cf321e94ff9252cf558e7dd76ec6f))
* Set prevalue using MQTT + set prevalue to RAW value (REST+MQTT) (#2252)
* **webui:** Add initial rotate to WebUI config page ([5767e93](https://github.com/Slider0007/AI-on-the-edge-device/commit/5767e93ab77bd92514af135dd39ed208bd869558))
* **webui:** Alignment: Add option to switch off rotation ([da2e900](https://github.com/Slider0007/AI-on-the-edge-device/commit/da2e900670596acde8d7d9b274b21047f1785804))
* **webui:** Allow firmware.bin as valid file name ([932d538](https://github.com/Slider0007/AI-on-the-edge-device/commit/932d538d7f0aca67268832d379f7cf987b482868))
* **webui:** Config page: Remove unnecessary checkboxes ([d93eeb5](https://github.com/Slider0007/AI-on-the-edge-device/commit/d93eeb5ad590fabdd398d9e94ac6314a4c963f52))
* **webui:** Data Graph: Add Raw value + refactor ([d07b86f](https://github.com/Slider0007/AI-on-the-edge-device/commit/d07b86fa76a07b6ee278a3ea0a7bebc2cdaa3c48))
* **webui:** Enahnce WebUI using new REST API /process_data and some further chnages ([85ceeba](https://github.com/Slider0007/AI-on-the-edge-device/commit/85ceeba408e000f404900fe454e14931e442024b))
* **webui:** Implement auto page refresh function for 'Overview' and 'Data Graph' ([3ab1eda](https://github.com/Slider0007/AI-on-the-edge-device/commit/3ab1eda2ad9ad8a24145a8568357c00fde1e4aea))
* **webui:** Improve handling of missing parameter ([935f930](https://github.com/Slider0007/AI-on-the-edge-device/commit/935f930ee45fb1818dd4ba85419462505f368860))
* **webui:** Overview.html: Rename value status result to valid / invalid ([12aae5f](https://github.com/Slider0007/AI-on-the-edge-device/commit/12aae5f7c845b02f022d7e78b26948d1a04f746d))
* **webui:** Updated WebUI to match new flow states ([751e42d](https://github.com/Slider0007/AI-on-the-edge-device/commit/751e42d083ddc6089e3c4320d5069ab85e457cd2))


### Bug Fixes

* Delete TFLiteClass only when ClassFlowCNN gets deleted ([f4f2d8b](https://github.com/Slider0007/AI-on-the-edge-device/commit/f4f2d8b305ae77a9b2f34c168520180109e479a3))
* Digit no zero crossing issue ([#74](https://github.com/Slider0007/AI-on-the-edge-device/issues/74)) ([9c1a35d](https://github.com/Slider0007/AI-on-the-edge-device/commit/9c1a35da36c6c2168e5135d3f705797c0503143e))
* **fileserver:** avoid sending *two* "last-chunk" sequences ([#2532](https://github.com/Slider0007/AI-on-the-edge-device/issues/2532)) ([#53](https://github.com/Slider0007/AI-on-the-edge-device/issues/53)) ([db467c0](https://github.com/Slider0007/AI-on-the-edge-device/commit/db467c0a034a1e9acbe3d02cb892eec15c4272ba))
* Fix first digit when extended resolution off ([#2466](https://github.com/Slider0007/AI-on-the-edge-device/issues/2466)) ([733e58d](https://github.com/Slider0007/AI-on-the-edge-device/commit/733e58d075fd8d3223e33c5237bb4d0070eee0c2))
* Fix last element missing in digit model drop down (#2282)
* Fix leading NaN (#2310)
* Fix broken sysinfo (implemented by caco3) (#2381)
* **mqtt:** mqtt_handler_set_prevalue: fix memory leak ([0b99624](https://github.com/Slider0007/AI-on-the-edge-device/commit/0b99624d9ecb45269f806fc1242faea194a2bcba))
* Update SmartLeds driver (avoid build warnings) ([b641a92](https://github.com/Slider0007/AI-on-the-edge-device/commit/b641a92d6b52911409cfa9a44db78007eaf272bc))
* **webui:** Analog ROI: Fix wrong multiplier view ([575f504](https://github.com/Slider0007/AI-on-the-edge-device/commit/575f504a0298ab81f864b9632733f29bf1303710))
* **webui:** Aspect ratio for analog ROIs incorrect ([1bb3470](https://github.com/Slider0007/AI-on-the-edge-device/commit/1bb347097055cf428da614588bd115ecd180637b))
* **webui:** Config: Remove unused parameter `ErrorMessage` in code and WebUI ([d07b86f](https://github.com/Slider0007/AI-on-the-edge-device/commit/d07b86fa76a07b6ee278a3ea0a7bebc2cdaa3c48))
* **webui:** Fix missing first entry of logfile + datafile in default viewer ([583a24f](https://github.com/Slider0007/AI-on-the-edge-device/commit/583a24ff3ca4ee81c304f76977861bd8f18b665d))
* **webui:** Remove icon of parameter tooltip admonitions ([7c9a1bf](https://github.com/Slider0007/AI-on-the-edge-device/commit/7c9a1bf4e19637319c865030749b23313ed3fa36))
* **webui:** Remove unused gethost.js ([d4e832b](https://github.com/Slider0007/AI-on-the-edge-device/commit/d4e832b7a12c6ec010bdb01a178c236d6987ca36))
* **webui:** Show info message if no recent log / data entries available ([f22be0c](https://github.com/Slider0007/AI-on-the-edge-device/commit/f22be0ce4d588202eaf8566212fca27fbae2d13f))
* **wifi:** Allow operation with empty WIFI password ([bd07d7a](https://github.com/Slider0007/AI-on-the-edge-device/commit/bd07d7a7e805a192cb9d1ca5cf47ccf335a7e61a))


### Refactor / Style Changes

* Adapted data log structure (value status only status number) ([d07b86f](https://github.com/Slider0007/AI-on-the-edge-device/commit/d07b86fa76a07b6ee278a3ea0a7bebc2cdaa3c48))
* Changed MQTT topic names ([d07b86f](https://github.com/Slider0007/AI-on-the-edge-device/commit/d07b86fa76a07b6ee278a3ea0a7bebc2cdaa3c48))
* Harmonize / update some logs ([7d8d646](https://github.com/Slider0007/AI-on-the-edge-device/commit/7d8d6463911077e5ca1c93e121b1cf8aa3762487))
* Harmonize time usage ([3505044](https://github.com/Slider0007/AI-on-the-edge-device/commit/3505044a8eda8262e2530971aab65c6dd51aaf52))
* **influxdb:** Refactor InfluxDBv1+v2 functions ([4f59958](https://github.com/Slider0007/AI-on-the-edge-device/commit/4f599584701f32b8d5186eb7aab67b8fba6833b7))
* Move function 'PowerResetCamera' to 'ClassControllCamera' ([69fb93b](https://github.com/Slider0007/AI-on-the-edge-device/commit/69fb93b993ff69490bc760b70e8c4e27089da5d9))
* Refactor camera init during boot ([932d538](https://github.com/Slider0007/AI-on-the-edge-device/commit/932d538d7f0aca67268832d379f7cf987b482868))
* Refactor ClassControllCamera ([751e42d](https://github.com/Slider0007/AI-on-the-edge-device/commit/751e42d083ddc6089e3c4320d5069ab85e457cd2))
* Refactor ClassTakeImage ([751e42d](https://github.com/Slider0007/AI-on-the-edge-device/commit/751e42d083ddc6089e3c4320d5069ab85e457cd2))
* Refactor demo mode ([932d538](https://github.com/Slider0007/AI-on-the-edge-device/commit/932d538d7f0aca67268832d379f7cf987b482868))
* Refactor file and folder retention functions ([932d538](https://github.com/Slider0007/AI-on-the-edge-device/commit/932d538d7f0aca67268832d379f7cf987b482868))
* Refactor string usage ([2632887](https://github.com/Slider0007/AI-on-the-edge-device/commit/2632887fb94f84062f23799a99e7b7cfbc05263f))
* Rename of `preValue` name variants to `fallbackValue` in code and WebUI ([d07b86f](https://github.com/Slider0007/AI-on-the-edge-device/commit/d07b86fa76a07b6ee278a3ea0a7bebc2cdaa3c48))
* Renamed REST API /value option: error -&gt; status ([d07b86f](https://github.com/Slider0007/AI-on-the-edge-device/commit/d07b86fa76a07b6ee278a3ea0a7bebc2cdaa3c48))
* Renaming & cleanup of some modules / functions in source code (#2265)
* **sdkconfig:** Adapt structure and headline namings to sdkconfig.esp32cam ([7922855](https://github.com/Slider0007/AI-on-the-edge-device/commit/79228559a38ad9069651cae04117e1aa1181549a))
* **webui:** Adapt some parameter name & visibility (regular / expert) ([597373c](https://github.com/Slider0007/AI-on-the-edge-device/commit/597373ce3a3d4d24510d98546c8ce9a839ecf9b1))
* **webui:** Overview: Show round counter in separate line ([d07b86f](https://github.com/Slider0007/AI-on-the-edge-device/commit/d07b86fa76a07b6ee278a3ea0a7bebc2cdaa3c48))
* **webui:** Parameter tooltips: Update style + support tables ([0f5ca2f](https://github.com/Slider0007/AI-on-the-edge-device/commit/0f5ca2ffccd03c108bf3f26ba7cf6af5be8fa38c))
* **webui:** Show round counter in separate line ([932d538](https://github.com/Slider0007/AI-on-the-edge-device/commit/932d538d7f0aca67268832d379f7cf987b482868))
* **wifi:** Refactor wifi init ([a707db3](https://github.com/Slider0007/AI-on-the-edge-device/commit/a707db35c2e2cd69bfb267a3678d2c0ba5c20ad6))


### Other Changes

* **build:** Add debug files artifact + General update ([0e79a93](https://github.com/Slider0007/AI-on-the-edge-device/commit/0e79a932990b05a828b4aca95cd43d49281c100d))
* **build:** Add platformIO env to local build firmware + parameter tooltips ([f59cd63](https://github.com/Slider0007/AI-on-the-edge-device/commit/f59cd63fa27e23089fae28d8950fa1c7fee1af46))
* **build:** Optimize caching ([027b89e](https://github.com/Slider0007/AI-on-the-edge-device/commit/027b89e0bcc53d3ff85083442bcba9c773af7bf6))
* **build:** Rework build workflow (add release-please action) ([113a7f3](https://github.com/Slider0007/AI-on-the-edge-device/commit/113a7f38abeaa9fb91cf05560671bcac414be303))
* **platformio:** Migration of PlatformIO 5.2.0 to 6.1.0 (resp. ESP IDF from 4.4.2 to 5.0.1) (#2305)
* **platformio:** Update platformIO to 6.3.2 ([7c0b2e5](https://github.com/Slider0007/AI-on-the-edge-device/commit/7c0b2e581d359776fa3cfaaf9f3eda446ec30d95))
* Remove miniz examples ([1ad5f69](https://github.com/Slider0007/AI-on-the-edge-device/commit/1ad5f6930152dcdd95b67e36bb0a98c5883c654e))
* Removed the stb_image files and re-add them as a submodule. (#2223)
* Remove obsolete ClassFlowWriteList (#2264)
* Remove redundant 'getFileSize()' function ([55ac7c1](https://github.com/Slider0007/AI-on-the-edge-device/commit/55ac7c1fb000adcbcb60b0f6baf504a4b2e60e4e))
* Remove webupdater
* **sdcard:** Buffered SD card R/W: Increase buffer from 128 to 512 byte ([4141e4d](https://github.com/Slider0007/AI-on-the-edge-device/commit/4141e4db308e1b0d4cc27b2dd55bcb64c42113c4))
* **sdkconfig:** Disable IPv6 ([7922855](https://github.com/Slider0007/AI-on-the-edge-device/commit/79228559a38ad9069651cae04117e1aa1181549a))
* **submodule:** Update esp-camera v2.04 ([cc9e03b](https://github.com/Slider0007/AI-on-the-edge-device/commit/cc9e03bad42dd49015170f735bb2fa6396d91528))
* **submodule:** Update esp-nn v1.0-rc1 ([cc9e03b](https://github.com/Slider0007/AI-on-the-edge-device/commit/cc9e03bad42dd49015170f735bb2fa6396d91528))
* **submodule:** Update tflite (master, 2023-05-08) ([cc9e03b](https://github.com/Slider0007/AI-on-the-edge-device/commit/cc9e03bad42dd49015170f735bb2fa6396d91528))
* **submodule:** Update TFLite related submodules -&gt; Reduced flash/RAM usage ([e70db93](https://github.com/Slider0007/AI-on-the-edge-device/commit/e70db939e2f787304b59d9009fd6485f3bb93e51))
* **testcases:** Adapt test cases to new naming + add test cases ([194f87c](https://github.com/Slider0007/AI-on-the-edge-device/commit/194f87ce999a170bc51e7d97923ecdf68630f537))
* **tflite:** New dig-class100 model (provided by haverland) ([9cff3e5](https://github.com/Slider0007/AI-on-the-edge-device/commit/9cff3e5bda258ea75f77a613e2b25234dd46f514))
* **tflite:** New digit/analog models (provided by haverland) ([a11e0f6](https://github.com/Slider0007/AI-on-the-edge-device/commit/a11e0f6930dff8debda670792efdf5da15f0b918))
* **tflite:** New tflite models (provided by haverland) ([6dc06bd](https://github.com/Slider0007/AI-on-the-edge-device/commit/6dc06bd4980417e9dee55adaee049397c800ea9a))
* **webui:** Config: Parameter `Use FallbackValue` delcare as expert parameter ([d07b86f](https://github.com/Slider0007/AI-on-the-edge-device/commit/d07b86fa76a07b6ee278a3ea0a7bebc2cdaa3c48))
* **webui:** Update image files for flowstates ([e68789a](https://github.com/Slider0007/AI-on-the-edge-device/commit/e68789af671539c0f68bc6c89a44da3a10781b69))
* **webui:** Update copyright year

## [15.1.1] - 2023-03-23

### Update Procedure

Update Procedure see [online documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#update-ota-over-the-air)

### Changes

For a full list of changes see [Full list of changes](https://github.com/jomjol/AI-on-the-edge-device/compare/v15.1.0...v15.1.1)

#### Added

- [#2206](https://github.com/jomjol/AI-on-the-edge-device/pull/2206) Log PSRAM usage
- [#2216](https://github.com/jomjol/AI-on-the-edge-device/pull/2216) Log MQTT connection refused reasons

#### Changed

- n.a.

#### Fixed

-  [#2224](https://github.com/jomjol/AI-on-the-edge-device/pull/2224), [#2213](https://github.com/jomjol/AI-on-the-edge-device/pull/2213) Reverted some of the PSRAM usage changes due to negative sideffects 
-  [#2203](https://github.com/jomjol/AI-on-the-edge-device/issues/2203) Correct API for pure InfluxDB v1
-  [#2180](https://github.com/jomjol/AI-on-the-edge-device/pull/2180) Fixed links in Parameter Documentation
-  Various minor fixes

#### Removed

-   n.a.

## [15.1.0] - 2023-03-12

### Update Procedure

Update Procedure see [online documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#update-ota-over-the-air)

:bangbang: Afterwards you should force-reload the Web Interface (usually Ctrl-F5 will do it)!

:bangbang: Afterwards you should check your configuration for errors!

### Changes

For a full list of changes see [Full list of changes](https://github.com/jomjol/AI-on-the-edge-device/compare/v15.0.3...v15.1.0)

#### Added
- The Configuration page has now tooltips with enhanced documentation
- MQTT:
    - Added `GJ` (`gigajoule`) as an energy meter unit
    - Removed State Class and unit from `raw` topic
    - Various Improvements (Only send Homeassistant Discovery the first time we connect, ...) (https://github.com/jomjol/AI-on-the-edge-device/pull/2091
- Added Expert Parameter to change CPU Clock from `160` to `240 Mhz`
- SD card basic read/write check and a folder/file presence check at boot to indicate SD card issues or missing folders / files ([#2085](https://github.com/jomjol/AI-on-the-edge-device/pull/2085))
- Simplified "WIFI roaming" by client triggered channel scan (AP switching at low RSSI) -> using expert parameter "RSSIThreshold" ([#2120](https://github.com/jomjol/AI-on-the-edge-device/pull/2120))
- Log WLAN disconnect reason codes (see [WLAN disconnect reasons](https://jomjol.github.io/AI-on-the-edge-device-docs/WLAN-disconnect-reason))
- Support of InfluxDB v2 ([#2004](https://github.com/jomjol/AI-on-the-edge-device/pull/2004))


#### Changed
- Updated models (tflite files), removed old versions (https://github.com/jomjol/AI-on-the-edge-device/pull/2089, https://github.com/jomjol/AI-on-the-edge-device/pull/2133)
  :bangbang: **Attention:** Update your configuration!
    -   Hybrid CNN network to `dig-cont_0611_s3` 
    -   Analog CNN network to `ana-cont-11.0.5` and `ana-clas100-1.5.7`
    -   Digital CNN network to `dig-class100-1.6.0`
-   Various Web interface Improvements/Enhancements:
    - Restructured Menu (Needs cache clearing to be applied)
    - Enhanced `Previous Value` page
    - Improved/faster Graph page
    - Various minor improvements
    - ROI config pages improvements
    - Improved Backup Functionality
- Added log file logs for Firmware Update
- Improved memory management (moved various stuff to external PSRAM, https://github.com/jomjol/AI-on-the-edge-device/pull/2117)
- Camera driver update: Support of contrast and saturation ([#2048](https://github.com/jomjol/AI-on-the-edge-device/pull/2048))   
  :bangbang:  **Attention**: This could have impact to old configurations. Please check your configuration and potentially adapt parametrization, if detection is negativly affected.
- Improved error handling and provide more verbose output in error cases during boot phase ([#2020](https://github.com/jomjol/AI-on-the-edge-device/pull/2020))
- Red board LED is indicating more different errors and states (see [Status LED Blink Codes](https://jomjol.github.io/AI-on-the-edge-device-docs/StatusLED-BlinkCodes))
- Logfile: Print start indication block after time is synced to indicate start in logfile after a cold boot
- `Image Quality Index`: Limit lower input range to 8 to avoid system instabilities

#### Fixed
- Various minor fixes
- Added State Class "measurement" to rate_per_time_unit
- GPIO: Avoid MQTT publishing to empty topic when "MQTT enable" flag is not set
- Fix timezone config parser
- Remote Setup truncated long passwords (https://github.com/jomjol/AI-on-the-edge-device/issues/2167)
-  Problem with timestamp in InfluxDB interface

#### Removed
-   n.a.


## [15.0.3] - 2023-02-28

**Name: Parameter Migration**

### Update Procedure

Update Procedure see [online documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#update-ota-over-the-air)

:bangbang: Afterwards you should force-reload the Web Interface (usually Ctrl-F5 will do it).

### Changes

This release only migrates some parameters, see #2023 for details and a list of all parameter changes.
The parameter migration happens automatically on the next startup. No user interaction is required.
A backup of the config is stored on the SD-card as `config.bak`.

Beside of the parameter change and the bugfix listed below, no changes are contained in this release!

If you want to revert back to `v14` or earlier, you will have to revert the migration changes in `config.ini` manually!

#### Added

-   n.a.

#### Changed

-   [#2023](https://github.com/jomjol/AI-on-the-edge-device/pull/2023) Migrated Parameters
-   Removed old `Topic` parameter, it is not used anymore

#### Fixed

-   [#2036](https://github.com/jomjol/AI-on-the-edge-device/issues/2036) Fix wrong url-encoding
-   **NEW v15.0.2:**  [#1933](https://github.com/jomjol/AI-on-the-edge-device/issues/1933) Bugfix InfluxDB Timestamp
-   **NEW v15.0.3:**  Re-added lost dropdownbox filling for Postprocessing Individual Parameters

#### Removed

-   n.a.


## [14.0.3] -2023-02-05

**Name: Stabilization and Improved User Experience**

Thanks to over 80 Pull Requests from 6 contributors, we can anounce another great release with many many improvements and new features:

### Update Procedure

Update Procedure see [online documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#update-ota-over-the-air)

### Changes

For a full list of changes see [Full list of changes](https://github.com/jomjol/AI-on-the-edge-device/compare/v13.0.8...v14.0.0)

#### Added

-   [1877](https://github.com/jomjol/AI-on-the-edge-device/pull/1877) Show WIFI signal text labels / Log RSSI value to logfile
-   [1671](https://github.com/jomjol/AI-on-the-edge-device/pull/1671) Added experimental support for WLAN 802.11k und 802.11v (Mesh-Support)
-   Web UI caching of static files
-   Added various debug tools
-   [1798](https://github.com/jomjol/AI-on-the-edge-device/pull/1798) Add error handling for memory intensive tasks
-   [1784](https://github.com/jomjol/AI-on-the-edge-device/pull/1784) Add option to disable brownout detector
-   Added full web browser based installation mode (including initial setup of SD-card) - see [WebInstaller](https://jomjol.github.io/AI-on-the-edge-device/index.html)
-   Added [Demo Mode](https://jomjol.github.io/AI-on-the-edge-device-docs/Demo-Mode)
-   [1648](https://github.com/jomjol/AI-on-the-edge-device/pull/1648) Added trigger to start a flow by [REST](https://jomjol.github.io/AI-on-the-edge-device-docs/REST-API) API or [MQTT](https://jomjol.github.io/AI-on-the-edge-device-docs/MQTT-API/)
-   Show special images during steps `Initializing` and `Take Image` as the current camera image might be incomplete or outdated

#### Changed

-   Migrated documentation (Wiki) to <https://jomjol.github.io/AI-on-the-edge-device-docs>. Please help us to make it even better.
-   New OTA Update page with progress indication
-   Various memory optimizations
-   Cleanup code/Web UI
-   Updated models
-   [1809](https://github.com/jomjol/AI-on-the-edge-device/pull/1809) Store preprocessed image with ROI to RAM
-   Better log messages on some errors/issues
-   [1742](https://github.com/jomjol/AI-on-the-edge-device/pull/1742) Replace alert boxes with overlay info boxes
-   Improve log message when web UI is installed incomplete
-   [1676](https://github.com/jomjol/AI-on-the-edge-device/pull/1676) Improve NTP handling
-   HTML: improved user informations (info boxes, error hints, ...)
-   [1904](https://github.com/jomjol/AI-on-the-edge-device/pull/1904) Removed newlines in JSON and replaced all whitespaces where there was more than one

#### Fixed

-   Fixed many many things
-   [1509](https://github.com/jomjol/AI-on-the-edge-device/pull/1509) Protect `wifi.ini` from beeing deleted.
-   [1530](https://github.com/jomjol/AI-on-the-edge-device/pull/1530) Homeassistant `Problem Sensor`
-   [1518](https://github.com/jomjol/AI-on-the-edge-device/pull/1518) JSON Strings
-   [1817](https://github.com/jomjol/AI-on-the-edge-device/pull/1817) DataGraph: datafiles sorted -> newest on top

#### Removed

-   n.a.

## [13.0.8] - 2022-12-19

**Name: Home Assistant MQTT Discovery Support**

### Update Procedure see [online documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#update-ota-over-the-air)

### Added

-   Implementation of [Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
-   Improved ROIs configuration: locked ROI geometry, equidistant delta x
-   Improved OTA Update mechanism (only working after installation for next update)
-   Added data logging in `/log/data` - One day per file and each measurement is on one line
    -   Format: csv - comma separated
    -   Content: `time`, `name-of-number`, `raw-value`, `return-value`, `pre-value`, `change-rate`, `change-absolute`, `error-text`, `cnn-digital`, `cnn-analog`
-   Show graph of values direct in the user interface (thanks to [@rdmueller](https://github.com/rdmueller))

    -   Using new data logging (see above)
    -   Possibility to choose different values and switch between different numbers (if present)

    Note: You need to activate data logging for this feature to work, see above!
-   PreValue is now contained in `/json` ([#1154](https://github.com/jomjol/AI-on-the-edge-device/issues/1154))
-   SD card info into the `System>Info` menu (thanks to [@Slider007](https://github.com/Slider0007))
-   Version check (Firmware vs. Web UI)
-   Various minor new features

### Changed

-   Updated tflite (`dig-cont_0600_s3.tflite`)
-   Updated OTA functionality (more robust, but not fully bullet prove yet)
-   Updated Espressif library to `espressif32@v5.2.0`
-   [#1176](https://github.com/jomjol/AI-on-the-edge-device/discussions/1176) accept minor negative values (-0.2) if extended resolution is enabled
-   [#1143](https://github.com/jomjol/AI-on-the-edge-device/issues/1143) added config parameter `AnalogDigitalTransitionStart`. It can setup very early and very late digit transition starts.
-   New version of `dig-class100` (v1.4.0): added images of heliowatt powermeter 
-   NEW v13.0.2: Update Tool "Logfile downloader and combiner" to handle the new csv file format.
-   NEW v13.0.2: MQTT: Added MQTT topic `status` (Digitalization Status), Timezone to MQTT topic `timestamp`.#
-   NEW v13.0.2: Logging: Disable heap logs by default, cleanup
-   NEW v13.0.7:
    -   log NTP server name
    -   Improved log messages
    -   Various preparations for next release
-   **NEW v13.0.8**: 
    -   Continue booting on PSRAM issues, Web UI will show an error
    -   Updated models
    -   Various UI enhancements
    -   Various internal improvements
    -   Show uptime in log
    -   Show uptime and round on overview page

### Fixed

-   [#1116](https://github.com/jomjol/AI-on-the-edge-device/issues/1116) precision problem at setting prevalue
-   [#1119](https://github.com/jomjol/AI-on-the-edge-device/issues/1119) renamed `firmware.bin` not working in OTA
-   [#1143](https://github.com/jomjol/AI-on-the-edge-device/issues/1143) changed postprocess for `analog->digit` (lowest digit processing)
-   [#1280](https://github.com/jomjol/AI-on-the-edge-device/issues/1280) check ROIs name for unsupported characters
-   [#983](https://github.com/jomjol/AI-on-the-edge-device/issues/983) old log files did not get deleted 
-   Failed NTP time sync during startup gets now retried every round if needed
-   Whitespaces and `=` in MQTT and InfluxDB passwords
-   Various minor fixes and improvements
-   NEW v13.0.2: Corrected Version comparison between firmware and Web UI.
-   NEW v13.0.3: Re-updated build environment to v5.2.0 (from accidental downgrad to v4.4.0)
-   NEW v13.0.4: Fix for reboot in case of MQTT not used
-   NEW v13.0.5: No reboot in case of missing NTP-connection
-   NEW v13.0.7:
    -   Prevent autoreboot on cam framebuffer init error
    -   Properly protect `wlan.ini` against deletion
    -   Fixed various MQTT topic content issues
    -   Fix Digit detected as 10 (<https://github.com/jomjol/AI-on-the-edge-device/pull/1525>)
    -   Fix frozen time in datafile on error
    -   Various minor fixes
-   **NEW v13.0.8**: 
    -   Fix Rate Problem ([#1578](https://github.com/jomjol/AI-on-the-edge-device/issues/1578), [#1572](https://github.com/jomjol/AI-on-the-edge-device/issues/1572))
    -   Stabilized MQTT
    -   Fixed redundant calls in OTA
    -   Block REST API calls till resource is ready
    -   Fixed number renaming ([#1635](https://github.com/jomjol/AI-on-the-edge-device/issues/1635))

### Removed

-   n.a.

## [12.0.1] 2022-09-29

Name: Improve **u**ser e**x**perience 

:bangbang: The release breaks a few things in ota update :bangbang:

**Make sure to read the instructions below carfully!**.

1.  Backup your configuration (use the `System > Backup/Restore` page)!
2.  You should update to `11.3.1` before you update to this release. All other migrations are not tested. 
    Rolling newer than `11.3.1` can also be used, but no guaranty.
3.  Upload and update the `firmware.bin` file from this release. **but do not reboot**
4.  Upload the `html-from-11.3.1.zip` in html upload and update the web interface.
5.  Now you can reboot.

If anything breaks you can try to
1\. Call `http://<IP>/ota?task=update&file=firmware.bin` resp. `http://<IP>/ota?task=update&file=html.zip` if the upload successed but the extraction failed.
1\. Use the initial_esp32_setup.zip ( <https://github.com/jomjol/AI-on-the-edge-device/wiki/Installation> ) as alternative.

### Added

-   Automatic release creation
-   Newest firmware of rolling branch now automatically build and provided in [Github Actions Output](https://github.com/jomjol/AI-on-the-edge-device/actions) (developers only)
-   [#1068](https://github.com/jomjol/AI-on-the-edge-device/issues/1068) New update mechanism: 
    -   Handling of all files (`zip`, `tfl`, `tflite`, `bin`) within in one common update interface
    -   Using the `update.zip` from the [Release page](https://github.com/jomjol/AI-on-the-edge-device/releases)
    -   Status (`upload`, `processing`, ...) displayed on Web Interface
    -   Automatical detection and suggestion for reboot where needed (Web Interface uupdates only need a page refresh)
    -   :bangbang: Best for OTA use Firefox. Chrome works with warnings. Safari stuck in upload.

### Changed

-   Integrated version info better shown on the Info page and in the log
-   Updated menu
-   Update used libraries (`tflite`, `esp32-cam`, `esp-nn`, as of 20220924) 

### Fixed

-   [#1092](https://github.com/jomjol/AI-on-the-edge-device/issues/1092) censor passwords in log outputs 
-   [#1029](https://github.com/jomjol/AI-on-the-edge-device/issues/1029) wrong change of `checkDigitConsistency` now working like releases before `11.3.1` 
-   Spelling corrections (**[cristianmitran](https://github.com/cristianmitran)**) 

### Removed

-   Remove the folder `/firmware` from GitHub repository. 
    If you want to get the latest `firmware.bin` and `html.zip` files, please download from the automated [build action](https://github.com/jomjol/AI-on-the-edge-device/actions) or [release page](https://github.com/jomjol/AI-on-the-edge-device/releases)

## [11.3.1](https://github.com/jomjol/AI-on-the-edge-device/releases/tag/v11.3.1), 2022-09-17

Intermediate Digits

-   **ATTENTION**: 

    -   first update the `firmware.bin` and ensure that the new version is running

    -   Only afterwards update the `html.zip`

    -   Otherwise the downwards compatibility of the new counter clockwise feature is not given and you end in a reboot loop, that needs manual flashing!


-   **NEW v11.3.1**: corrected corrupted asset `firmware.bin`
-   Increased precision (more than 6-7 digits)
-   Implements Counter Clockwise Analog Pointers
-   Improved post processing algorithm
-   Debugging: intensive use of testcases
-   MQTT: improved handling, extended logging, automated reconnect
-   HTML: Backup Option for Configuration
-   HTML: Improved Reboot
-   HTML: Update WebUI (Reboot, Infos, CPU Temp, RSSI)
-   This version is largely also based on the work of **[caco3](https://github.com/caco3)**,  **[adellafave](https://github.com/adellafave)**,  **[haverland](https://github.com/haverland)**,  **[stefanbode](https://github.com/stefanbode)**, **[PLCHome](https://github.com/PLCHome)**

## [11.2.0](https://github.com/jomjol/AI-on-the-edge-device/releases/tag/v11.2.0), 2022-08-28

Intermediate Digits

-   Updated Tensorflow / TFlite to newest tflite (version as of 2022-07-27)

-   Updated analog neural network file (`ana-cont_11.3.0_s2.tflite` - default, `ana-class100_0120_s1_q.tflite`)

-   Updated digital neural network file (`dig-cont_0570_s3.tflite` - default, `dig-class100_0120_s2_q.tflite`)

-   Added automated filtering of tflite-file in the graphical configuration (thanks to @**[caco3](https://github.com/caco3)**)

-   Updated consistency algorithm & test cases

-   HTML: added favicon and system name, Improved reboot dialog  (thanks to @**[caco3](https://github.com/caco3)**)

## [11.1.1](https://github.com/jomjol/AI-on-the-edge-device/releases/tag/v11.1.1), 2022-08-22

Intermediate Digits

-   New and improved consistency check (especially with analog and digital counters mixed)
-   Bug Fix: digital counter algorithm

## [11.0.1](https://github.com/jomjol/AI-on-the-edge-device/releases/tag/v11.0.1), 2022-08-18

Intermediate Digits

-   **NEW v11.0.1**: Bug Fix InfluxDB configuration (only update of html.zip necessary)

-   Implementation of new CNN types to detect intermediate values of digits with rolling numbers

    -   By default the old algo (0, 1, ..., 9, "N") is active (due to the limited types of digits trained so far)
    -   Activation can be done by selection a tflite file with the new trained model in the 'config.ini'
    -   **Details can be found in the [wiki](https://github.com/jomjol/AI-on-the-edge-device/wiki/Neural-Network-Types)** (different types, trained image types, naming convention)

-   Updated  neural network files (and adaption to new naming convention)

-   Published a tool to download and combine log files - **Thanks to **

    -   Files see ['/tools/logfile-tool'](tbd), How-to see [wiki](https://github.com/jomjol/AI-on-the-edge-device/wiki/Gasmeter-Log-Downloader)

-   Bug Fix: InfluxDB enabling in grahic configuration

## [10.6.2](https://github.com/jomjol/AI-on-the-edge-device/releases/tag/v10.6.2), 2022-07-24

Stability Increase

### Added

-   **NEW 10.6.2**: ignore hidden files in model selection (configuration page)

-   **NEW 10.6.1**: Revoke esp32cam & tflite update

-   **NEW 10.6.1**: Bug Fix: tflite-filename with ".", HTML spelling error

-   IndluxDB: direct injection into InfluxDB - thanks to **[wetneb](https://github.com/wetneb)**

-   MQTT: implemented "Retain Flag" and extend with absolute Change (in addition to rate)

-   `config.ini`: removal of modelsize (readout from tflite)

-   Updated analog neural network file (`ana1000s2.tflite`) & digital neural network file (`dig1400s2q.tflite`)

-   TFMicro/Lite: Update (espressif Version 20220716)

-   Updated esp32cam (v20220716)

-   ESP-IDF: Update to 4.4

-   Internal update (CNN algorithm optimizations, reparation for new neural network type)

-   Bug Fix: no time with fixed IP, Postprocessing, MQTT

## [10.5.2](https://github.com/jomjol/AI-on-the-edge-device/releases/tag/v10.5.2), 2022-02-22

Stability Increase

### Changed

-   NEW 10.5.2: Bug Fix: wrong `firmware.bin` (no rate update)
-   NEW 10.5.1: Bug Fix: wrong return value, rate value & PreValue status, HTML: SSID & IP were not displayed
-   MQTT: changed wifi naming to "wifiRSSI"
-   HTML: check selectable values for consistency
-   Refactoring of check postprocessing consistency (e.g. max rate, negative rate, ...)
-   Bug Fix: corrected error in "Check Consistency Increase"

## [10.4.0](https://github.com/jomjol/AI-on-the-edge-device/releases/tag/v10.4.0), 2022-02-12

Stability Increase

### Changed

-   Graphical configuration: select available neural network files (_.tfl,_.tflite) from drop down menu
-   OTA-update: add option to upload tfl / tflite files to the correct location (`/config/`)
    -   In the future the new files will also be copied to the `firmware` directory of the repository
-   Added Wifi RSSI to MQTT information
-   Updated analog neural network file (`ana-s3-q-20220105.tflite`)
-   Updated digital neural network file (`dig-s1-q-20220102.tflite`)
-   Updated build environment to `Espressif 3.5.0`

## [10.3.0] - (2022-01-29)

Stability Increase

### Changed

-   Implemented LED flash dimming (`LEDIntensity`).
    Remark: as auto illumination in the camera is used, this is rather for energy saving. It will not help reducing reflections
-   Additional camera parameters: saturation, contrast (although not too much impact yet)
-   Some readings will have removable "N"s that can not be removed automatically and are handled with an "error" --> no return value in the field "value" anymore (still reported back via field "raw value")
-   Updated esp32 camera hardware driver
-   Bug fix: MQTT, HTML improvements

**ATTENTION:  The new ESP32 camera hardware driver is much more stable on newer OV2640 versions (no or much less reboots) but seems to be not fully compatible with older versions.**

If you have problem with stalled systems you can try the following

-   Update the parameter `ImageQuality` to `12` instead of current value `5` (manually in the `config.ini`)

-   If this is not helping, you might need to update your hardware or stay with version 9.2

## [10.2.0] - (2022-01-14)

Stability Increase

### Changed

-   Due to the updated camera driver, the image looks different and a new setup might be needed

    -   Update reference image
    -   Update Alignment marks

-   Reduce reboot due to camera problems

-   Update esp32-camera to new version (master as of 2022-01-09)

## [10.1.1] - (2022-01-12)

 Stability Increase

### Changed

-   Bug Fix MQTT problem
-   Issue:
    -   Changing from v9.x to 10.x the MQTT-parameter "Topic" was renamed into "MainTopic" to address multiple number meters. This renaming should have been done automatically in the background within the graphical configuration, but was not working. Instead the parameter "Topic" was deleted and "MainTopic" was set to disabled and "undefined".
-   ToDo
    -   Update the `html.zip`
    -   If old `config.ini` available: copy it to `/config`, open the graphical configuration and save it again.
    -   If old `config.ini` not available: reset the parameter "MainTopic" within the `config.ini` manually
    -   Reboot

## [10.1.0] -  (2022-01-09)

Stability Increase

### Changed

-   Reduce ESP32 frequency to 160MHz

-   Update tflite (new source: <https://github.com/espressif/tflite-micro-esp-examples>)

-   Update analog neural network (ana-s3-q-20220105.tflite)

-   Update digital neural network (dig-s1-q-20220102.tflite)

-   Increased web-server buffers

-   bug fix: compiler compatibility

## [10.0.2] - (2022-01-01)

Stability Increase

### Changed

-   NEW v10.0.2: Corrected JSON error

-   Updated compiler toolchain to ESP-IDF 4.3

-   Removal of memory leak

-   Improved error handling during startup (check PSRAM and camera with remark in logfile)

-   MQTT: implemented raw value additionally, removal of regex contrain

-   Normalized Parameter `MaxRateValue`  to "change per minute"

-   HTML: improved input handling

-   Corrected error handling: in case of error the old value, rate, timestamp are not transmitted any more

## [9.2.0] -  (2021-12-02)

External Illumination

### Changed

-   Direct JSON access: `http://IP-ADRESS/json`
-   Error message in log file in case camera error during startup
-   Upgrade analog CNN to v9.1.0
-   Upgrade digital CNN to v13.3.0 (added new images)
-   html: support of different ports

## [9.1.1] - External Illumination (2021-11-16)

### Changed

-   NEW 9.1.1 bug fix: LED implemenetation
-   External LEDs: change control mode (resolve bug with more than 2 LEDs)
-   Additional info into log file
-   Bug fix: decimal shift, html, log file

## [9.0.0] - External Illumination (2021-10-23)

### Changed

-   Implementation of external illumination to adjust positioning, brightness and color of the illumination now set individually
    -   Technical details can be found in the wiki: <https://github.com/jomjol/AI-on-the-edge-device/wiki/External-LED>
        <img src="https://raw.githubusercontent.com/jomjol/ai-on-the-edge-device/master/images/intern_vs_external.jpg" width="500">
-   New housing published for external LEDs and small clearing: <https://www.thingiverse.com/thing:5028229>

## [8.5.0] - Multi Meter Support (2021-10-07)

### Changed

-   Upgrade digital CNN to v13.1.0 (added new images)
-   bug fix: wlan password with space, double digit output

## [8.4.0] - Multi Meter Support (2021-09-25)

### Changed

-   License change (remove MIT license, remark see below)

-   html: show hostname in title and main page

-   configuration:

    -   moved setting `ExtendedResolution` to individual number settings
    -   New parameter `IgnoreLeadingNaN` (delete leading NaN's specifically)
    -   **ATTENTION**: update of the `config.ini` needed (open, adjust `ExtendedResolution`, save)

-   Bug fixing (html, images of recognized numbers)

    **ATTENTION: LICENSE CHANGE - removal of MIT License.**

-   Currently no licence published - copyright belongs to author

-   If you are interested in a commercial usage or dedicated versions please contact the developer
    -   no limits to private usage

## [8.3.0] - Multi Meter Support (2021-09-12)

### Changed

-   Upgrade digital CNN to v12.1.0 (added new images)
-   Dedicated NaN handling, internal refactoring (CNN-Handling)
-   HTML: confirmation after config.ini update
-   Bug fixing

## [8.2.0] - Multi Meter Support (2021-08-24)

### Changed

-   Improve server responsiveness


-   Flow status and prevalue status in overview
-   Improved prevalue handling

## [8.1.0] - Multi Meter Support (2021-08-12)

### Changed

-   GPIO: using the general mqtt main topic for GPIO


-   Upgrade digital CNN to v12.0.0  (added new images)
-   Update tfmicro to new master (2021-08-07)
-   Bug fix: remove text in mqtt value, remove connect limit in wlan reconnet

## [8.0.5] - Multi Meter Support (2021-08-01)

### Changed

-   NEW 8.0.5: bug fix: saving prevalue


-   NEW 8.0.4: bug fix: load config.ini after upgrade
-   NEW 8.0.3: bug fix: reboot during `config.ini` handling, html error
-   NEW 8.0.2: saving roundes prevalue, bug fix html server
-   NEW 8.0.1: bug fix: html handling of parameter `FixedExposure` and `ImageSize`
-   Dual / multi meter support (more than 1 number to be recognized)
    This is implemented with the feature "number" on the ROI definition as well as selected options
-   MQTT: standardization of the naming - including new topics (`json`,  `freeMem`, `uptime`)c
-   Preparation for extended GPIO support (thanks to Zwerk2k) - not tested and fully functional yet
-   Bug fixing: html server, memory leak, MQTT connect, hostname, turn of flash LED

<span style="color: red;">**ATTENTION: the configuration and prevalue files are modified automatically and will not be backward compatible!**</span>

## [7.1.2] MQTT-Update - (2021-06-17)

### Changed

-   NEW: 7.1.2: bug fix setting hostname, Flash-LED not off during reboot


-   NEW: 7.1.1: bug fix wlan password with "="  (again)

-   MQTT error message: changes "no error", send retain flag

-   Update wlan handling to esp-idf 4.1

-   Upgrade digital CNN to v8.7.0  (added new images)

-   Bug fix: MQTT, WLAN, LED-Controll, GPIO usage, fixed IP, calculation flow rate

## [7.0.1] MQTT-Update - (2021-05-13)

### Changed

-   NEW: 7.0.1: bug fix wlan password with "="


-   Upgrade digital CNN to v8.5.0  (added new images)

-   New MQTT topics: flow rate (units/minute), time stamp (last correct read readout)

-   Update MQTT/Error topic to " " in case no error (instead of empty string)

-   Portrait or landscape image orientation in rotated image (avoid cropping)

## [6.7.2] Image Processing in Memory - (2021-05-01)

### Changed

-   NEW 6.7.2: Updated html for setup modus - remove reboot on edit configuration)


-   NEW 6.7.1: Improved stability of camera (back to v6.6.1) - remove black strips and areas

-   Upgrade digital CNN to v8.3.0  (added new type of digits)

-   Internal update: TFlite (v2.5), esp32cam, startup sequence

-   Rollback to espressif v2.1.0, as v3.2.0 shows unstable reboot

-   Bugfix: WLan-passwords, reset of hostname

## [6.6.1] Image Processing in Memory - (2021-04-05)

### Changed

-   NEW 6.6.1: failed SD card initialization indicated by fast blinking LED at startup


-   Improved SD-card handling (increase compatibility with more type of cards)

## [6.5.0] Image Processing in Memory - (2021-03-25)

### Changed

-   Upgrade digital CNN to v8.2.0  (added new type of digits)


-   Supporting alignment structures in ROI definition
-   Bug fixing: definition of  hostname in `config.ini`

## [6.4.0] Image Processing in Memory - (2021-03-20)

### Changed

-   Additional alignment marks for settings the ROIs (analog and digit)


-   Upgrade analog CNN to v7.0.0 (added new type of pointer)

## [6.3.1] Image Processing in Memory - (2021-03-16)

### Changed

-   NEW: 6.3.1: bug fixing in initial edit reference image and `config.ini` (Spelling error in `InitialRotate`)


-   Initial setup mode: bug fixing, error correction
-   Bug-fixing

## [6.2.2] Image Processing in Memory - (2021-03-10)

### Changed

-   NEW 6.2.2: bug fixing


-   NEW 6.2.1: Changed brightness and contrast to default if not enabled (resolves to bright images)

-   Determination of fixed illumination settings during startup - speed up of 5s in each run

-   Update digital CNN to v8.1.1 (additional digital images trained)

-   Extended error message in MQTT error message

-   Image brightness is now adjustable

-   Bug fixing: minor topics

## [6.1.0] Image Processing in Memory - (2021-01-20)

### Changed

-   Disabling of analog / digital counters in configuration


-   Improved Alignment Algorithm (`AlignmentAlgo`  = `Default`,  `Accurate` , `Fast`)
-   Analog counters: `ExtendedResolution` (last digit is extended by sub comma value of CNN)
-   `config.ini`: additional parameter `hostname`  (additional to wlan.ini)
-   Switching of GPIO12/13 via http-interface: `/GPIO?GPIO=12&Status=high/low`
-   Bug fixing: html configuration page, wlan password ("=" now possible)

## [6.0.0] Image Processing in Memory - (2021-01-02)

### Changed

-   **Major change**: image processing fully in memory - no need of SD card buffer anymore

    -   Need to limit camera resolution to VGA (due to memory limits)


-   MQTT: Last Will Testament (LWT) implemented: "connection lost" in case of connection lost to `TopicError`
-   Disabled `CheckDigitIncreaseConsistency` in default configuration - must now be explicit enabled if needed
-   Update digital CNN to v7.2.1 (additional digital images trained)
-   Setting of arbitrary time server in `config.ini`
-   Option for fixed IP-, DNS-Settings in `wlan.ini`
-   Increased stability (internal image and camera handling)
-   Bug fixing: edit digits, handling PreValue, html-bugs

## [5.0.0] Setup Modus - (2020-12-06)

### Changed

-   Implementation of initial setup modus for fresh installation


-   Code restructuring (full compatibility between pure ESP-IDF and Platformio w/ espressif)

## [4.1.1] Configuration editor - (2020-12-02)

### Changed

-   Bug fixing: internal improvement of file handling (reduce not responding)

## [4.1.0] Configuration editor - (2020-11-30)

### Changed

-   Implementation of configuration editor (including basic and expert mode)


-   Adjustable time zone to adjust to local time setting (incl. daylight saving time)

-   MQTT: additional topic for error reporting

-   standardized access to current logfile via `http://IP-ADRESS/logfileact`

-   Update digital CNN to v7.2.0, analog CNN to 6.3.0

-   Bug fixing: truncation error,  CheckDigitConsistency & PreValue implementation

## [4.0.0] Tflite Core - (2020-11-15)

### Changed

-   Implementation of rolling log-files


-   Update Tflite-Core to master@20201108 (v2.4)

-   Bug-fixing for reducing reboots

## [3.1.0] MQTT-Client - (2020-10-26)

### Changed

-   Update digital CNN to v6.5.0 and HTML (Info to hostname, IP, ssid)

-   New implementation of "checkDigitConsistency" also for digits

-   MQTT-Adapter: user and password for sign in MQTT-Broker

## [3.0.0] MQTT-Client  (2020-10-14)

### Changed

-   Implementation of MQTT Client


-   Improved Version Control
-   bug-fixing

## [2.2.1] Version Control  (2020-09-27)

### Changed

-   Bug-Fixing (hostname in wlan.ini and error handling inside flow)

## \[2.2.0| Version Control  (2020-09-27)

### Changed

-   Integrated automated versioning system (menu: SYSTEM --> INFO)


-   Update Build-System to PlatformIO - Espressif 32 v2.0.0 (ESP-IDF 4.1)

## [2.1.0] Decimal Shift, Chrome & Edge  (2020-09-25)

### Changed

-   Implementation of Decimal Shift


-   Update default CNN for digits to v6.4.0

-   Improvement HTML

-   Support for Chrome and Edge

-   Reduce logging to minimum - extended logging on demand

-   Implementation of hostname in wlan.ini (`hostname = "HOSTNAME")`

-   Bug fixing, code corrections

## [2.0.0] Layout update  (2020-09-12)

### Changed

-   Update to **new and modern layout**
-   Support for Chrome improved
-   Improved robustness: improved error handling in auto flow reduces spontaneous reboots
-   File server: Option for "DELETE ALL"
-   WLan: support of spaces in SSID and password
-   Reference Image: Option for mirror image, option for image update on the fly
-   additional parameter in `wasserzaehler.html?noerror=true`  to suppress an potential error message
-   bug fixing

## [1.1.3](2020-09-09)

### Changed

-   **Bug in configuration of analog ROIs corrected** - correction in v.1.0.2 did not work properly


-   Improved update page for the web server (`/html` can be updated via a zip-file, which is provided in `/firmware/html.zip`)
-   Improved Chrome support

## [1.1.0](2020-09-06)

### Changed

-   Implementation of "delete complete directory"
    **Attention: beside the `firmware.bin`, also the content of `/html` needs to be updated!**

## [1.0.2](2020-09-06)

### Changed

-   Bug in configuration of analog ROIs corrected


-   minor bug correction

## [1.0.1](2020-09-05)

### Changed

-   preValue.ini Bug corrected


-   minor bug correction

## [1.0.0](2020-09-04)

### Changed

-   **First usable version** - compatible to previous project (<https://github.com/jomjol/water-meter-system-complete>)


-   NEW:
    -   no docker container for CNN calculation necessary
    -   web based configuration editor on board

## [0.1.0](2020-08-07)

### Changed

-   Initial Version


[15.1.1]: https://github.com/jomjol/AI-on-the-edge-device/compare/v15.1.0...v15.1.1
[15.1.0]: https://github.com/jomjol/AI-on-the-edge-device/compare/v15.0.3...v15.1.0
[15.0.3]: https://github.com/jomjol/AI-on-the-edge-device/compare/v14.0.3...v15.0.3
[14.0.3]: https://github.com/jomjol/AI-on-the-edge-device/compare/v13.0.8...v14.0.3
[13.0.8]: https://github.com/jomjol/AI-on-the-edge-device/compare/v12.0.1...v13.0.8
[13.0.7]: https://github.com/jomjol/AI-on-the-edge-device/compare/v12.0.1...v13.0.7
[13.0.5]: https://github.com/jomjol/AI-on-the-edge-device/compare/v12.0.1...v13.0.5
[13.0.4]: https://github.com/jomjol/AI-on-the-edge-device/compare/v12.0.1...v13.0.4
[13.0.1]: https://github.com/jomjol/AI-on-the-edge-device/compare/v12.0.1...v13.0.1
[12.0.1]: https://github.com/jomjol/AI-on-the-edge-device/compare/v11.3.1...v12.0.1
[11.4.3]: https://github.com/haverland/AI-on-the-edge-device/compare/v10.6.2...v11.4.3
[11.4.2]: https://github.com/haverland/AI-on-the-edge-device/compare/v10.6.2...v11.4.2
[11.3.9]: https://github.com/haverland/AI-on-the-edge-device/compare/v10.6.2...v11.3.9
