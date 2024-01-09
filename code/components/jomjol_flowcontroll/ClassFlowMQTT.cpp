#include "ClassFlowMQTT.h"

#ifdef ENABLE_MQTT
#include <sstream>
#include <iomanip>
#include <time.h>

#include "Helper.h"
#include "connect_wlan.h"
#include "read_wlanini.h"
#include "time_sntp.h"
#include "interface_mqtt.h"
#include "server_mqtt.h"
#include "ClassLogFile.h"
#include "ClassFlowControll.h"


static const char *TAG = "MQTT";

extern const char* libfive_git_version(void);
extern const char* libfive_git_revision(void);
extern const char* libfive_git_branch(void);


void ClassFlowMQTT::SetInitialParameter(void)
{
    presetFlowStateHandler(true);
    flowpostprocessing = NULL; 
    previousElement = NULL;
    ListFlowControll = NULL; 
    disabled = false;

    uri = "";
    maintopic = wlan_config.hostname;
    clientname = wlan_config.hostname;
    user = "";
    password = "";
    TLSEncryption = false;
    TLSCACertFilename = "";
    TLSClientCertFilename = "";
    TLSClientKeyFilename = "";
    SetRetainFlag = false;
    
    keepAlive = 25*60; 
}       

ClassFlowMQTT::ClassFlowMQTT()
{
    SetInitialParameter();
}

ClassFlowMQTT::ClassFlowMQTT(std::vector<ClassFlow*>* lfc)
{
    SetInitialParameter();

    ListFlowControll = lfc;
    for (int i = 0; i < ListFlowControll->size(); ++i)
    {
        if (((*ListFlowControll)[i])->name().compare("ClassFlowPostProcessing") == 0)
        {
            flowpostprocessing = (ClassFlowPostProcessing*) (*ListFlowControll)[i];
        }
    }
}

ClassFlowMQTT::ClassFlowMQTT(std::vector<ClassFlow*>* lfc, ClassFlow *_prev)
{
    SetInitialParameter();

    previousElement = _prev;
    ListFlowControll = lfc;

    for (int i = 0; i < ListFlowControll->size(); ++i)
    {
        if (((*ListFlowControll)[i])->name().compare("ClassFlowPostProcessing") == 0)
        {
            flowpostprocessing = (ClassFlowPostProcessing*) (*ListFlowControll)[i];
        }
    }
}


bool ClassFlowMQTT::ReadParameter(FILE* pfile, std::string& aktparamgraph)
{
    std::vector<std::string> splitted;

    aktparamgraph = trim(aktparamgraph);

    if (aktparamgraph.size() == 0)
        if (!GetNextParagraph(pfile, aktparamgraph))
            return false;

    if (toUpper(aktparamgraph).compare("[MQTT]") != 0)       // Paragraph does not fit MQTT
        return false;

    while (getNextLine(pfile, &aktparamgraph) && !isNewParagraph(aktparamgraph))
    {
        splitted = ZerlegeZeile(aktparamgraph);
        if ((toUpper(splitted[0]) == "URI") && (splitted.size() > 1))
        {
            uri = splitted[1];
        }

        if (((toUpper(splitted[0]) == "TOPIC") || (toUpper(splitted[0]) == "MAINTOPIC")) && (splitted.size() > 1))
        {
            maintopic = splitted[1];
        }

        if ((toUpper(splitted[0]) == "CLIENTID") && (splitted.size() > 1))
        {
            clientname = splitted[1];
        }

        if ((toUpper(splitted[0]) == "USER") && (splitted.size() > 1))
        {
            user = splitted[1];
        }

        if ((toUpper(splitted[0]) == "PASSWORD") && (splitted.size() > 1))
        {
            password = splitted[1];
        }   

        if ((toUpper(splitted[0]) == "TLSENCRYPTION") && (splitted.size() > 1))
        {
            if (toUpper(splitted[1]) == "TRUE")
                TLSEncryption = true;  
            else
                TLSEncryption = false;
        }

        if ((toUpper(splitted[0]) == "TLSCACERT") && (splitted.size() > 1))
        {
            TLSCACertFilename = "/sdcard" + splitted[1];
        }
        
        if ((toUpper(splitted[0]) == "TLSCLIENTCERT") && (splitted.size() > 1))
        {
            TLSClientCertFilename = "/sdcard" + splitted[1];
        }

        if ((toUpper(splitted[0]) == "TLSCLIENTKEY") && (splitted.size() > 1))
        {
            TLSClientKeyFilename = "/sdcard" + splitted[1];
        }      

        if ((toUpper(splitted[0]) == "RETAINMESSAGES") && (splitted.size() > 1))
        {
            if (toUpper(splitted[1]) == "TRUE")
                SetRetainFlag = true;  
            else
                SetRetainFlag = false;

            setMqtt_Server_Retain(SetRetainFlag);
        }

        if ((toUpper(splitted[0]) == "HOMEASSISTANTDISCOVERY") && (splitted.size() > 1))
        {
            if (toUpper(splitted[1]) == "TRUE")
                SetHomeassistantDiscoveryEnabled(true);
            else
                SetHomeassistantDiscoveryEnabled(false);
        }
        
        if ((toUpper(splitted[0]) == "METERTYPE") && (splitted.size() > 1)) {
        /* Use meter type for the device class 
           Make sure it is a listed one on https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes */
            if (toUpper(splitted[1]) == "WATER_M3") {
                mqttServer_setMeterType("water", "m³", "h", "m³/h");
            }
            else if (toUpper(splitted[1]) == "WATER_L") {
                mqttServer_setMeterType("water", "L", "h", "L/h");
            }
            else if (toUpper(splitted[1]) == "WATER_FT3") {
                mqttServer_setMeterType("water", "ft³", "m", "ft³/m"); // Minutes
            }
            else if (toUpper(splitted[1]) == "WATER_GAL") {
                mqttServer_setMeterType("water", "gal", "h", "gal/h");
            }
            else if (toUpper(splitted[1]) == "GAS_M3") {
                mqttServer_setMeterType("gas", "m³", "h", "m³/h");
            }
            else if (toUpper(splitted[1]) == "GAS_FT3") {
                mqttServer_setMeterType("gas", "ft³", "m", "ft³/m"); // Minutes
            }
            else if (toUpper(splitted[1]) == "ENERGY_WH") {
                mqttServer_setMeterType("energy", "Wh", "h", "W");
            }
            else if (toUpper(splitted[1]) == "ENERGY_KWH") {
                mqttServer_setMeterType("energy", "kWh", "h", "kW");
            }
            else if (toUpper(splitted[1]) == "ENERGY_MWH") {
                mqttServer_setMeterType("energy", "MWh", "h", "MW");
            }
            else if (toUpper(splitted[1]) == "ENERGY_GJ") {
                mqttServer_setMeterType("energy", "GJ", "h", "GJ/h");
            }
        }
    }
      
    scheduleSendingStaticTopics();
    mqttServer_setMainTopic(maintopic);

    /* Note:
     * Originally, we started the MQTT client here.
     * How ever we need the interval parameter from the ClassFlowControll, but that only gets started later.
     * To work around this, we delay the start and trigger it from ClassFlowControll::ReadParameter() */

    return true;
}


