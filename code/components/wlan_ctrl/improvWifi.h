#ifndef IMPROV_WIFI_H
#define IMPROV_WIFI_H

#include <cstdint>
#include <string>
#include <vector>

#include "improvTypes.h"


class ImprovWiFi {
    private:
        const char *const CHIP_FAMILY_DESC[5] = {"ESP32", "ESP32-C3", "ESP32-S2", "ESP32-S3", "ESP8266"};
        ImprovTypes::ImprovWiFiParamsStruct improvWiFiParams;

        uint8_t _buffer[128];
        uint8_t _position = 0;

        void sendDeviceUrl(ImprovTypes::Command cmd);
        void SerialWriteCallback(const unsigned char *txData, int length);
        bool onCommandCallback(ImprovTypes::ImprovCommand cmd);
        void onErrorCallback(ImprovTypes::Error err);
        void setState(ImprovTypes::State state);
        void sendResponse(std::vector<uint8_t> &response);
        void setError(ImprovTypes::Error error);
        void getWifiNetworks();

        // improv SDK
        bool parseImprovSerial(size_t position, uint8_t byte, const uint8_t *buffer);
        ImprovTypes::ImprovCommand parseImprovData(const std::vector<uint8_t> &data, bool check_checksum = true);
        ImprovTypes::ImprovCommand parseImprovData(const uint8_t *data, size_t length, bool check_checksum = true);
        std::vector<uint8_t> build_rpc_response(ImprovTypes::Command command,
                                                const std::vector<std::string> &datum,
                                                bool add_checksum);


        // Type definition
        // **************************
        // Serial write function
        typedef void(SerialWrite)(const unsigned char *txData, int length);

        // Callback function called when any error occurs during the protocol handlingor wifi connection.
        typedef void(OnImprovError)(ImprovTypes::Error);

        // Callback function called when the attempt of wlan connection is successful
        typedef void(OnImprovConnected)(const char *ssid, const char *password);

        // Callback functions to customize the wifi connection
        typedef bool(CustomConnectWiFi)(const char *ssid, const char *password);
        typedef void(CustomScanWiFi)(unsigned char *scanResponse, int bufLen, uint16_t *networkNum);
        typedef bool(CustomIsConnected)(void);
        typedef std::string(CustomGetLocalIpCallback)(void);


        SerialWrite *serWriteCallback = NULL;
        OnImprovError *onImproErrorCallback = NULL;
        OnImprovConnected *onImprovConnectedCallback = NULL;
        CustomConnectWiFi *customConnectWiFiCallback = NULL;
        CustomScanWiFi *customScanWiFiCallback = NULL;
        CustomIsConnected *customIsConnectedCallback = NULL;
        CustomGetLocalIpCallback *customGetLocalIpCallback = NULL;

        std::vector<std::string> split(std::string s, std::string delimiter);

    public:
        ImprovWiFi(void) {}

        // Check serial communication
        void handleSerial(const uint8_t *data, size_t length);

        /**
         * Set details of your device
         *
         * Parameters
         * - `chipFamily` - Chip variant
         * - `firmwareName` - Firmware name
         * - `firmwareVersion` - Firmware version
         * - `deviceName` - Your device name
         */
        void setDeviceInfo(ImprovTypes::ChipFamily chipFamily, const char *firmwareName,
                            const char *firmwareVersion, const char *deviceName);

        void serialWrite(SerialWrite *serWriteCb);

        void onImprovError(OnImprovError *errorCallback);
        void onImprovConnected(OnImprovConnected *connectedCallback);

        void setCustomConnectWiFi(CustomConnectWiFi *connectWiFiCallBack);
        void setCustomScanWiFi(CustomScanWiFi *scanWiFiCallBack);
        void setCustomisConnected(CustomIsConnected *isConnectedCallBack);
        void setCustomGetLocalIpCallback(CustomGetLocalIpCallback *getLocalIpCallback);

        // Check if connection is established
        bool isConnected();
};

#endif /* IMPROV_WIFI_H */
