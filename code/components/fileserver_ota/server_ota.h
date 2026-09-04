#ifndef SERVEROTA_H
#define SERVEROTA_H

#include <string>

#include <esp_http_server.h>


enum class UnzipOtaStatus : uint8_t {
    Failed,
    Success
};

struct UnzipOtaResult {
    UnzipOtaStatus status;
    std::string firmwarePath;
};


void checkOtaStaged();

#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
void checkOtaPartitionState();
#endif // CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE

void doReboot();
void doRebootOTA();

void registerOtaRebootUri(httpd_handle_t server);

#endif // SERVEROTA_H
