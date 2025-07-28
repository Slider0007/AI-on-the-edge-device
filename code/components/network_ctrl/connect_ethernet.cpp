#include "connect_ethernet.h"

#ifdef BOARD_FEATURE_ETHERNET

#include "esp_event.h"
#include "esp_eth.h"
#include <esp_mac.h>
#include "esp_log.h"
#include "esp_netif.h"
#include <esp_netif_sntp.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"

#ifdef ENABLE_MQTT
#include "interface_mqtt.h"
#endif // ENABLE_MQTT

#include "configClass.h"
#include "connect_wlan.h"
#include "time_sntp.h"
#include "ClassLogFile.h"
#include "helper.h"
#include "statusled.h"
#include "mdns_service.h"

static const char *TAG = "ETHERNET";

static EventGroupHandle_t ethEventGroup;
#define ETH_CONNECTED_BIT BIT0

static const CfgData::SectionNetwork *cfgDataPtr = NULL;
static esp_eth_handle_t ethHandle = nullptr;

static struct strEthState {
    bool initialized = false;
    bool connected = false;
    bool connectionSuccessful = false;
} ethState;


static struct IpCfg {
    std::string ipAddress = "Undefined";
    std::string subnetMask = "Undefined";
    std::string gatewayAddress = "Undefined";
    std::string dnsServer = "Undefined";
    std::string macAddress = "00:00:00:00:00:00";
} ipCfg;


static std::string macToString(const uint8_t (&mac)[6])
{
    char macFormatted[18]; // "AA:BB:CC:DD:EE:FF" + null terminator
    snprintf(macFormatted, sizeof(macFormatted), MACSTR, MAC2STR(mac));
    return std::string(macFormatted);
}


static void ethEventHandler(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData)
{
    if (eventId == ETHERNET_EVENT_CONNECTED) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Ethernet connected (Link up)");
        ethState.connected = true;
        xEventGroupSetBits(ethEventGroup, ETH_CONNECTED_BIT);
    }
    else if (eventId == ETHERNET_EVENT_DISCONNECTED) {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Ethernet disconnected (Link down)");
        ethState.connected = false;
        xEventGroupClearBits(ethEventGroup, ETH_CONNECTED_BIT);
    }
}


static void ipEventHandler(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData)
{
    if (eventId == IP_EVENT_ETH_GOT_IP) {
        ethState.connected = true;
        ethState.connectionSuccessful = true;

        if (cfgDataPtr->ethernet.ipv4.networkConfig == NETWORK_IP_CONFIG_DHCP) {
            char buf[20];
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)eventData;

            ipCfg.ipAddress = std::string(esp_ip4addr_ntoa(&event->ip_info.ip, buf, sizeof(buf)));
            ipCfg.subnetMask = std::string(esp_ip4addr_ntoa(&event->ip_info.netmask, buf, sizeof(buf)));
            ipCfg.gatewayAddress = std::string(esp_ip4addr_ntoa(&event->ip_info.gw, buf, sizeof(buf)));

            esp_netif_dns_info_t dnsInfo;
            esp_netif_get_dns_info(event->esp_netif, ESP_NETIF_DNS_MAIN, &dnsInfo);
            ipCfg.dnsServer = std::string(esp_ip4addr_ntoa((const esp_ip4_addr_t *)&dnsInfo.ip, buf, sizeof(buf)));
        }

        LogFile.writeToFile(ESP_LOG_INFO, TAG,
                            "Assigned IP: " + ipCfg.ipAddress + ", Subnet: " + ipCfg.subnetMask + ", Gateway: " + ipCfg.gatewayAddress +
                                ", DNS: " + ipCfg.dnsServer);

        // Start NTP service
        if (getTimeSyncEnabled()) {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Start NTP service");
            esp_netif_sntp_start();
        }

#ifdef ENABLE_MQTT
        vTaskDelay(pdMS_TO_TICKS(1000));
        startMqttClient();
#endif
    }
    else if (eventId == IP_EVENT_ETH_LOST_IP) {
        ethState.connectionSuccessful = false;

        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Ethernet: IP address lost");
    }
    else {
        ethState.connectionSuccessful = false;

        LogFile.writeToFile(ESP_LOG_WARN, TAG, "Unhandled IP event: " + std::to_string(eventId));
    }
}


