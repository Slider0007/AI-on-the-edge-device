# MQTT reconnect host regression

Run from the repository root with Python 3 and a native C++17 compiler:

```sh
python3 -m unittest discover -s code/test/host -p 'test_*.py' -v
```

`CXX` and `CXXFLAGS` can select a compiler and additional flags. The test extracts
and compiles the actual `publishMqttData` and `isConnectedState` function bodies
from `interface_mqtt.cpp`, with ESP/network boundaries stubbed.

The checks cover retained online status (topic, payload, QoS and retain flag),
reconnection after a failed publish in the same measurement cycle, failure and
retry of the online publication, continued suppression after that failure,
next-cycle recovery, and disabled/disconnected behavior.

This test does not exercise ESP-IDF task scheduling, MQTT acknowledgements,
physical network reconnection, or Home Assistant. A target firmware build and
an integration test remain separate verification steps.
