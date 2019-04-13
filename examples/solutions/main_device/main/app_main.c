#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "iot_export.h"
#include "iot_import.h"
#include "awss.h"
#include "platform_hal.h"
#include "app_entry.h"
#include "iot_import_awss.h"
#include "light_control.h"
#include "restore.h"

static const char *TAG = "main";
#define NVS_KEY_WIFI_CONFIG "wifi_config"
#define CONNECT_AP_TIMEOUT 60000

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGW(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        break;

    default:
        break;
    }

    return ESP_OK;
}

static void initialise_wifi(void)
{
    ESP_LOGI(TAG, "初始化WiFi设置");
    tcpip_adapter_init();
    esp_init_wifi_event_group();
    ESP_ERROR_CHECK(esp_event_loop_init(NULL, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    set_user_wifi_event_cb(wifi_event_handler);
}

void smart_light_example(void *parameter)
{
    while (1)
    {
        // wait for WiFi connected
        HAL_Wait_Net_Ready(0);
        ESP_LOGI(TAG, "网络准备就绪！");

        app_main_paras_t paras;
        char *argv[] = {"main", "loop"};
        paras.argc = 2;
        paras.argv = argv;
        ESP_LOGI(TAG, "进入 linkkit main...");
        linkkit_main((void *)&paras);
    }
}

static void linkkit_event_monitor(int event)
{
    char *tag = "配网回调";
    switch (event)
    {
    case IOTX_AWSS_START: // AWSS start without enbale, just supports device discover
        // operate led to indicate user
        ESP_LOGI(tag, "IOTX_AWSS_START");
        break;
    case IOTX_AWSS_ENABLE: // AWSS enable, AWSS doesn't parse awss packet until AWSS is enabled.
        ESP_LOGI(tag, "IOTX_AWSS_ENABLE");
        // operate led to indicate user
        break;
    case IOTX_AWSS_LOCK_CHAN: // AWSS lock channel(Got AWSS sync packet)
        ESP_LOGI(tag, "IOTX_AWSS_LOCK_CHAN");
        // operate led to indicate user
        break;
    case IOTX_AWSS_PASSWD_ERR: // AWSS decrypt passwd error
        ESP_LOGE(tag, "IOTX_AWSS_PASSWD_ERR");
        // operate led to indicate user
        break;
    case IOTX_AWSS_GOT_SSID_PASSWD:
        ESP_LOGI(tag, "IOTX_AWSS_GOT_SSID_PASSWD");
        // operate led to indicate user
        break;
    case IOTX_AWSS_CONNECT_ADHA: // AWSS try to connnect adha (device
                                 // discover, router solution)
        ESP_LOGI(tag, "IOTX_AWSS_CONNECT_ADHA");
        // operate led to indicate user
        break;
    case IOTX_AWSS_CONNECT_ADHA_FAIL: // AWSS fails to connect adha
        ESP_LOGE(tag, "IOTX_AWSS_CONNECT_ADHA_FAIL");
        // operate led to indicate user
        break;
    case IOTX_AWSS_CONNECT_AHA: // AWSS try to connect aha (AP solution)
        ESP_LOGI(tag, "IOTX_AWSS_CONNECT_AHA");
        // operate led to indicate user
        break;
    case IOTX_AWSS_CONNECT_AHA_FAIL: // AWSS fails to connect aha
        ESP_LOGE(tag, "IOTX_AWSS_CONNECT_AHA_FAIL");
        // operate led to indicate user
        break;
    case IOTX_AWSS_SETUP_NOTIFY: // AWSS sends out device setup information
                                 // (AP and router solution)
        ESP_LOGI(tag, "IOTX_AWSS_SETUP_NOTIFY");
        // operate led to indicate user
        break;
    case IOTX_AWSS_CONNECT_ROUTER: // AWSS try to connect destination router
        ESP_LOGI(tag, "IOTX_AWSS_CONNECT_ROUTER");
        // operate led to indicate user
        break;
    case IOTX_AWSS_CONNECT_ROUTER_FAIL: // AWSS fails to connect destination
                                        // router.
        ESP_LOGE(tag, "IOTX_AWSS_CONNECT_ROUTER_FAIL");
        // operate led to indicate user
        break;
    case IOTX_AWSS_GOT_IP: // AWSS connects destination successfully and got
                           // ip address
        ESP_LOGI(tag, "IOTX_AWSS_GOT_IP");
        // operate led to indicate user
        break;
    case IOTX_AWSS_SUC_NOTIFY: // AWSS sends out success notify (AWSS
                               // sucess)
        ESP_LOGI(tag, "IOTX_AWSS_SUC_NOTIFY");
        // operate led to indicate user
        break;
    case IOTX_AWSS_BIND_NOTIFY: // AWSS sends out bind notify information to
                                // support bind between user and device
        ESP_LOGI(tag, "IOTX_AWSS_BIND_NOTIFY");
        awss_stop();
        // operate led to indicate user
        break;
    case IOTX_AWSS_ENABLE_TIMEOUT: // AWSS enable timeout
                                   // user needs to enable awss again to support get ssid & passwd of router
        ESP_LOGW(tag, "IOTX_AWSS_ENALBE_TIMEOUT");
        // operate led to indicate user
        break;
    case IOTX_CONN_CLOUD: // Device try to connect cloud
        ESP_LOGI(tag, "IOTX_CONN_CLOUD");
        // operate led to indicate user
        break;
    case IOTX_CONN_CLOUD_FAIL: // Device fails to connect cloud, refer to
                               // net_sockets.h for error code
        ESP_LOGE(tag, "IOTX_CONN_CLOUD_FAIL");
        // operate led to indicate user
        break;
    case IOTX_CONN_CLOUD_SUC: // Device connects cloud successfully
        ESP_LOGI(tag, "IOTX_CONN_CLOUD_SUC");
        // operate led to indicate user
        break;
    case IOTX_RESET: // Linkkit reset success (just got reset response from
                     // cloud without any other operation)
        ESP_LOGI(tag, "IOTX_RESET");
        // operate led to indicate user
        break;
    default:
        break;
    }
}

void app_main()
{
    ESP_LOGI(TAG, "程序开始");
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "IDF版本%s", esp_get_idf_version());
    ESP_LOGI(TAG, "esp-aliyun版本：%s", HAL_GetEAVerison());
    ESP_LOGI(TAG, "iotkit-embedded版本：%s", HAL_GetIEVerison());

    restore_factory_init();

    led_light_start();

    initialise_wifi();

    wifi_config_t wifi_config;
    ret = esp_info_load(NVS_KEY_WIFI_CONFIG, &wifi_config, sizeof(wifi_config_t));
    if (ret < 0)
    {
        ESP_LOGI(TAG, "开始等待配网...");
        // make sure user touches device belong to themselves
        awss_set_config_press(1);

        set_iotx_info();
        // awss callback
        iotx_event_regist_cb(linkkit_event_monitor);

        // awss entry
        awss_start();
    }
    else
    {
        ESP_LOGI(TAG, "已配网，正在连接%s ...", (char *)(wifi_config.sta.ssid));
        HAL_Awss_Connect_Ap(CONNECT_AP_TIMEOUT, (char *)(wifi_config.sta.ssid), (char *)(wifi_config.sta.password), 0, 0, NULL, 0);
    }

    xTaskCreate(smart_light_example, "smart_light_example", 10240, NULL, 5, NULL);
}