esp_err_t ethW5500SetMac()
{
    uint8_t baseMac[6];
    esp_err_t retVal = esp_efuse_mac_get_default(baseMac);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to get ESP MAC | Error:" + intToHexString(retVal));
        return ESP_FAIL;
    }

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "ESP MAC: " + macToString(baseMac));


    uint8_t localMac[6];
    esp_derive_local_mac(localMac, baseMac);
    localMac[5] = (localMac[5] + 5) & 0xFF; // Add offset 5 to last byte

    retVal = esp_eth_ioctl(ethHandle, ETH_CMD_S_MAC_ADDR, localMac);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to set local MAC | Error:" + intToHexString(retVal));
        return ESP_FAIL;
    }

    ipCfg.macAddress = macToString(localMac);
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Configured MAC (W5500): " + ipCfg.macAddress);

    return ESP_OK;
}


void ethW5500Reset(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << GPIO_ETH_RST,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Perform hardware reset: active LOW
    gpio_set_level(GPIO_ETH_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(150));
    gpio_set_level(GPIO_ETH_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(250));
}


esp_err_t initEthernetW5500()
{
    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Initializing ethernet module (W5500)");

    cfgDataPtr = &ConfigClass::getInstance()->get()->sectionNetwork;

    ethW5500Reset();

    esp_event_loop_create_default();

    ethEventGroup = xEventGroupCreate();

    spi_bus_config_t spiBusCfg = {
        .mosi_io_num = GPIO_ETH_MOSI,
        .miso_io_num = GPIO_ETH_MISO,
        .sclk_io_num = GPIO_ETH_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .data_io_default_level = 0,
        .max_transfer_sz = 0,
        .intr_flags = ESP_INTR_FLAG_IRAM,
    };
    spi_bus_initialize(SPI2_HOST, &spiBusCfg, SPI_DMA_CH_AUTO);

    gpio_set_direction(GPIO_ETH_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_ETH_CS, 1);
    spi_device_interface_config_t spiDevCfg = {
        .mode = 0,
        .clock_speed_hz = 40 * 1000 * 1000, // 40 Mhz
        .spics_io_num = GPIO_ETH_CS,
        .queue_size = 20,
    };

    gpio_set_direction(GPIO_ETH_INT, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_ETH_INT, GPIO_PULLUP_ONLY);
    eth_w5500_config_t ethW5500Cfg = ETH_W5500_DEFAULT_CONFIG(SPI2_HOST, &spiDevCfg);
    ethW5500Cfg.int_gpio_num = GPIO_ETH_INT;
    gpio_install_isr_service(0);

    eth_mac_config_t ethMacCfg = ETH_MAC_DEFAULT_CONFIG();
    esp_eth_mac_t *ethMac = esp_eth_mac_new_w5500(&ethW5500Cfg, &ethMacCfg);

    eth_phy_config_t ethPhyCfg = ETH_PHY_DEFAULT_CONFIG();
    ethPhyCfg.reset_gpio_num = GPIO_ETH_RST;
    // ethPhyCfg.phy_addr = 1;
    esp_eth_phy_t *ethPhy = esp_eth_phy_new_w5500(&ethPhyCfg);

    esp_eth_config_t ethConfig = ETH_DEFAULT_CONFIG(ethMac, ethPhy);
    esp_eth_driver_install(&ethConfig, &ethHandle);

    if (ethW5500SetMac() != ESP_OK) {
        return ESP_FAIL;
    }

    esp_netif_config_t netifCfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *ethNetif = esp_netif_new(&netifCfg);
    esp_netif_attach(ethNetif, esp_eth_new_netif_glue(ethHandle));

    if (cfgDataPtr->ethernet.ipv4.networkConfig == NETWORK_IP_CONFIG_STATIC) {
        LogFile.writeToFile(ESP_LOG_INFO, TAG,
                            "Use static network config | IP: " + cfgDataPtr->ethernet.ipv4.ipAddress +
                                ", Subnet: " + cfgDataPtr->ethernet.ipv4.subnetMask +
                                ", Gateway: " + cfgDataPtr->ethernet.ipv4.gatewayAddress + ", DNS: " + cfgDataPtr->ethernet.ipv4.dnsServer);

        esp_err_t retVal = esp_netif_dhcpc_stop(ethNetif); // Stop DHCP service
        if (retVal != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_dhcpc_stop: Error: " + intToHexString(retVal));
            return retVal;
        }
        esp_netif_ip_info_t ipInfo;
        memset(&ipInfo, 0, sizeof(esp_netif_ip_info_t));

        ipCfg.ipAddress = cfgDataPtr->ethernet.ipv4.ipAddress;
        ipInfo.ip.addr = esp_ip4addr_aton(ipCfg.ipAddress.c_str());

        ipCfg.subnetMask = cfgDataPtr->ethernet.ipv4.subnetMask;
        ipInfo.netmask.addr = esp_ip4addr_aton(ipCfg.subnetMask.c_str());

        ipCfg.gatewayAddress = cfgDataPtr->ethernet.ipv4.gatewayAddress;
        ipInfo.gw.addr = esp_ip4addr_aton(ipCfg.gatewayAddress.c_str());

        retVal = esp_netif_set_ip_info(ethNetif, &ipInfo);
        if (retVal != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_set_ip_info: Error: " + intToHexString(retVal));
            return retVal;
        }

        if (cfgDataPtr->ethernet.ipv4.dnsServer.empty()) {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "No DNS address set, use gateway address as DNS");
            ipCfg.dnsServer = cfgDataPtr->ethernet.ipv4.gatewayAddress;
        }
        else {
            ipCfg.dnsServer = cfgDataPtr->ethernet.ipv4.dnsServer;
        }

        esp_netif_dns_info_t dnsInfo;
        dnsInfo.ip.u_addr.ip4.addr = esp_ip4addr_aton(ipCfg.dnsServer.c_str());

        retVal = esp_netif_set_dns_info(ethNetif, ESP_NETIF_DNS_MAIN, &dnsInfo);
        if (retVal != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_set_dns_info: Error: " + intToHexString(retVal));
            return retVal;
        }
    }
    else {
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Use DHCP provided network config");
    }

    esp_err_t retVal = esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &ethEventHandler, NULL);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to register ETH_EVENT | Error: " + intToHexString(retVal));
        return ESP_FAIL;
    }

    retVal = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ipEventHandler, NULL);
    if (retVal != ESP_OK) {
        LogFile.writeToFile(ESP_LOG_ERROR, TAG, "Failed to register IP_EVENT | Error: " + intToHexString(retVal));
        return ESP_FAIL;
    }

    esp_eth_start(ethHandle);

    // Set hostname
    if (!cfgDataPtr->hostname.empty()) {
        retVal = esp_netif_set_hostname(ethNetif, cfgDataPtr->hostname.c_str());
        if (retVal != ESP_OK) {
            LogFile.writeToFile(ESP_LOG_ERROR, TAG, "esp_netif_set_hostname: Error: " + intToHexString(retVal));
        }
        else {
            LogFile.writeToFile(ESP_LOG_INFO, TAG, "Assigned hostname: " + cfgDataPtr->hostname);
        }
    }

    ethState.initialized = true;

    LogFile.writeToFile(ESP_LOG_INFO, TAG, "Waiting for ethernet connection");

    // Waiting and check for ethernet link (Timeout: 30s)
    EventBits_t bits = xEventGroupWaitBits(ethEventGroup, ETH_CONNECTED_BIT, pdFALSE, pdTRUE,
                                           pdMS_TO_TICKS(ETHERNET_WAITING_TIME_FOR_CONNECTION));

    if ((bits & ETH_CONNECTED_BIT) == ETH_CONNECTED_BIT) {
        // Get network speed info
        eth_speed_t speed;
        esp_eth_ioctl(ethHandle, ETH_CMD_G_SPEED, &speed);
        LogFile.writeToFile(ESP_LOG_INFO, TAG, "Speed: " + std::string((speed == 0) ? "10 Mbps" : (speed == 1) ? "100 Mbps" : "Unknown"));

        // Init mDNS service
        mDnsInit(cfgDataPtr->hostname);
    }
    else {
        LogFile.writeToFile(ESP_LOG_WARN, TAG, "No ethernet connection established");
    }

    return ESP_OK;
}


bool getEthernetConnectionState()
{
    return ethState.connected;
}


bool getEthDhcpStatus(void)
{
    if (ConfigClass::getInstance()->get()->sectionNetwork.ethernet.ipv4.networkConfig == NETWORK_IP_CONFIG_STATIC) {
        return false;
    }

    return true;
}


std::string getEthIpAddress(void)
{
    return ipCfg.ipAddress;
}


std::string getEthNetmaskAddress(void)
{
    return ipCfg.subnetMask;
}


std::string getEthGatewayAddress(void)
{
    return ipCfg.gatewayAddress;
}


std::string getEthDnsAddress(void)
{
    return ipCfg.dnsServer;
}


std::string getEthMac(void)
{
    return ipCfg.macAddress;
}


void deinitEthernet(void)
{
    mDnsDeinit();

    ethState = {0};

    esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, ipEventHandler);
    esp_event_handler_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, ethEventHandler);

    esp_eth_stop(ethHandle);
}

#endif // BOARD_FEATURE_ETHERNET
