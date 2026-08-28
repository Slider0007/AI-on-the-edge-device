#include "time_sntp.h"
#include "../../include/defines.h"

#include <sys/time.h>

#include <esp_log.h>
#include "esp_sntp.h"
#include "esp_netif_sntp.h"

#include "configClass.h"
#include "ClassLogFile.h"
#include "interface_mqtt.h"


static const char *TAG = "SNTP";

static std::string timeZone = "";
static std::string timeServer = "";
static bool timeSyncEnabled = true;
static bool timeWasNotSetAtBoot = false;
static bool timeWasNotSetAtBoot_PrintStartBlock = false;
static bool isTimeSynchronized = false;


std::string convertTimeToString(time_t _time, const char *frm)
{
    struct tm timeinfo;
    char strftimeBuf[64];

    localtime_r(&_time, &timeinfo);
    strftime(strftimeBuf, sizeof(strftimeBuf), frm, &timeinfo);
    return std::string(strftimeBuf);
}


std::string getCurrentTimeString(const char *frm)
{
    time_t now;

    time(&now);
    return convertTimeToString(now, frm);
}


bool getTimeIsSet(void)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    // Is time set? If not, tm_year will be (1970 - 1900).
    if ((timeinfo.tm_year < (2025 - 1900))) {
        return false;
    }

    return true;
}


bool getTimeSyncEnabled(void)
{
    return timeSyncEnabled;
}


bool getTimeIsSynced(void)
{
    if (timeSyncEnabled) {
        return isTimeSynchronized;
    }

    return false;
}


std::string getNTPSyncStatus(void)
{
    if (timeSyncEnabled) {
        if (isTimeSynchronized) {
            return "Synchronized";
        }
        else {
            return "Not Synchronized";
        }
    }

    return "Disabled";
}


bool getTimeWasNotSetAtBoot(void)
{
    return timeWasNotSetAtBoot;
}


std::string getServerName(void)
{
    char buf[128];

    if (esp_sntp_getservername(0)) {
        snprintf(buf, sizeof(buf), "%s", esp_sntp_getservername(0));
        return std::string(buf);
    }
    else { // Use IPv4 or IPv6 address instead
        ip_addr_t const *ip = esp_sntp_getserver(0);
        if (ipaddr_ntoa_r(ip, buf, sizeof(buf)) != NULL) {
            return std::string(buf);
        }
    }
    return "Unknown SNTP server";
}


bool setTime(const std::string &timeString)
{
    struct tm timeObj = {};
    if (strptime(timeString.c_str(), "%Y-%m-%dT%H:%M:%S", &timeObj) == NULL) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to parse time string: " + timeString);
        return false;
    }
    timeObj.tm_isdst = -1; // Auto detect DST setting

    time_t time = mktime(&timeObj);
    if (time == -1) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to convert parsed time to epoch");
        return false;
    }

    struct timeval now = {.tv_sec = time, .tv_usec = 0};
    if (settimeofday(&now, NULL) != 0) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to set device time");
        return false;
    }

    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Time manually set to " + timeString);
    return true;
}


void setTimeZone(std::string _tzstring)
{
    setenv("TZ", _tzstring.c_str(), 1);
    tzset();
    _tzstring = "Time zone set to " + _tzstring;
    LogFile.writeToFile(ESP_LOG_INFO, TAG, _tzstring);
}


void timeSyncNotificationCallback(struct timeval *tv)
{
    if (timeWasNotSetAtBoot_PrintStartBlock) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "===================================");
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "============== Start ==============");
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "= Before sync: log_1970-01-01.txt =");
        timeWasNotSetAtBoot_PrintStartBlock = false;
    }
    LogFile.writeToFile(ESP_LOG_INFO, TAG,
                        "Time is synced with NTP server " + getServerName() + ": " + getCurrentTimeString("%Y-%m-%d %H:%M:%S"));
    isTimeSynchronized = true;

#ifdef ENABLE_MQTT
    // Start MQTT service
    startMqttClient();
#endif // ENABLE_MQTT
}


/**
 * Init NTP client and set time
 * The RTC keeps the time after a soft reboot (except on power on or pin reset)
 * There should only be a minor correction through NTP
 */
