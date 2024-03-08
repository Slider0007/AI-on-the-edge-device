#ifndef CLASSFLOWCONTROLL_H
#define CLASSFLOWCONTROLL_H

#include <string>

#include "ClassFlow.h"
#include "ClassFlowDefineTypes.h"
#include "ClassFlowTakeImage.h"
#include "ClassFlowAlignment.h"
#include "ClassFlowCNNGeneral.h"
#include "ClassFlowPostProcessing.h"

#ifdef ENABLE_MQTT
#include "ClassFlowMQTT.h"
#endif //ENABLE_MQTT

#ifdef ENABLE_INFLUXDB
#include "ClassFlowInfluxDB.h"
#include "ClassFlowInfluxDBv2.h"
#endif //ENABLE_INFLUXDB


enum flowStateEvent {
	MULTIPLE_ERROR_IN_ROW = -2,
	SINGLE_ERROR = -1,
	NONE,
	SINGLE_DEVIATION,
	MULTIPLE_DEVIATION_IN_ROW
};


class ClassFlowControll : public ClassFlow
{
	protected:
		std::vector<ClassFlow*> FlowControll;
		std::vector<ClassFlow*> FlowControlPublish;
		std::vector<strFlowState*> FlowStateEvaluationEvent;
		std::vector<strFlowState*> FlowStatePublishEvent;

		ClassFlowTakeImage* flowtakeimage;
		ClassFlowAlignment* flowalignment;	
		ClassFlowCNNGeneral* flowanalog;
		ClassFlowCNNGeneral* flowdigit;
		ClassFlowPostProcessing* flowpostprocessing;
		#ifdef ENABLE_MQTT
		ClassFlowMQTT* flowMQTT;
		#endif //ENABLE_MQTT
		#ifdef ENABLE_INFLUXDB
		ClassFlowInfluxDB* flowInfluxDB;
		ClassFlowInfluxDBv2* flowInfluxDBv2;
		#endif //ENABLE_INFLUXDB
		
		ClassFlow* CreateClassFlow(std::string _type);
		void SetInitialParameter(void);	

		float AutoInterval;
		bool AutoStart;
		bool SetupModeActive;
		bool readParameterDone;
		
		std::string aktstatus;
		std::string aktstatusWithTime;
		int flowStateErrorInRow;
		int flowStateDeviationInRow;

public:
	ClassFlowControll();
	virtual ~ClassFlowControll();
	bool InitFlow(std::string config);
	void DeinitFlow(void);
	ClassFlow* getFlowClass(std::string _classname);

	bool ReadParameter(FILE* pfile, std::string& aktparamgraph);	
	bool doFlowImageEvaluation(std::string time);
	bool doFlowPublishData(std::string time);
	bool doFlowTakeImageOnly(std::string time);
	
	std::string TranslateAktstatus(std::string _input);
	bool getStatusSetupModus() {return SetupModeActive;};

	std::string getNumbersName();
	std::string getNumbersName(int _number);
	int getNumbersSize();
	int getNumbersROISize(int _seqNo, int _filter);
	int getNumbersNamePosition(std::string _name);
	std::string getNumbersValue(std::string _name, int _type);
	std::string getNumbersValue(int _position, int _type);
	std::string getReadoutAll(int _type);
	std::string getReadout(bool _rawvalue, bool _noerror, int _number);

	bool UpdateFallbackValue(std::string _newvalue, std::string _numbers);
	std::string GetFallbackValue(std::string _number = "");	

	#ifdef ENABLE_MQTT
	bool StartMQTTService();
	#endif //ENABLE_MQTT

	void DigitalDrawROI(CImageBasis *_zw);
	void AnalogDrawROI(CImageBasis *_zw);

	esp_err_t GetJPGStream(std::string _fn, httpd_req_t *req);
	esp_err_t SendRawJPG(httpd_req_t *req);

	std::string doSingleStep(std::string _stepname, std::string _host);

	float getProcessInterval();
	bool isAutoStart();
	bool isAutoStart(long &_interval);

	std::string getActStatusWithTime();
	std::string getActStatus();
	void setActStatus(std::string _aktstatus);
	void setFlowStateError();
	void clearFlowStateEventInRowCounter();
	int getFlowStateErrorOrDeviation();
	bool FlowStateEventOccured();
	void PostProcessEventHandler();

	std::vector<HTMLInfo*> GetAllDigital();
	std::vector<HTMLInfo*> GetAllAnalog();	

	t_CNNType GetTypeDigital();
	t_CNNType GetTypeAnalog();

	int CleanTempFolder();

	CImageBasis* getRawImage();

	std::string name() {return "ClassFlowControll";};
};

#endif //CLASSFLOWCONTROLL_H



