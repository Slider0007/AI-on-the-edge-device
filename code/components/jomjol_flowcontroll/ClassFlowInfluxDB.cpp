#ifdef ENABLE_INFLUXDB
#include <sstream>
#include "ClassFlowInfluxDB.h"
#include "Helper.h"
#include "connect_wlan.h"

#include "time_sntp.h"
#include "interface_influxdb.h"

#include "ClassFlowPostProcessing.h"
#include "esp_log.h"
#include "../../include/defines.h"

#include "ClassLogFile.h"

#include <time.h>

static const char* TAG = "INFLUXDB";


void ClassFlowInfluxDB::SetInitialParameter(void)
{
    PresetFlowStateHandler(true);
    flowpostprocessing = NULL;  
    previousElement = NULL;
    ListFlowControll = NULL;

    uri = "";
    database = "";
    measurement = "";
    user = "";
    password = "";

    disabled = false;
    InfluxDBenable = false;
}       


ClassFlowInfluxDB::ClassFlowInfluxDB()
{
    SetInitialParameter();
}


ClassFlowInfluxDB::ClassFlowInfluxDB(std::vector<ClassFlow*>* lfc)
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


ClassFlowInfluxDB::ClassFlowInfluxDB(std::vector<ClassFlow*>* lfc, ClassFlow *_prev)
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


bool ClassFlowInfluxDB::ReadParameter(FILE* pfile, std::string& aktparamgraph)
{
    std::vector<std::string> splitted;
    std::string _param;

    aktparamgraph = trim(aktparamgraph);

    if (aktparamgraph.size() == 0)
        if (!this->GetNextParagraph(pfile, aktparamgraph))
            return false;

    if (toUpper(aktparamgraph).compare("[INFLUXDB]") != 0) 
        return false;

    while (this->getNextLine(pfile, &aktparamgraph) && !this->isNewParagraph(aktparamgraph))
    {
        //ESP_LOGD(TAG, "while loop reading line: %s", aktparamgraph.c_str());
        splitted = ZerlegeZeile(aktparamgraph);
        _param = GetParameterName(splitted[0]);

        if ((toUpper(_param) == "URI") && (splitted.size() > 1))
        {
            this->uri = splitted[1];
        }

        if (((toUpper(_param) == "DATABASE")) && (splitted.size() > 1))
        {
            this->database = splitted[1];
        }

        if ((toUpper(_param) == "USER") && (splitted.size() > 1))
        {
            this->user = splitted[1];
        } 

        if ((toUpper(_param) == "PASSWORD") && (splitted.size() > 1))
        {
            this->password = splitted[1];
        }               

        if (((toUpper(_param) == "MEASUREMENT")) && (splitted.size() > 1))
        {
            handleMeasurement(splitted[0], splitted[1]);
        }
        
        if (((toUpper(_param) == "FIELD")) && (splitted.size() > 1))
        {
            handleFieldname(splitted[0], splitted[1]);
        }
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Init with URI: " + uri + ", Database: " + database + ", User: " + user + ", Password: " + password);

    if ((uri.length() > 0) && (uri != "undefined") && (database.length() > 0) && (database != "undefined")) { 
        InfluxDBInit(uri, database, user, password); 
        InfluxDBenable = true;
    } 
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Init failed, missing or wrong parameter");
        return false;
    }
   
    return true;
}


bool ClassFlowInfluxDB::doFlow(std::string zwtime)
{
    if (!InfluxDBenable)
        return true;

    PresetFlowStateHandler();

    if (flowpostprocessing != NULL) {
        std::vector<NumberPost*>* NUMBERS = flowpostprocessing->GetNumbers();
        std::string namenumber = "";

        for (int i = 0; i < (*NUMBERS).size(); ++i) {
            if ((*NUMBERS)[i]->isActualValueANumber) {
                if ((*NUMBERS)[i]->FieldV1.length() > 0) {
                    namenumber = (*NUMBERS)[i]->FieldV1;
                }
                else {
                    namenumber = (*NUMBERS)[i]->name;
                    if (namenumber == "default")
                        namenumber = "value";
                    else
                        namenumber = namenumber + "/value";
                }

                InfluxDBPublish((*NUMBERS)[i]->MeasurementV1, namenumber, (*NUMBERS)[i]->sActualValue, (*NUMBERS)[i]->sTimeProcessed);
            }
        }
    }
    else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to read post-processing data");
        return false;
    }
      
    return true;
}


void ClassFlowInfluxDB::handleMeasurement(std::string _decsep, std::string _value)
{
    std::string _digit, _decpos;
    int _pospunkt = _decsep.find_first_of(".");

    if (_pospunkt > -1)
        _digit = _decsep.substr(0, _pospunkt);
    else
        _digit = "default";
    
    for (int j = 0; j < flowpostprocessing->GetNumbers()->size(); ++j) {
        if (_digit == "default" || (*flowpostprocessing->GetNumbers())[j]->name == _digit)
            (*flowpostprocessing->GetNumbers())[j]->MeasurementV1 = _value;

        //ESP_LOGI(TAG, "handleMeasurement: Name: %s, Pospunkt: %d, value: %s", _digit.c_str(), _pospunkt, _value);
    }
}


void ClassFlowInfluxDB::handleFieldname(std::string _decsep, std::string _value)
{
    std::string _digit, _decpos;
    int _pospunkt = _decsep.find_first_of(".");

    if (_pospunkt > -1)
        _digit = _decsep.substr(0, _pospunkt);
    else
        _digit = "default";
    
    for (int j = 0; j < flowpostprocessing->GetNumbers()->size(); ++j) {
        if (_digit == "default" || (*flowpostprocessing->GetNumbers())[j]->name == _digit)
            (*flowpostprocessing->GetNumbers())[j]->FieldV1 = _value;
        
        //ESP_LOGI(TAG, "handleFieldname: Name: %s, Pospunkt: %d, value: %s", _digit.c_str(), _pospunkt, _value);
    }
}


ClassFlowInfluxDB::~ClassFlowInfluxDB()
{
    // nothing to do
}

#endif //ENABLE_INFLUXDB