bool initTime()
{
    // Set time zone in any case, even no time source is selected
    timeZone = ConfigClass::getInstance()->get()->sectionNetwork.time.timeZone;
    if (timeZone == "") {
        timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "No time zone set, using default: " + timeZone);
    }
    setTimeZone(timeZone);

    // Configure time server
    timeServer = ConfigClass::getInstance()->get()->sectionNetwork.time.ntp.timeServer;
    if (timeServer.empty()) {        // parameter part is empty
        timeServer = "pool.ntp.org"; // Use Default
    }

    if (ConfigClass::getInstance()->get()->sectionNetwork.time.ntp.timeSyncEnabled) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init NTP service");
        timeSyncEnabled = true;

        esp_sntp_config_t sntpConfig = ESP_NETIF_SNTP_DEFAULT_CONFIG(timeServer.c_str());
        sntpConfig.sync_cb = timeSyncNotificationCallback;
        sntpConfig.start = false; // Start SNTP only after wifi is connected and IP is assigned --> connect_wlan.cpp
        sntpConfig.wait_for_sync = false;
        esp_netif_sntp_init(&sntpConfig);
        isTimeSynchronized = false;
    }
    else {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "NTP service disabled");
        timeSyncEnabled = false;
        isTimeSynchronized = false;
    }

    if (!getTimeIsSet()) {
        timeWasNotSetAtBoot = true;
        timeWasNotSetAtBoot_PrintStartBlock = true;
    }

    // Get current time
    time_t now;
    time(&now);
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Current time: " + convertTimeToString(now, "%Y-%m-%d %H:%M:%S"));

    // Reset manual time parameter
    ConfigClass::getInstance()->cfgTmp()->sectionNetwork.time.timeSetManual = "";

    return true;
}


/**
 * Update Time zone
 */
void reconfigureTimeZone(std::string _timeZone)
{
    if (timeZone == _timeZone) {
        return;
    }

    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Time zone has been modified");

    timeZone = _timeZone;

    if (_timeZone == "") {
        _timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "No time zone set, using default: " + _timeZone);
    }

    setTimeZone(_timeZone);
}


/**
 * Time server and init or restart NTP client
 */
void reconfigureTime(bool _timeSyncEnabled, std::string _timeServer, std::string _timeZone)
{
    if (_timeServer.empty()) {        // Parameter is empty
        _timeServer = "pool.ntp.org"; // Use default
    }

    if (_timeZone.empty()) {                      // Parameter is empty
        _timeZone = "CET-1CEST,M3.5.0,M10.5.0/3"; // Use default
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "No time zone set, using default: " + timeZone);
    }

    // Set time manually (NTP disabled and time parameter set)
    if (!_timeSyncEnabled && !ConfigClass::getInstance()->get()->sectionNetwork.time.timeSetManual.empty()) {
        setTime(ConfigClass::getInstance()->get()->sectionNetwork.time.timeSetManual);
        ConfigClass::getInstance()->cfgTmp()->sectionNetwork.time.timeSetManual.clear();
    }

    // Return if no config changes detected
    if (timeSyncEnabled == _timeSyncEnabled && timeServer == _timeServer && timeZone == _timeZone) {
        return;
    }

    LogFile.writeToFile(ESP_LOG_WARN, TAG, "Time configuration has been modified");

    // Reconfigure time zone
    reconfigureTimeZone(_timeZone);

    // Reconfigure time server
    if (!_timeSyncEnabled) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "NTP service disabled");
        timeSyncEnabled = false;
        timeServer = "";
        esp_netif_sntp_deinit();
        isTimeSynchronized = false;
    }
    else {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Time server: " + _timeServer);
        timeSyncEnabled = true;
        timeServer = _timeServer;

        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Init NTP service");
        esp_netif_sntp_deinit();
        esp_sntp_config_t sntpConfig = ESP_NETIF_SNTP_DEFAULT_CONFIG(timeServer.c_str());
        sntpConfig.sync_cb = timeSyncNotificationCallback;
        esp_netif_sntp_init(&sntpConfig);
        isTimeSynchronized = false;
    }
}