bool ClassFlowMQTT::Start(float _processingInterval) 
{
    keepAlive = _processingInterval * 60 * 2.5; // Seconds, make sure it is greater than 2 processing cycles!

    std::stringstream stream;
    stream << std::fixed << std::setprecision(1) << "Processing interval: " << _processingInterval <<
            "min -> MQTT LWT timeout: " << ((float)keepAlive/60) << "min";
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, stream.str());

    mqttServer_setParameter(flowpostprocessing->GetNumbers(), keepAlive, _processingInterval);

    bool MQTTConfigCheck = MQTT_Configure(uri, clientname, user, password, maintopic, LWT_TOPIC, LWT_CONNECTED,
                                            LWT_DISCONNECTED, TLSEncryption, TLSCACertFilename, TLSClientCertFilename,
                                            TLSClientKeyFilename, keepAlive, SetRetainFlag, (void *)&GotConnected);

    if (!MQTTConfigCheck) {
        return false;
    }

    return (MQTT_Init() == 1);
}


bool ClassFlowMQTT::doFlow(std::string zwtime)
{
    presetFlowStateHandler(false, zwtime);
    bool success;
    std::string namenumber = "";
    int qos = 1;

    /* Send static topics and if selected the the HA discovery topics as well*/
    sendDiscovery_and_static_Topics();

    success = publishSystemData(qos);

    if (flowpostprocessing && getMQTTisConnected())
    {
        std::vector<NumberPost*>* NUMBERS = flowpostprocessing->GetNumbers();

        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Publishing MQTT topics");

        for (int i = 0; i < (*NUMBERS).size(); ++i) {
            namenumber = (*NUMBERS)[i]->name;
            if (namenumber == "default")
                namenumber = maintopic + "/";
            else
                namenumber = maintopic + "/" + namenumber + "/";

            success |= MQTTPublish(namenumber + "actual_value", (*NUMBERS)[i]->sActualValue, qos, SetRetainFlag);
            success |= MQTTPublish(namenumber + "fallback_value", (*NUMBERS)[i]->sFallbackValue, qos, SetRetainFlag);
            success |= MQTTPublish(namenumber + "raw_value", (*NUMBERS)[i]->sRawValue, qos, SetRetainFlag);
            success |= MQTTPublish(namenumber + "value_status", (*NUMBERS)[i]->sValueStatus, qos, SetRetainFlag);
            success |= MQTTPublish(namenumber + "rate_per_min", (*NUMBERS)[i]->sRatePerMin, qos, SetRetainFlag);
            success |= MQTTPublish(namenumber + "rate_per_processing", (*NUMBERS)[i]->sRatePerProcessing, qos, SetRetainFlag);

            if (getTimeUnit() == "h") { // Rate per hour
                success |= MQTTPublish(namenumber + "rate_per_time_unit", std::to_string((*NUMBERS)[i]->ratePerMin * 60), qos, SetRetainFlag); // per minutes => per hour
            }
            else { // Default: Rate per minute
                success |= MQTTPublish(namenumber + "rate_per_time_unit", (*NUMBERS)[i]->sRatePerMin, qos, SetRetainFlag); // rate per minutes
            }
            success |= MQTTPublish(namenumber + "timestamp_processed", (*NUMBERS)[i]->sTimeProcessed, qos, SetRetainFlag);
            
            std::string json = flowpostprocessing->getJsonFromNumber(i, "\n");
            success |= MQTTPublish(namenumber + "json", json, qos, SetRetainFlag);
        }
    }

    if (!success) {
        LogFile.WriteToFile(ESP_LOG_WARN, TAG, "Failed to publish one or more topics (system / result)");
    }
    
    return true;
}


void ClassFlowMQTT::doPostProcessEventHandling()
{
    // Post cycle process handling can be included here. Function is called after processing cycle is completed
    
}


ClassFlowMQTT::~ClassFlowMQTT()
{
    MQTTdestroy_client(true);
}

#endif //ENABLE_MQTT