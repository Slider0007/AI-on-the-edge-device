#include "ClassFlow.h"
#include <fstream>
#include <string>
#include <iostream>
#include "esp_log.h"
#include "../../include/defines.h"

static const char *TAG = "CLASS";


void ClassFlow::SetInitialParameter(void)
{
	ListFlowControll = NULL;
	previousElement = NULL;	
	disabled = false;
}

bool ClassFlow::isNewParagraph(std::string input)
{
	if ((input[0] == '[') || ((input[0] == ';') && (input[1] == '[')))
	{
		return true;
	}
	return false;
}

bool ClassFlow::GetNextParagraph(FILE* pfile, std::string& aktparamgraph)
{
	while (getNextLine(pfile, &aktparamgraph) && !isNewParagraph(aktparamgraph));

	if (isNewParagraph(aktparamgraph))
		return true;
	return false;
}


ClassFlow::ClassFlow(void)
{
	SetInitialParameter();
}


ClassFlow::ClassFlow(std::vector<ClassFlow*> * lfc)
{
	SetInitialParameter();	
	ListFlowControll = lfc;
}


ClassFlow::ClassFlow(std::vector<ClassFlow*> * lfc, ClassFlow *_prev)
{
	SetInitialParameter();	
	ListFlowControll = lfc;
	previousElement = _prev;
}


void ClassFlow::PresetFlowStateHandler(bool _init, std::string _time)
{
    FlowState.ClassName = name();
	FlowState.ExecutionTime = _time;
    FlowState.isSuccessful = true;
	FlowState.onlyWarning = false;
    FlowState.ErrorCode = 0;

	if (_init) {
	    FlowState.getExecuted = false;
	}
	else {
    	FlowState.getExecuted = true;
	}
}


void ClassFlow::FlowStateHandlerSetError(int8_t _errorCode, bool _onlyWarning)
{	
	FlowState.isSuccessful = false;
	FlowState.onlyWarning = _onlyWarning;
	FlowState.ErrorCode = _errorCode;
}


struct strFlowState* ClassFlow::getFlowState()
{
	return &FlowState;
}


bool ClassFlow::ReadParameter(FILE* pfile, std::string &aktparamgraph)
{
	return false;
}


bool ClassFlow::doFlow(std::string time)
{
	return false;
}


void ClassFlow::doAutoErrorHandling()
{
	// Handled in derived classes
}


std::string ClassFlow::getHTMLSingleStep(std::string host)
{
	return "";
}


std::string ClassFlow::getReadout()
{
	return std::string();
}


std::string ClassFlow::GetParameterName(std::string _input)
{
    std::string _param;
    int _pospunkt = _input.find_first_of(".");
    if (_pospunkt > -1)
    {
        _param = _input.substr(_pospunkt+1, _input.length() - _pospunkt - 1);
    }
    else
    {
        _param = _input;
    }
//    ESP_LOGD(TAG, "Parameter: %s, Pospunkt: %d", _param.c_str(), _pospunkt);
	return _param;
}


bool ClassFlow::getNextLine(FILE* pfile, std::string *rt)
{
	char zw[256];
	if (pfile == NULL)
	{
		*rt = "";
		return false;
	}
	if (!fgets(zw, sizeof(zw), pfile))
	{
		*rt = "";
		ESP_LOGD(TAG, "END OF FILE");
		return false;
	}
	ESP_LOGD(TAG, "%s", zw);
	*rt = zw;
	*rt = trim(*rt);
	while ((zw[0] == '#' || (rt->size() == 0)) && !(zw[1] == '['))
	{
		*rt = "";
		if (!fgets(zw, sizeof(zw), pfile))
			return false;
		ESP_LOGD(TAG, "%s", zw);
		*rt = zw;
		*rt = trim(*rt);
	}
	return true;
}
