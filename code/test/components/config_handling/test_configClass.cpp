#include "configClass.h"

static const char *TAG_CFGTEST = "CONFIG_TEST";


/**
 * @brief Test JSON parse and serialization
 *
 * Be aware: The results are depending on the device for which it's compiled (GPIO section derived from hardware config).
 */
void test_configJsonParseAndSerialization()
{
    /* @TODO: Possible to add without embedded files having an additional environment or only add for test environment?
    *  1. Needs to be added to platformio.ini:
            board_build.embed_txtfiles = list of files
        or
    *   target_add_binary_data(firmware.elf "config_json_default_expected.txt" TEXT) in CMakeFile.txt
    *
    *  2. Needs to be added to CMakeList.txt of component:
    *       EMBED_TXTFILES config_json_default_expected.txt
    *
    * Workaround: Add content as string into code

    // Embedded file: Default config
    extern const char config_json_default_expected_start[] asm("_binary_config_json_default_expected_txt_start");
    extern const char config_json_default_expected_end[] asm("_binary_config_json_default_expected_txt_end");
    */


    // Check default values (ESP32CAM device)
    ESP_LOGI(TAG_CFGTEST, "TEST 1: Check default values");
    std::string cfgDataExpexcted =
        "{\"config\":{\"version\":4,\"lastmodified\":\"\"},\"operationmode\":{\"opmode\":-1,\"automaticprocessinterval\":\"1.0\","
        "\"usedemoimages\":false},\"takeimage\":{\"flashlight\":{\"flashtime\":2000,\"flashintensity\":50},\"camera\":{\"cameramodel\":1,"
        "\"camerafrequency\":20,\"imagequality\":12,\"brightness\":0,\"contrast\":0,\"saturation\":0,\"sharpness\":0,"
        "\"exposurecontrolmode\":1,\"autoexposurelevel\":0,\"manualexposurevalue\":300,\"gaincontrolmode\":1,\"manualgainvalue\":0,"
        "\"specialeffect\":0,\"mirrorimage\":false,\"flipimage\":false,\"zoomfactor\":1000,\"zoomoffsetx\":0,\"zoomoffsety\":0},\"debug\":"
        "{\"saverawimages\":false,\"rawimageslocation\":\"/log/"
        "source\",\"rawimagesretention\":3}},\"imagealignment\":{\"alignmentalgo\":0,\"searchfield\":{\"x\":20,\"y\":20},"
        "\"imagerotation\":\"0.0\",\"marker\":[{\"x\":1,\"y\":1},{\"x\":1,\"y\":1}],\"debug\":{\"savedebuginfo\":false}},"
        "\"numbersequences\":{\"sequence\":[{\"sequenceid\":0,\"sequencename\":\"main\"}]},\"digit\":{\"enabled\":true,\"model\":\"dig-"
        "class100_0173_s2_q.tflite\",\"cnngoodthreshold\":\"0.00\",\"sequence\":[{\"sequenceid\":0,\"sequencename\":\"main\",\"roi\":[]}],"
        "\"debug\":{\"saveroiimages\":false,\"roiimageslocation\":\"/log/"
        "digit\",\"roiimagesretention\":3}},\"analog\":{\"enabled\":true,\"model\":\"ana-class100_0171_s1_q.tflite\",\"sequence\":[{"
        "\"sequenceid\":0,\"sequencename\":\"main\",\"roi\":[]}],\"debug\":{\"saveroiimages\":false,\"roiimageslocation\":\"/log/"
        "analog\",\"roiimagesretention\":3}},\"postprocessing\":{\"sequence\":[{\"sequenceid\":0,\"sequencename\":\"main\","
        "\"decimalshift\":0,\"analogdigitsyncvalue\":\"9.2\",\"extendedresolution\":true,\"ignoreleadingnan\":false,"
        "\"checkdigitincreaseconsistency\":false,\"maxratechecktype\":1,\"maxrate\":\"0.150\",\"allownegativerate\":false,"
        "\"usefallbackvalue\":true,\"fallbackvalueagestartup\":720}],\"debug\":{\"savedebuginfo\":false}},\"mqtt\":{\"enabled\":false,"
        "\"uri\":\"\",\"maintopic\":\"watermeter\",\"clientid\":\"watermeter\",\"authmode\":0,\"username\":\"\",\"password\":\"\",\"tls\":"
        "{\"cacert\":\"\",\"clientcert\":\"\",\"clientkey\":\"\"},\"processdatanotation\":0,\"retainprocessdata\":false,\"homeassistant\":"
        "{\"discoveryenabled\":false,\"discoveryprefix\":\"homeassistant\",\"statustopic\":\"homeassistant/"
        "status\",\"metertype\":1,\"retaindiscovery\":false}},\"influxdbv1\":{\"enabled\":false,\"uri\":\"\",\"database\":\"\","
        "\"authmode\":0,\"username\":\"\",\"password\":\"\",\"tls\":{\"cacert\":\"\",\"clientcert\":\"\",\"clientkey\":\"\"},\"sequence\":"
        "[{\"sequenceid\":0,\"sequencename\":\"main\",\"measurementname\":\"\",\"fieldkey1\":\"\"}]},\"influxdbv2\":{\"enabled\":false,"
        "\"uri\":\"\",\"bucket\":\"\",\"organization\":\"\",\"authmode\":1,\"token\":\"\",\"tls\":{\"cacert\":\"\",\"clientcert\":\"\","
        "\"clientkey\":\"\"},\"sequence\":[{\"sequenceid\":0,\"sequencename\":\"main\",\"measurementname\":\"\",\"fieldkey1\":\"\"}]},"
        "\"webhook\":{\"enabled\":false,\"uri\":\"\",\"apikey\":\"\",\"publishimage\":0,\"authmode\":0,\"username\":\"\",\"password\":"
        "\"\",\"tls\":{\"cacert\":\"\",\"clientcert\":\"\",\"clientkey\":\"\"}},\"gpio\":{\"customizationenabled\":false,\"gpiopin\":[{"
        "\"gpionumber\":1,\"gpiousage\":\"restricted: "
        "uart0-tx\",\"pinenabled\":false,\"pinname\":\"\",\"pinmode\":\"input\",\"capturemode\":\"cyclic-polling\",\"inputdebouncetime\":"
        "200,\"pwmfrequency\":5000,\"logicactivelow\":false,\"exposetomqtt\":false,\"exposetorest\":false,\"smartled\":{\"type\":0,"
        "\"quantity\":1,\"colorredchannel\":255,\"colorgreenchannel\":255,\"colorbluechannel\":255},\"intensitycorrectionfactor\":100},{"
        "\"gpionumber\":3,\"gpiousage\":\"restricted: "
        "uart0-rx\",\"pinenabled\":false,\"pinname\":\"\",\"pinmode\":\"input\",\"capturemode\":\"cyclic-polling\",\"inputdebouncetime\":"
        "200,\"pwmfrequency\":5000,\"logicactivelow\":false,\"exposetomqtt\":false,\"exposetorest\":false,\"smartled\":{\"type\":0,"
        "\"quantity\":1,\"colorredchannel\":255,\"colorgreenchannel\":255,\"colorbluechannel\":255},\"intensitycorrectionfactor\":100},{"
        "\"gpionumber\":4,\"gpiousage\":\"flashlight-pwm\",\"pinenabled\":false,\"pinname\":\"\",\"pinmode\":\"flashlight-default\","
        "\"capturemode\":\"cyclic-polling\",\"inputdebouncetime\":200,\"pwmfrequency\":5000,\"logicactivelow\":false,\"exposetomqtt\":"
        "false,\"exposetorest\":false,\"smartled\":{\"type\":0,\"quantity\":1,\"colorredchannel\":255,\"colorgreenchannel\":255,"
        "\"colorbluechannel\":255},\"intensitycorrectionfactor\":100},{\"gpionumber\":12,\"gpiousage\":\"spare\",\"pinenabled\":false,"
        "\"pinname\":\"\",\"pinmode\":\"input\",\"capturemode\":\"cyclic-polling\",\"inputdebouncetime\":200,\"pwmfrequency\":5000,"
        "\"logicactivelow\":false,\"exposetomqtt\":false,\"exposetorest\":false,\"smartled\":{\"type\":0,\"quantity\":1,"
        "\"colorredchannel\":255,\"colorgreenchannel\":255,\"colorbluechannel\":255},\"intensitycorrectionfactor\":100},{\"gpionumber\":"
        "13,\"gpiousage\":\"spare\",\"pinenabled\":false,\"pinname\":\"\",\"pinmode\":\"input\",\"capturemode\":\"cyclic-polling\","
        "\"inputdebouncetime\":200,\"pwmfrequency\":5000,\"logicactivelow\":false,\"exposetomqtt\":false,\"exposetorest\":false,"
        "\"smartled\":{\"type\":0,\"quantity\":1,\"colorredchannel\":255,\"colorgreenchannel\":255,\"colorbluechannel\":255},"
        "\"intensitycorrectionfactor\":100}]},\"log\":{\"debug\":{\"loglevel\":2,\"logfilesretention\":5,\"debugfilesretention\":5},"
        "\"data\":{\"enabled\":false,\"datafilesretention\":30}},\"network\":{\"opmode\":0,\"timedoffdelay\":60,\"wlan\":{\"ssid\":\"\","
        "\"password\":\"\",\"hostname\":\"watermeter\",\"ipv4\":{\"networkconfig\":0,\"ipaddress\":\"\",\"subnetmask\":\"\","
        "\"gatewayaddress\":\"\",\"dnsserver\":\"\"},\"wlanroaming\":{\"enabled\":false,\"rssithreshold\":-75}},\"wlanap\":{\"ssid\":\"AI-"
        "on-the-Edge "
        "Device\",\"password\":\"\",\"channel\":11,\"ipv4\":{\"ipaddress\":\"192.168.4.1\"}},\"time\":{\"timezone\":\"CET-1CEST,M3.5.0,"
        "M10.5.0/"
        "3\",\"ntp\":{\"timesyncenabled\":true,\"timeserver\":\"\",\"processstartinterlock\":true}}},\"system\":{\"cpufrequency\":160},"
        "\"webui\":{\"httpauth\":{\"authmode\":0,\"username\":\"aiote\",\"password\":\"\"},\"autorefresh\":{\"overviewpage\":{"
        "\"enabled\":true,\"refreshtime\":5},\"datagraphpage\":{\"enabled\":false,\"refreshtime\":60}}}}";
    // std::string cfgDataExpexcted(config_json_default_expected_start, config_json_default_expected_end -
    // config_json_default_expected_start);
    ConfigClass::getInstance()->readConfigFile(true); // Use default config
    TEST_ASSERT_EQUAL_STRING(cfgDataExpexcted.c_str(), ConfigClass::getInstance()->getJsonBuffer());
}

/**
 * @brief test config handling
 */
void test_configHandling()
{
    test_configJsonParseAndSerialization();
}
