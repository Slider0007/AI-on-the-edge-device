#ifndef CLASSFLOWCONTROL_H
#define CLASSFLOWCONTROL_H

#include <string>

#include "configClass.h"
#include "ClassFlow.h"
#include "ClassFlowDefineTypes.h"
#include "ClassFlowTakeImage.h"
#include "ClassFlowAlignment.h"
#include "ClassFlowCNNGeneral.h"
#include "ClassFlowPostProcessing.h"

#ifdef ENABLE_MQTT
    #include "ClassFlowMQTT.h"
#endif // ENABLE_MQTT

#ifdef ENABLE_INFLUXDB
    #include "ClassFlowInfluxDBv1.h"
    #include "ClassFlowInfluxDBv2.h"
#endif // ENABLE_INFLUXDB


enum flowStateEvent {
    MULTIPLE_ERROR_IN_ROW = -2,
    SINGLE_ERROR = -1,
    NONE,
    SINGLE_DEVIATION,
    MULTIPLE_DEVIATION_IN_ROW
};


class ClassFlowControl : public ClassFlow
{
  protected:
    ConfigClass *cfgClassPtr;
    std::vector<ClassFlow *> FlowControlImage;
    std::vector<ClassFlow *> FlowControlPublish;
    std::vector<strFlowState *> FlowStateEvaluationEvent;
    std::vector<strFlowState *> FlowStatePublishEvent;

    ClassFlowTakeImage *flowtakeimage;
    ClassFlowAlignment *flowalignment;
    ClassFlowCNNGeneral *flowanalog;
    ClassFlowCNNGeneral *flowdigit;
    ClassFlowPostProcessing *flowpostprocessing;
#ifdef ENABLE_MQTT
    ClassFlowMQTT *flowMQTT;
#endif // ENABLE_MQTT
#ifdef ENABLE_INFLUXDB
    ClassFlowInfluxDBv1 *flowInfluxDBv1;
    ClassFlowInfluxDBv2 *flowInfluxDBv2;
#endif // ENABLE_INFLUXDB

    std::string actualProcessState;
    std::string actualProcessStateWithTime;
    int flowStateErrorInRow;
    int flowStateDeviationInRow;

  public:
    ClassFlowControl();
    virtual ~ClassFlowControl();

    bool initFlow();
    void deinitFlow(void);

    bool loadParameter();
    bool doFlowImageEvaluation(std::string time);
    bool doFlowPublishData(std::string time);

    bool getStatusSetupModus();
    float getProcessInterval();
    bool isAutoStart();
    bool isAutoStart(long &_interval);

    void setActualProcessState(std::string _actualProcessState);
    std::string getActualProcessState();
    std::string getActualProcessStateWithTime();
    std::string translateActualProcessState(std::string classname);

    void setFlowStateError();
    void clearFlowStateEventInRowCounter();
    int getFlowStateErrorOrDeviation();
    bool flowStateEventOccured();
    void postProcessEventHandler();

    void drawDigitRoi(CImageBasis *image);
    void drawAnalogRoi(CImageBasis *image);

    #ifdef ENABLE_MQTT
    bool initMqttService();
    #endif // ENABLE_MQTT

    const std::vector<SequenceData *> &getSequenceData() const { return sequenceData; };
    std::string getSequenceResultInline(int type, std::string sequenceName = "");

    bool setFallbackValue(std::string _sequenceName, std::string _newvalue);
    std::string getFallbackValue(std::string _sequenceName);

    CImageBasis *getRawImage();
    esp_err_t getJPGStream(std::string _fn, httpd_req_t *req);
    esp_err_t sendRawJPG(httpd_req_t *req);

    std::string name() { return "ClassFlowControl"; };

    // Only used for unity testing
    std::vector<SequenceData *> &getSequenceData() { return sequenceData; };
};

#endif // CLASSFLOWCONTROL_H
