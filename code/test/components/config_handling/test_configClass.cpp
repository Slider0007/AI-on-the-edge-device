#include "configClass.h"

static const char *TAG_CFGTEST = "CFGTEST";

/**
 * @brief test JSON parse and serialization
 */
void test_configJsonParseAndSerialization()  //@TODO
{
    // Be aware: The results depends on the device for which it's compiled.
    // Set to default (ESP32CAM device)
    std::string cfgData =
        "{\"config\":{\"version\":3,\"lastmodified\":\"\"},\"operationmode\":{"
        "\"opmode\":-1,\"automaticprocessinterval\":\"1.0\",\"usedemoimages\":"
        "false},\"takeimage\":{\"flashlight\":{\"flashtime\":2000,"
        "\"flashintensity\":50},\"camera\":{\"camerafrequency\":20,"
        "\"imagequality\":12,\"imagesize\":\"VGA\",\"brightness\":0,\"contrast\":"
        "0,\"saturation\":0,\"sharpness\":0,\"exposurecontrolmode\":1,"
        "\"autoexposurelevel\":0,\"manualexposurevalue\":300,\"gaincontrolmode\":"
        "1,\"manualgainvalue\":0,\"specialeffect\":0,\"mirrorimage\":false,"
        "\"flipimage\":false,\"zoommode\":0,\"zoomoffsetx\":0,\"zoomoffsety\":0},"
        "\"debug\":{\"saverawimages\":false,\"rawimageslocation\":\"/log/"
        "source\",\"rawimagesretention\":5}},\"imagealignment\":{"
        "\"alignmentalgo\":0,\"searchfield\":{\"x\":20,\"y\":20},"
        "\"imagerotation\":\"0.0\",\"flipimagesize\":false,\"marker\":[{\"x\":1,"
        "\"y\":1},{\"x\":1,\"y\":1}],\"debug\":{\"savedebuginfo\":false}},"
        "\"numbersequences\":{\"sequence\":[{\"sequenceid\":0,\"sequencename\":"
        "\"main\"}]},\"digit\":{\"enabled\":true,\"model\":\"dig-class100-0168_"
        "s2_q.tflite\",\"cnngoodthreshold\":\"0.00\",\"sequence\":[{"
        "\"sequenceid\":0,\"sequencename\":\"main\",\"roi\":[]}],\"debug\":{"
        "\"saveroiimages\":false,\"roiimageslocation\":\"/log/"
        "digit\",\"roiimagesretention\":3}},\"analog\":{\"enabled\":false,"
        "\"model\":\"ana-class100_0171_s1_q.tflite\",\"sequence\":[{"
        "\"sequenceid\":0,\"sequencename\":\"main\",\"roi\":[]}],\"debug\":{"
        "\"saveroiimages\":false,\"roiimageslocation\":\"/log/"
        "analog\",\"roiimagesretention\":3}},\"postprocessing\":{\"enabled\":"
        "true,\"usefallbackvalue\":true,\"fallbackvalueagestartup\":720,"
        "\"sequence\":[{\"sequenceid\":0,\"sequencename\":\"main\","
        "\"decimalshift\":0,\"analogdigitsyncvalue\":\"9.2\","
        "\"extendedresolution\":true,\"allownegativerate\":false,"
        "\"maxratechecktype\":1,\"maxrate\":\"0.15\",\"ignoreleadingnan\":false,"
        "\"checkdigitincreaseconsistency\":false}],\"debug\":{\"savedebuginfo\":"
        "false}},\"mqtt\":{\"enabled\":false,\"uri\":\"\",\"maintopic\":"
        "\"watermeter\",\"clientid\":\"watermeter\",\"authmode\":0,\"username\":"
        "\"\",\"password\":\"\",\"tls\":{\"cacert\":\"\",\"clientcert\":\"\","
        "\"clientkey\":\"\"},\"processdatanotation\":0,\"retainprocessdata\":false,"
        "\"homeassistant\":{\"discoveryenabled\":false,\"discoveryprefix\":"
        "\"homeassistant\",\"statustopic\":\"homeassistant/"
        "status\",\"meterType\":1,\"retaindiscovery\":false}},\"influxdbv1\":{"
        "\"enabled\":false,\"uri\":\"\",\"database\":\"\",\"authmode\":0,"
        "\"username\":\"\",\"password\":\"\",\"tls\":{\"cacert\":\"\","
        "\"clientcert\":\"\",\"clientkey\":\"\"},\"sequence\":[{\"sequenceid\":0,"
        "\"sequencename\":\"main\",\"measurementname\":\"\",\"fieldname\":\"\"}]}"
        ",\"influxdbv2\":{\"enabled\":false,\"uri\":\"\",\"bucket\":\"\","
        "\"authmode\":0,\"organization\":\"\",\"token\":\"\",\"tls\":{\"cacert\":"
        "\"\",\"clientcert\":\"\",\"clientkey\":\"\"},\"sequence\":[{"
        "\"sequenceid\":0,\"sequencename\":\"main\",\"measurementname\":\"\","
        "\"fieldname\":\"\"}]},\"gpio\":{\"customizationenabled\":false,"
        "\"gpiopin\":[{\"gpionumber\":1,\"gpiousage\":\"restricted: "
        "uart0-tx\",\"pinenabled\":false,\"pinname\":\"\",\"pinmode\":\"input\","
        "\"capturemode\":\"cyclic-polling\",\"inputdebouncetime\":200,"
        "\"pwmfrequency\":5000,\"exposetomqtt\":false,\"exposetorest\":false,"
        "\"smartled\":{\"type\":0,\"quantity\":1,\"colorredchannel\":255,"
        "\"colorgreenchannel\":255,\"colorbluechannel\":255},"
        "\"intensitycorrectionfactor\":100},{\"gpionumber\":3,\"gpiousage\":"
        "\"restricted: "
        "uart0-rx\",\"pinenabled\":false,\"pinname\":\"\",\"pinmode\":\"input\","
        "\"capturemode\":\"cyclic-polling\",\"inputdebouncetime\":200,"
        "\"pwmfrequency\":5000,\"exposetomqtt\":false,\"exposetorest\":false,"
        "\"smartled\":{\"type\":0,\"quantity\":1,\"colorredchannel\":255,"
        "\"colorgreenchannel\":255,\"colorbluechannel\":255},"
        "\"intensitycorrectionfactor\":100},{\"gpionumber\":4,\"gpiousage\":"
        "\"flashlight-pwm\",\"pinenabled\":false,\"pinname\":\"\",\"pinmode\":"
        "\"flashlight-default\",\"capturemode\":\"cyclic-polling\","
        "\"inputdebouncetime\":200,\"pwmfrequency\":5000,\"exposetomqtt\":false,"
        "\"exposetorest\":false,\"smartled\":{\"type\":0,\"quantity\":1,"
        "\"colorredchannel\":255,\"colorgreenchannel\":255,\"colorbluechannel\":"
        "255},\"intensitycorrectionfactor\":100},{\"gpionumber\":12,"
        "\"gpiousage\":\"spare\",\"pinenabled\":false,\"pinname\":\"\","
        "\"pinmode\":\"input\",\"capturemode\":\"cyclic-polling\","
        "\"inputdebouncetime\":200,\"pwmfrequency\":5000,\"exposetomqtt\":false,"
        "\"exposetorest\":false,\"smartled\":{\"type\":0,\"quantity\":1,"
        "\"colorredchannel\":255,\"colorgreenchannel\":255,\"colorbluechannel\":"
        "255},\"intensitycorrectionfactor\":100},{\"gpionumber\":13,"
        "\"gpiousage\":\"spare\",\"pinenabled\":false,\"pinname\":\"\","
        "\"pinmode\":\"input\",\"capturemode\":\"cyclic-polling\","
        "\"inputdebouncetime\":200,\"pwmfrequency\":5000,\"exposetomqtt\":false,"
        "\"exposetorest\":false,\"smartled\":{\"type\":0,\"quantity\":1,"
        "\"colorredchannel\":255,\"colorgreenchannel\":255,\"colorbluechannel\":"
        "255},\"intensitycorrectionfactor\":100}]},\"log\":{\"debug\":{"
        "\"loglevel\":2,\"logfilesretention\":5,\"debugfilesretention\":5},"
        "\"data\":{\"enabled\":false,\"datafilesretention\":30}},\"system\":{"
        "\"time\":{\"timesyncenabled\":true,\"timeserver\":\"\",\"timezone\":"
        "\"CET-1CEST,M3.5.0,M10.5.0/"
        "3\"},\"hostname\":\"watermeter\",\"wlanroaming\":{\"enabled\":false,"
        "\"rssithreshold\":-75},\"cpufrequency\":160},\"webui\":{\"autorefresh\":"
        "{\"overviewpage\":{\"enabled\":true,\"refreshtime\":5},"
        "\"datagraphpage\":{\"enabled\":false,\"refreshtime\":60}}}}";

    ConfigClass::getInstance()->readConfigFile(true, cfgData);
    TEST_ASSERT_EQUAL_STRING(cfgData.c_str(), ConfigClass::getInstance()->getJsonBuffer());

    // Check low plausibilty boundery

    // Check high plausibilty boundery

    // Special palusibility checks
}

/**
 * @brief test config handling
 */
void test_configHandling() { test_configJsonParseAndSerialization(); }