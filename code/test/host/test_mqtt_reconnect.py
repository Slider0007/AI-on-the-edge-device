"""Host regression: execute real MQTT functions, stubbing ESP/network boundaries only.
Run with Python 3 and a native C++17 compiler. This does not replace ESP-IDF/hardware tests.
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

CPP_PREFIX = r'''
#include <string>
#include <map>
#include <functional>
#include <vector>
#include <cstdio>
#define ESP_LOG_DEBUG 4
#define ESP_LOG_INFO 3
#define ESP_LOG_WARN 2
#define ESP_LOG_ERROR 1
#define MQTT_STATUS_TOPIC "/device/status/connection"
#define MQTT_STATUS_ONLINE "online"
const char *TAG="MQTT_IF";
struct {bool mqttEnabled=true,mqttInitialized=true,mqttConnected=true;int failedOnCycle=-1,mqttReconnectCnt=0;} mqttState;
int cycle=42; int getFlowCycleCounter(){return cycle;} int startMqttClient(){return 0;}
struct {void writeToFile(int,const char*,const std::string&) {}} LogFile;
struct Config {std::string mainTopic="watermeter";struct {bool discoveryEnabled=false;std::string statusTopic="homeassistant/status";} homeAssistant;};
Config cfg; Config *cfgDataPtr=&cfg;
std::map<std::string,std::function<void()>> *connectFunctionMap=nullptr;
using Handler=std::function<bool(std::string,char*,int)>;
std::map<std::string,Handler> subscriptions;std::map<std::string,Handler>*subscribeFunctionMap=&subscriptions;
bool mqtt_handler_flow_start(std::string,char*,int){return true;}
bool mqtt_handler_set_fallbackvalue(std::string,char*,int){return true;}
bool mqtt_handler_reboot(std::string,char*,int){return true;}
bool mqttServer_schedulePublishHADiscoveryFromMqtt(std::string,char*,int){return true;}
void registerMqttSubscribeFunction(std::string topic,Handler h){subscriptions[topic]=h;}
void *mqttClient=nullptr;
struct Publication {std::string topic,payload;int qos;bool retain;};
std::vector<Publication> sent;
int attempts=0;
bool failTransport=false;
int esp_mqtt_client_publish(void*,const char*topic,const char*payload,int,int qos,bool retain){++attempts;if(failTransport)return -1;sent.push_back({topic,payload,qos,retain});return 1;}
int esp_mqtt_client_subscribe(void*,const char*,int){return 1;}
'''
CPP_CASES = r'''
int failures=0,checks=0;
void check(bool ok,const char*message){++checks;if(!ok){++failures;printf("FAIL: %s\n",message);}}
void reset(){mqttState={true,true,true,-1,0};cycle=42;attempts=0;sent.clear();failTransport=false;subscriptions.clear();}
void checkBirth(){
 check(sent.size()==1,"reconnect must publish exactly one birth message");
 if(sent.size()==1){check(sent[0].topic=="watermeter/device/status/connection","birth must use the availability topic");check(sent[0].payload=="online","birth must be online");check(sent[0].qos==1,"birth must use QoS 1");check(sent[0].retain,"birth must be retained");}
}
int main(){
 reset();isConnectedState();checkBirth();
 reset();failTransport=true;
 check(!publishMqttData("watermeter/status","processing",1,false),"failed transport must report failure");
 check(attempts==2,"regular publish must retry once");
 check(mqttState.failedOnCycle==42,"transport failure must suppress this cycle");
 mqttState.mqttConnected=false;failTransport=false;mqttState.mqttConnected=true;
 isConnectedState();checkBirth();
 check(publishMqttData("watermeter/data","valid",1,true),"post-reconnect data publish must succeed");
 check(sent.size()==2,"successful reconnect must release the failed-cycle guard");
 reset();mqttState.failedOnCycle=42;failTransport=true;isConnectedState();
 check(attempts==2,"failed reconnect birth must still attempt and retry");
 check(sent.empty(),"failed birth must not record a successful delivery");
 check(mqttState.failedOnCycle==42,"failed birth must reinstate current-cycle suppression");
 publishMqttData("watermeter/data","valid",1,true);
 check(attempts==2,"failed birth must suppress subsequent same-cycle publishes");
 failTransport=false;++cycle;publishMqttData("watermeter/data","next-cycle",1,true);
 check(attempts==3 && sent.size()==1,"next cycle must recover even without a new reconnect");
 reset();failTransport=true;publishMqttData("watermeter/data","valid",1,true);failTransport=false;
 publishMqttData("watermeter/data","valid",1,true);
 check(attempts==2 && sent.empty(),"ordinary suppression must remain until reconnect or next cycle");
 reset();mqttState.mqttEnabled=false;
 check(!publishMqttData("watermeter/data","valid",1,true),"disabled MQTT must reject publishes");
 check(attempts==0,"disabled MQTT must not reach transport");
 reset();mqttState.mqttConnected=false;mqttState.failedOnCycle=42;isConnectedState();
 check(attempts==0 && mqttState.failedOnCycle==42,"disconnected callback must neither publish nor clear suppression");
 printf("%d checks, %d failures\n",checks,failures);return failures?1:0;
}
'''

def extract_function(source, signature):
    start = source.index(signature)
    position = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[position] == "{") - (source[position] == "}")
        position += 1
    return source[start:position]

class MqttReconnectTest(unittest.TestCase):
    def test_reconnect_publication_contract(self):
        root = Path(__file__).resolve().parents[3]
        source = (root / "code/components/mqtt_ctrl/interface_mqtt.cpp").read_text()
        actual = "\n".join(extract_function(source, signature) for signature in
                           ["bool publishMqttData(", "void isConnectedState(void)"])
        with tempfile.TemporaryDirectory(prefix="mqtt-reconnect-") as temporary:
            cpp = Path(temporary) / "test.cpp"
            binary = Path(temporary) / "test"
            cpp.write_text(CPP_PREFIX + "\n" + actual + "\n" + CPP_CASES)
            command = [os.environ.get("CXX", "g++"), *shlex.split(os.environ.get("CXXFLAGS", "")),
                       "-std=c++17", "-Wall", "-Wextra", str(cpp), "-o", str(binary)]
            built = subprocess.run(command, capture_output=True, text=True, timeout=60)
            self.assertEqual(built.returncode, 0, built.stdout + built.stderr)
            tested = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
            self.assertEqual(tested.returncode, 0, tested.stdout + tested.stderr)
            print(tested.stdout.strip())

if __name__ == "__main__":
    unittest.main()
