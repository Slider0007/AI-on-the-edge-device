#include "ClassFlow.h"
#include "../../include/defines.h"

#include <esp_log.h>


static const char *TAG = "CLASSFLOW";


ClassFlow::ClassFlow(void)
{
	// Handled in derived classes
}


ClassFlow::ClassFlow(void *)
{
	// Handled in derived classes
}


bool ClassFlow::loadParameter()
{
	// Handled in derived classes
	return true;
}


bool ClassFlow::doFlow(std::string time)
{
	// Handled in derived classes
	return true;
}


void ClassFlow::doPostProcessEventHandling()
{
	// Handled in derived classes
}


void ClassFlow::presetFlowStateHandler(bool _init, std::string _time)
{
    FlowState.ClassName = name();
	FlowState.ExecutionTime = _time;
    FlowState.isSuccessful = true;
	FlowState.EventCode.clear();
	FlowState.EventCode.shrink_to_fit();


	if (_init)
	    FlowState.getExecuted = false;
	else
    	FlowState.getExecuted = true;
}


void ClassFlow::setFlowStateHandlerEvent(int _eventCode)
{
	FlowState.isSuccessful = false;
	FlowState.EventCode.push_back(_eventCode); // negative event code -> error; positive event code -> warning
}


struct strFlowState* ClassFlow::getFlowState()
{
	return &FlowState;
}
