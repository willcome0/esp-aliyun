#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "iot_import.h"
#include "iot_export.h"
#include "app_entry.h"
#include "esp_system.h"
#include "esp_log.h"

// RGB-LED
// #define PRODUCT_KEY "a1i35Bp8m5w"
// #define PRODUCT_SECRET "78WhxhY1Hhis5yqL"
// #define DEVICE_NAME "RGB-LED"
// #define DEVICE_SECRET "RTpmU7Oc3Zk9IuWvwiBGgFNeTVNegY5E"

// // Slave
// #define PRODUCT_KEY "a1y0bg1jCBS"
// #define PRODUCT_SECRET "tB1GZDwIeGNpav5q"
// #define DEVICE_NAME "Slave1"
// #define DEVICE_SECRET "Uo6lMAtNCl8cA2jp7kccilLiWbAXjfCo"

// Slave小
#define PRODUCT_KEY     "a1DlgeCqfvh"
#define PRODUCT_SECRET  "NCIRwiAYKQJoocZt"
#define DEVICE_NAME     "slave1"
#define DEVICE_SECRET   "l5ktB4SHBiJ1sSfJ18ELJfCs6wiJwrsY"

/* These are pre-defined topics */
#define TOPIC_UPDATE "/" PRODUCT_KEY "/" DEVICE_NAME "/user/update"
#define TOPIC_DATA "/" PRODUCT_KEY "/" DEVICE_NAME "/user/data"
#define TOPIC_ENENT_PROPERTY_POST "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"
#define ALINK_BODY_FORMAT "{\"id\":\"123\",\"version\":\"1.0\",\"method\":\"thing.event.property.post\",\"params\":%s}"

#define MQTT_MSGLEN (1024)



static int user_argc;
static char **user_argv;

void set_iotx_info()
{
    HAL_SetProductKey(PRODUCT_KEY);
    HAL_SetProductSecret(PRODUCT_SECRET);
    HAL_SetDeviceName(DEVICE_NAME);
    HAL_SetDeviceSecret(DEVICE_SECRET);
}

void mqtt_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    uintptr_t packet_id = (uintptr_t)msg->msg;
    iotx_mqtt_topic_info_pt topic_info = (iotx_mqtt_topic_info_pt)msg->msg;

    switch (msg->event_type)
    {
    case IOTX_MQTT_EVENT_UNDEF:
        ESP_LOGI("#事件回调--", "MQTT事件未定义");
        break;

    case IOTX_MQTT_EVENT_DISCONNECT:
        ESP_LOGI("#事件回调--", "MQTT断开连接");
        break;

    case IOTX_MQTT_EVENT_RECONNECT:
        ESP_LOGI("#事件回调--", "MQTT重连");
        break;

    case IOTX_MQTT_EVENT_SUBCRIBE_SUCCESS:
        ESP_LOGI("#事件回调--", "主题订阅成功（有应答），信息包ID：%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_SUBCRIBE_TIMEOUT:
        ESP_LOGI("#事件回调--", "主题订阅超时，信息包ID：%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_SUBCRIBE_NACK:
        ESP_LOGI("#事件回调--", "主题订阅失败（无应答），信息包ID：%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_UNSUBCRIBE_SUCCESS:
        ESP_LOGI("#事件回调--", "主题取消订阅成功（有应答），信息包ID：%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
        ESP_LOGI("#事件回调--", "主题取消订阅超时，信息包ID：%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_UNSUBCRIBE_NACK:
        ESP_LOGI("#事件回调--", "主题取消订阅失败（无应答），信息包ID：%u", (unsigned int)packet_id);
        break;

/*
    对于发送QoS = 1的消息，发送成功后会收到云端ACK并通过回调事件IOTX_MQTT_EVENT_PUBLISH_SUCCESS 
    告知用户，查看log
*/
    case IOTX_MQTT_EVENT_PUBLISH_SUCCESS:
        ESP_LOGI("#事件回调--", "发送消息成功（有应答），信息包ID：%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_PUBLISH_TIMEOUT:
        ESP_LOGI("#事件回调--", "发送消息超时，信息包ID：%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_PUBLISH_NACK:
        ESP_LOGI("#事件回调--", "发送消息失败（无应答），信息包ID：%u", (unsigned int)packet_id);
        break;

    case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
        ESP_LOGI("#事件回调--", "主题消息到达但没有任何相关句柄：");
        ESP_LOGI("#事件回调--", "主题=%.*s", topic_info->topic_len, topic_info->ptopic);
        ESP_LOGI("#事件回调--", "主题数据=%.*s", topic_info->payload_len, topic_info->payload);       
        break;

    case IOTX_MQTT_EVENT_BUFFER_OVERFLOW:
        // ESP_LOGI("#my", "缓存溢出：%s", msg->msg);
        ESP_LOGI("#事件回调--", "缓存溢出");
        break;

    default:
        ESP_LOGI("#事件回调--", "不应该到这");
        break;
    }
}

// 订阅主题后的回调函数
static void _demo_message_arrive(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    iotx_mqtt_topic_info_pt ptopic_info = (iotx_mqtt_topic_info_pt)msg->msg;

    ESP_LOGI("#DATA主题回调--", "订阅DATA，接收到消息");
    switch (msg->event_type)
    {
    case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
        /* print topic name and topic message */
        
        ESP_LOGI("#DATA主题回调--", "信息包ID：%d", ptopic_info->packet_id);
        ESP_LOGI("#DATA主题回调--", "主题名：'%.*s' (Length: %d)",
                 ptopic_info->topic_len,
                 ptopic_info->ptopic,
                 ptopic_info->topic_len);
        ESP_LOGI("#DATA主题回调--", "载荷数据：'%.*s' (Length: %d)",
                 ptopic_info->payload_len,
                 ptopic_info->payload,
                 ptopic_info->payload_len);
        
        break;
    default:
        ESP_LOGI("#DATA主题回调--", "不应该来到这");
        break;
    }
    ESP_LOGI("#DATA主题回调--", "订阅DATA，接收到消息结束");
}

    void *g_p_mqtt_client;
int mqtt_client(void)
{
    int rc, cnt = 0;
    
    char msg_pub[128];


    /* Device AUTH */
    iotx_conn_info_pt pconn_info;
    if (0 != IOT_SetupConnInfo(PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, (void **)&pconn_info))
    {
        ESP_LOGI("#my", "AUTH请求失败");
        return -1;
    }

    /* Initialize MQTT parameter */
    iotx_mqtt_param_t mqtt_params;
    memset(&mqtt_params, 0x0, sizeof(mqtt_params));

    mqtt_params.port = pconn_info->port;
    mqtt_params.host = pconn_info->host_name;
    mqtt_params.client_id = pconn_info->client_id;
    mqtt_params.username = pconn_info->username;
    mqtt_params.password = pconn_info->password;
    mqtt_params.pub_key = pconn_info->pub_key;

    mqtt_params.request_timeout_ms = 2000;
    mqtt_params.clean_session = 0;
    mqtt_params.keepalive_interval_ms = 60000;
    mqtt_params.read_buf_size = MQTT_MSGLEN;
    mqtt_params.write_buf_size = MQTT_MSGLEN;

    mqtt_params.handle_event.h_fp = mqtt_event_handle;
    mqtt_params.handle_event.pcontext = NULL;

    /* Construct a MQTT client with specify parameter */

    g_p_mqtt_client = IOT_MQTT_Construct(&mqtt_params);
    if (NULL == g_p_mqtt_client)
    {
        ESP_LOGI("#my", "MQTT构建失败");
        return -1;
    }

    /*********************发送消息**********************************/
    /* Initialize topic information */
    ESP_LOGI("#my", "发送消息：");
    iotx_mqtt_topic_info_t topic_msg;
    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
    strcpy(msg_pub, "主题UPDATE开始发送数据");

    topic_msg.qos = IOTX_MQTT_QOS1;
    topic_msg.retain = 0;
    topic_msg.dup = 0;
    topic_msg.payload = (void *)msg_pub;
    topic_msg.payload_len = strlen(msg_pub);

    rc = IOT_MQTT_Publish(g_p_mqtt_client, TOPIC_UPDATE, &topic_msg);
    if (rc < 0)
    {
        IOT_MQTT_Destroy(&g_p_mqtt_client);
        ESP_LOGI("#my", "发送消息时发生错误");
        return -1;
    }
    ESP_LOGI("#my", "主题名：%s\n 载荷数据：%s\n 返回值：%d", TOPIC_UPDATE, topic_msg.payload, rc);
    
    
    
    
    /*********************订阅主题**********************************/
    ESP_LOGI("#my", "订阅主题：%s", TOPIC_DATA);
    rc = IOT_MQTT_Subscribe(g_p_mqtt_client, TOPIC_DATA, IOTX_MQTT_QOS1, _demo_message_arrive, NULL);
    if (rc < 0)
    {
        IOT_MQTT_Destroy(&g_p_mqtt_client);
        ESP_LOGI("#my", "主题订阅失败，返回值为%d", rc);
        return -1;
    }

    IOT_MQTT_Yield(g_p_mqtt_client, 200);

    HAL_SleepMs(2000);

    /*********************发送消息**********************************/
    /* Initialize topic information */
    memset(msg_pub, 0x0, 128);
    strcpy(msg_pub, "主题DATA开始发送数据");
    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
    topic_msg.qos = IOTX_MQTT_QOS1;
    topic_msg.retain = 0;
    topic_msg.dup = 0;
    topic_msg.payload = (void *)msg_pub;
    topic_msg.payload_len = strlen(msg_pub);

    rc = IOT_MQTT_Publish(g_p_mqtt_client, TOPIC_DATA, &topic_msg);
    ESP_LOGI("#my", "主题名：%s\n 载荷数据：%s\n 返回值：%d", TOPIC_DATA, topic_msg.payload, rc);
    IOT_MQTT_Yield(g_p_mqtt_client, 200);

    do
    {
        ESP_LOGI("#循环--", "循环开始");
        /* Generate topic message */
        cnt++;
        int msg_len = snprintf(msg_pub, sizeof(msg_pub), "{\"attr_name\":\"temperature\",\"循环次数\":\"%d\"}", cnt); // 返回数据长度
        if (msg_len < 0)
        {
            ESP_LOGI("#循环--", "snprintf时发生错误");
            return -1;
        }

        topic_msg.payload = (void *)msg_pub;    // 发送的消息
        topic_msg.payload_len = msg_len;        // 消息长度

        rc = IOT_MQTT_Publish(g_p_mqtt_client, TOPIC_DATA, &topic_msg);
        if (rc < 0)
        {
            ESP_LOGI("#循环--", "发送消息时发生错误");
        }
        ESP_LOGI("#循环--", "信息包：ID%u", (uint32_t)rc);
        ESP_LOGI("#循环--", "载荷数据：%s", msg_pub);


/******************************************************/
    // {
    //     memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
    //     if (cnt%2)
    //     {
    //         sprintf(msg_pub, ALINK_BODY_FORMAT, "{\"LightStatus\":1}");
    //     }
    //     else
    //     {
    //         sprintf(msg_pub, ALINK_BODY_FORMAT, "{\"LightStatus\":0}");
    //     }

    //     topic_msg.qos = IOTX_MQTT_QOS1;
    //     topic_msg.retain = 0;
    //     topic_msg.dup = 0;
    //     topic_msg.payload = (void *)msg_pub;
    //     topic_msg.payload_len = strlen(msg_pub);

    //     rc = IOT_MQTT_Publish(g_p_mqtt_client, TOPIC_ENENT_PROPERTY_POST, &topic_msg);
    //     if (rc < 0)
    //     {
    //         IOT_MQTT_Destroy(&g_p_mqtt_client);
    //         ESP_LOGI("#MQTT--", "发送消息时发生错误");
    //         return -1;
    //     }
    //     ESP_LOGI("#MQTT--", "主题名：%s\n 载荷数据：%s\n 返回值：%d", TOPIC_ENENT_PROPERTY_POST, topic_msg.payload, rc);
    // }




        /* handle the MQTT packet received from TCP or SSL connection */
        IOT_MQTT_Yield(g_p_mqtt_client, 3000);

        /* infinite loop if running with 'loop' argument */
        if (user_argc >= 2 && !strcmp("loop", user_argv[1]))
        {
            // HAL_SleepMs(2000);
            // cnt = 0;
        }
        ESP_LOGI("#循环--", "最小空闲字节数：%u 空闲字节数：%u", esp_get_minimum_free_heap_size(), esp_get_free_heap_size());
    } while (cnt);

    IOT_MQTT_Yield(g_p_mqtt_client, 200);

    IOT_MQTT_Unsubscribe(g_p_mqtt_client, TOPIC_DATA);

    IOT_MQTT_Yield(g_p_mqtt_client, 200);

    IOT_MQTT_Destroy(&g_p_mqtt_client);

    return 0;
}

int linkkit_main(void *paras)
{
    IOT_SetLogLevel(IOT_LOG_DEBUG);

    user_argc = 0;
    user_argv = NULL;

    if (paras != NULL)
    {
        app_main_paras_t *p = (app_main_paras_t *)paras;
        user_argc = p->argc;
        user_argv = p->argv;
    }

    HAL_SetProductKey(PRODUCT_KEY);
    HAL_SetDeviceName(DEVICE_NAME);
    HAL_SetDeviceSecret(DEVICE_SECRET);
    HAL_SetProductSecret(PRODUCT_SECRET);

    /* Choose Login Server */
    int domain_type = IOTX_CLOUD_REGION_SHANGHAI;
    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);

    /* Choose Login  Method */
    int dynamic_register = 0;
    IOT_Ioctl(IOTX_IOCTL_SET_DYNAMIC_REGISTER, (void *)&dynamic_register);

    mqtt_client();
    IOT_DumpMemoryStats(IOT_LOG_DEBUG);
    IOT_SetLogLevel(IOT_LOG_NONE);

    ESP_LOGI("#my", "结束例程！");

    return 0;
}













// /*
//  * Copyright (C) 2015-2018 Alibaba Group Holding Limited
//  */
// #ifdef DEPRECATED_LINKKIT
// #include "deprecated/solo.c"
// #else
// #include "stdio.h"
// #include "iot_export_linkkit.h"
// #include "cJSON.h"
// #include "app_entry.h"
// #include "light_control.h"
// #include "esp_log.h"

// #if defined(OTA_ENABLED) && defined(BUILD_AOS)
// #include "ota_service.h"
// #endif

// static const char *TAG = "solo";
// #define USE_CUSTOME_DOMAIN (0)

// // for demo only
// #define PRODUCT_KEY "a1i35Bp8m5w"
// #define PRODUCT_SECRET "78WhxhY1Hhis5yqL"
// #define DEVICE_NAME "RGB-LED"
// #define DEVICE_SECRET "RTpmU7Oc3Zk9IuWvwiBGgFNeTVNegY5E"

// #if USE_CUSTOME_DOMAIN
// #define CUSTOME_DOMAIN_MQTT "iot-as-mqtt.cn-shanghai.aliyuncs.com"
// #define CUSTOME_DOMAIN_HTTP "iot-auth.cn-shanghai.aliyuncs.com"
// #endif

// #define USER_EXAMPLE_YIELD_TIMEOUT_MS (200)



// typedef struct
// {
//     int master_devid;
//     int cloud_connected;
//     int master_initialized;
// } user_example_ctx_t;

// static user_example_ctx_t g_user_example_ctx;

// static user_example_ctx_t *user_example_get_ctx(void)
// {
//     return &g_user_example_ctx;
// }

// void *example_malloc(size_t size)
// {
//     return HAL_Malloc(size);
// }

// void example_free(void *ptr)
// {
//     HAL_Free(ptr);
// }

// static int user_connected_event_handler(void)
// {
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();

//     EXAMPLE_TRACE("Cloud Connected");
//     user_example_ctx->cloud_connected = 1;
// #if defined(OTA_ENABLED) && defined(BUILD_AOS)
//     ota_service_init(NULL);
// #endif
//     return 0;
// }

// static int user_disconnected_event_handler(void)
// {
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();

//     EXAMPLE_TRACE("Cloud Disconnected");

//     user_example_ctx->cloud_connected = 0;

//     return 0;
// }

// static int user_down_raw_data_arrived_event_handler(const int devid, const unsigned char *payload,
//                                                     const int payload_len)
// {
//     EXAMPLE_TRACE("Down Raw Message, Devid: %d, Payload Length: %d", devid, payload_len);
//     return 0;
// }

// static int user_service_request_event_handler(const int devid, const char *serviceid, const int serviceid_len,
//                                               const char *request, const int request_len,
//                                               char **response, int *response_len)
// {
//     int contrastratio = 0, to_cloud = 0;
//     cJSON *root = NULL, *item_transparency = NULL, *item_from_cloud = NULL;
//     EXAMPLE_TRACE("Service Request Received, Devid: %d, Service ID: %.*s, Payload: %s", devid, serviceid_len,
//                   serviceid,
//                   request);

//     /* Parse Root */
//     root = cJSON_Parse(request);
//     if (root == NULL || !cJSON_IsObject(root))
//     {
//         EXAMPLE_TRACE("JSON Parse Error");
//         return -1;
//     }

//     if (strlen("Custom") == serviceid_len && memcmp("Custom", serviceid, serviceid_len) == 0)
//     {
//         /* Parse Item */
//         const char *response_fmt = "{\"Contrastratio\":%d}";
//         item_transparency = cJSON_GetObjectItem(root, "transparency");
//         if (item_transparency == NULL || !cJSON_IsNumber(item_transparency))
//         {
//             cJSON_Delete(root);
//             return -1;
//         }
//         EXAMPLE_TRACE("transparency: %d", item_transparency->valueint);
//         contrastratio = item_transparency->valueint + 1;

//         /* Send Service Response To Cloud */
//         *response_len = strlen(response_fmt) + 10 + 1;
//         *response = HAL_Malloc(*response_len);
//         if (*response == NULL)
//         {
//             EXAMPLE_TRACE("Memory Not Enough");
//             return -1;
//         }
//         memset(*response, 0, *response_len);
//         HAL_Snprintf(*response, *response_len, response_fmt, contrastratio);
//         *response_len = strlen(*response);
//     }
//     else if (strlen("SyncService") == serviceid_len && memcmp("SyncService", serviceid, serviceid_len) == 0)
//     {
//         /* Parse Item */
//         const char *response_fmt = "{\"ToCloud\":%d}";
//         item_from_cloud = cJSON_GetObjectItem(root, "FromCloud");
//         if (item_from_cloud == NULL || !cJSON_IsNumber(item_from_cloud))
//         {
//             cJSON_Delete(root);
//             return -1;
//         }
//         EXAMPLE_TRACE("FromCloud: %d", item_from_cloud->valueint);
//         to_cloud = item_from_cloud->valueint + 1;

//         /* Send Service Response To Cloud */
//         *response_len = strlen(response_fmt) + 10 + 1;
//         *response = HAL_Malloc(*response_len);
//         if (*response == NULL)
//         {
//             EXAMPLE_TRACE("Memory Not Enough");
//             return -1;
//         }
//         memset(*response, 0, *response_len);
//         HAL_Snprintf(*response, *response_len, response_fmt, to_cloud);
//         *response_len = strlen(*response);
//     }
//     cJSON_Delete(root);

//     return 0;
// }

// static rgb_t s_rgb = {50, 50, 50};
// static bool LED_STATE = 0;
// static void user_parse_cloud_cmd(const char *request, const int request_len)
// {
//     if (request == NULL || request_len <= 0)
//     {
//         return;
//     }

//     char *ptr = NULL;
//     // TODO: parse it as fixed format JSON string
//     if (strstr(request, "LightSwitch") != NULL)
//     { // {"LightSwitch":0}    // 修改
//         ptr = strstr(request, ":");
//         ptr++;
//         bool on = atoi(ptr);
//         LED_STATE = on;
//         if (on)
//         {
//             ESP_LOGI(TAG, "Set R:%d G:%d B:%d", s_rgb.r, s_rgb.g, s_rgb.b);
//             lightbulb_set_aim(s_rgb.r * PWM_TARGET_DUTY / 100, s_rgb.g * PWM_TARGET_DUTY / 100, s_rgb.b * PWM_TARGET_DUTY / 100, 0, 0, 0);
//             ESP_LOGI(TAG, "LED ON！！！");
//         }
//         else
//         {
//             ESP_LOGI(TAG, "Set Light OFF");
//             lightbulb_set_aim(0, 0, 0, 0, 0, 0);
//             ESP_LOGI(TAG, "LED OFF！！！");
//         }
//     }
//     else if (strstr(request, "RGBColor") != NULL)
//     { // {"RGBColor":{"Red":120,"Blue":36,"Green":108}}
//         ptr = strstr(request, "Red");
//         ptr += 5;
//         s_rgb.r = atoi(ptr);

//         ptr = strstr(request, "Blue");
//         ptr += 6;
//         s_rgb.g = atoi(ptr);

//         ptr = strstr(request, "Green");
//         ptr += 7;
//         s_rgb.b = atoi(ptr);

//         ESP_LOGI(TAG, "设置 R:%d G:%d B:%d", s_rgb.r, s_rgb.g, s_rgb.b);
//         lightbulb_set_aim(s_rgb.r * PWM_TARGET_DUTY / 100, s_rgb.g * PWM_TARGET_DUTY / 100, s_rgb.b * PWM_TARGET_DUTY / 100, 0, 0, 0);
//     }
//     else
//     {
//         ESP_LOGW(TAG, "不支持的命令");
//     }
// }

// static int user_property_set_event_handler(const int devid, const char *request, const int request_len)
// {
//     int res = 0;
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();
//     EXAMPLE_TRACE("Property Set Received, Devid: %d, Request: %s", devid, request);

//     printf("\r\n收到的消息：%s\r\n", request);
//     printf("\r\n收到的消息长度：%d\r\n", request_len);

//     user_parse_cloud_cmd(request, request_len);

//     res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
//                              (unsigned char *)request, request_len);
//     EXAMPLE_TRACE("Post Property Message ID: %d", res);

//     return 0;
// }

// static int user_property_get_event_handler(const int devid, const char *request, const int request_len, char **response,
//                                            int *response_len)
// {
//     cJSON *request_root = NULL, *item_propertyid = NULL;
//     cJSON *response_root = NULL;
//     int index = 0;
//     EXAMPLE_TRACE("Property Get Received, Devid: %d, Request: %s", devid, request);

//     /* Parse Request */
//     request_root = cJSON_Parse(request);
//     if (request_root == NULL || !cJSON_IsArray(request_root))
//     {
//         EXAMPLE_TRACE("JSON Parse Error");
//         return -1;
//     }

//     /* Prepare Response */
//     response_root = cJSON_CreateObject();
//     if (response_root == NULL)
//     {
//         EXAMPLE_TRACE("No Enough Memory");
//         cJSON_Delete(request_root);
//         return -1;
//     }

//     for (index = 0; index < cJSON_GetArraySize(request_root); index++)
//     {
//         item_propertyid = cJSON_GetArrayItem(request_root, index);
//         if (item_propertyid == NULL || !cJSON_IsString(item_propertyid))
//         {
//             EXAMPLE_TRACE("JSON Parse Error");
//             cJSON_Delete(request_root);
//             cJSON_Delete(response_root);
//             return -1;
//         }

//         EXAMPLE_TRACE("Property ID, index: %d, Value: %s", index, item_propertyid->valuestring);

//         if (strcmp("WIFI_Tx_Rate", item_propertyid->valuestring) == 0)
//         {
//             cJSON_AddNumberToObject(response_root, "WIFI_Tx_Rate", 1111);
//         }
//         else if (strcmp("WIFI_Rx_Rate", item_propertyid->valuestring) == 0)
//         {
//             cJSON_AddNumberToObject(response_root, "WIFI_Rx_Rate", 2222);
//         }
//         else if (strcmp("RGBColor", item_propertyid->valuestring) == 0)
//         {
//             cJSON *item_rgbcolor = cJSON_CreateObject();
//             if (item_rgbcolor == NULL)
//             {
//                 cJSON_Delete(request_root);
//                 cJSON_Delete(response_root);
//                 return -1;
//             }
//             cJSON_AddNumberToObject(item_rgbcolor, "Red", 100);
//             cJSON_AddNumberToObject(item_rgbcolor, "Green", 100);
//             cJSON_AddNumberToObject(item_rgbcolor, "Blue", 100);
//             cJSON_AddItemToObject(response_root, "RGBColor", item_rgbcolor);
//         }
//         else if (strcmp("HSVColor", item_propertyid->valuestring) == 0)
//         {
//             cJSON *item_hsvcolor = cJSON_CreateObject();
//             if (item_hsvcolor == NULL)
//             {
//                 cJSON_Delete(request_root);
//                 cJSON_Delete(response_root);
//                 return -1;
//             }
//             cJSON_AddNumberToObject(item_hsvcolor, "Hue", 50);
//             cJSON_AddNumberToObject(item_hsvcolor, "Saturation", 50);
//             cJSON_AddNumberToObject(item_hsvcolor, "Value", 50);
//             cJSON_AddItemToObject(response_root, "HSVColor", item_hsvcolor);
//         }
//         else if (strcmp("HSLColor", item_propertyid->valuestring) == 0)
//         {
//             cJSON *item_hslcolor = cJSON_CreateObject();
//             if (item_hslcolor == NULL)
//             {
//                 cJSON_Delete(request_root);
//                 cJSON_Delete(response_root);
//                 return -1;
//             }
//             cJSON_AddNumberToObject(item_hslcolor, "Hue", 70);
//             cJSON_AddNumberToObject(item_hslcolor, "Saturation", 70);
//             cJSON_AddNumberToObject(item_hslcolor, "Lightness", 70);
//             cJSON_AddItemToObject(response_root, "HSLColor", item_hslcolor);
//         }
//         else if (strcmp("WorkMode", item_propertyid->valuestring) == 0)
//         {
//             cJSON_AddNumberToObject(response_root, "WorkMode", 4);
//         }
//         else if (strcmp("NightLightSwitch", item_propertyid->valuestring) == 0)
//         {
//             cJSON_AddNumberToObject(response_root, "NightLightSwitch", 1);
//         }
//         else if (strcmp("Brightness", item_propertyid->valuestring) == 0)
//         {
//             cJSON_AddNumberToObject(response_root, "Brightness", 30);
//         }
//         else if (strcmp("LightSwitch", item_propertyid->valuestring) == 0)
//         {
//             cJSON_AddNumberToObject(response_root, "LightSwitch", 1);
//         }
//         else if (strcmp("ColorTemperature", item_propertyid->valuestring) == 0)
//         {
//             cJSON_AddNumberToObject(response_root, "ColorTemperature", 2800);
//         }
//         else if (strcmp("PropertyCharacter", item_propertyid->valuestring) == 0)
//         {
//             cJSON_AddStringToObject(response_root, "PropertyCharacter", "testprop");
//         }
//         else if (strcmp("Propertypoint", item_propertyid->valuestring) == 0)
//         {
//             cJSON_AddNumberToObject(response_root, "Propertypoint", 50);
//         }
//         else if (strcmp("LocalTimer", item_propertyid->valuestring) == 0)
//         {
//             cJSON *array_localtimer = cJSON_CreateArray();
//             if (array_localtimer == NULL)
//             {
//                 cJSON_Delete(request_root);
//                 cJSON_Delete(response_root);
//                 return -1;
//             }

//             cJSON *item_localtimer = cJSON_CreateObject();
//             if (item_localtimer == NULL)
//             {
//                 cJSON_Delete(request_root);
//                 cJSON_Delete(response_root);
//                 cJSON_Delete(array_localtimer);
//                 return -1;
//             }
//             cJSON_AddStringToObject(item_localtimer, "Timer", "10 11 * * * 1 2 3 4 5");
//             cJSON_AddNumberToObject(item_localtimer, "Enable", 1);
//             cJSON_AddNumberToObject(item_localtimer, "IsValid", 1);
//             cJSON_AddItemToArray(array_localtimer, item_localtimer);
//             cJSON_AddItemToObject(response_root, "LocalTimer", array_localtimer);
//         }
//     }
//     cJSON_Delete(request_root);

//     *response = cJSON_PrintUnformatted(response_root);
//     if (*response == NULL)
//     {
//         EXAMPLE_TRACE("No Enough Memory");
//         cJSON_Delete(response_root);
//         return -1;
//     }
//     cJSON_Delete(response_root);
//     *response_len = strlen(*response);

//     EXAMPLE_TRACE("Property Get Response: %s", *response);

//     return SUCCESS_RETURN;
// }

// static int user_report_reply_event_handler(const int devid, const int msgid, const int code, const char *reply,
//                                            const int reply_len)
// {
//     const char *reply_value = (reply == NULL) ? ("NULL") : (reply);
//     const int reply_value_len = (reply_len == 0) ? (strlen("NULL")) : (reply_len);

//     EXAMPLE_TRACE("Message Post Reply Received, Devid: %d, Message ID: %d, Code: %d, Reply: %.*s", devid, msgid, code,
//                   reply_value_len,
//                   reply_value);
//     return 0;
// }

// static int user_trigger_event_reply_event_handler(const int devid, const int msgid, const int code, const char *eventid,
//                                                   const int eventid_len, const char *message, const int message_len)
// {
//     EXAMPLE_TRACE("Trigger Event Reply Received, Devid: %d, Message ID: %d, Code: %d, EventID: %.*s, Message: %.*s", devid,
//                   msgid, code,
//                   eventid_len,
//                   eventid, message_len, message);

//     return 0;
// }

// static int user_timestamp_reply_event_handler(const char *timestamp)
// {
//     EXAMPLE_TRACE("Current Timestamp: %s", timestamp);

//     return 0;
// }

// static int user_initialized(const int devid)
// {
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();
//     EXAMPLE_TRACE("Device Initialized, Devid: %d", devid);

//     if (user_example_ctx->master_devid == devid)
//     {
//         user_example_ctx->master_initialized = 1;
//     }

//     return 0;
// }

// /** type:
//   *
//   * 0 - new firmware exist
//   *
//   */
// static int user_fota_event_handler(int type, const char *version)
// {
//     char buffer[128] = {0};
//     int buffer_length = 128;
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();

//     if (type == 0)
//     {
//         EXAMPLE_TRACE("New Firmware Version: %s", version);

//         IOT_Linkkit_Query(user_example_ctx->master_devid, ITM_MSG_QUERY_FOTA_DATA, (unsigned char *)buffer, buffer_length);
//     }

//     return 0;
// }

// /** type:
//   *
//   * 0 - new config exist
//   *
//   */
// static int user_cota_event_handler(int type, const char *config_id, int config_size, const char *get_type,
//                                    const char *sign, const char *sign_method, const char *url)
// {
//     char buffer[128] = {0};
//     int buffer_length = 128;
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();

//     if (type == 0)
//     {
//         EXAMPLE_TRACE("New Config ID: %s", config_id);
//         EXAMPLE_TRACE("New Config Size: %d", config_size);
//         EXAMPLE_TRACE("New Config Type: %s", get_type);
//         EXAMPLE_TRACE("New Config Sign: %s", sign);
//         EXAMPLE_TRACE("New Config Sign Method: %s", sign_method);
//         EXAMPLE_TRACE("New Config URL: %s", url);

//         IOT_Linkkit_Query(user_example_ctx->master_devid, ITM_MSG_QUERY_COTA_DATA, (unsigned char *)buffer, buffer_length);
//     }

//     return 0;
// }

// #if 1
// static uint64_t user_update_sec(void)
// {
//     static uint64_t time_start_ms = 0;

//     if (time_start_ms == 0) {
//         time_start_ms = HAL_UptimeMs();
//     }

//     return (HAL_UptimeMs() - time_start_ms) / 1000;
// }
// #endif

// void user_post_property(void)
// {
//     static int example_index = 0;
//     int res = 0;
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();
//     char *property_payload = "NULL";

//     // if (example_index == 0)
//     // {
//         /* Normal Example */
//         static bool led_state = 0;

//         led_state = !led_state;
//         if (led_state)
//         {
//             property_payload = "{\"LightStatus\":0}";
//             set_light_status(0);
//         }
//         else
//         {
//             property_payload = "{\"LightStatus\":1}";
//             set_light_status(1);
//         }
            
//         // example_index++;
//     // }
//     // else if (example_index == 1)
//     // {
//     //     /* Wrong Property ID */
//     //     property_payload = "{\"LightSwitchxxxx\":1}";
//     //     example_index++;
//     // }
//     // else if (example_index == 2)
//     // {
//     //     /* Wrong Value Format */
//     //     property_payload = "{\"LightSwitch\":\"test\"}";
//     //     example_index++;
//     // }
//     // else if (example_index == 3)
//     // {
//     //     /* Wrong Value Range */
//     //     property_payload = "{\"LightSwitch\":10}";
//     //     example_index++;
//     // }
//     // else if (example_index == 4)
//     // {
//     //     /* Missing Property Item */
//     //     property_payload = "{\"RGBColor\":{\"Red\":45,\"Green\":30}}";
//     //     example_index++;
//     // }
//     // else if (example_index == 5)
//     // {
//     //     /* Wrong Params Format */
//     //     property_payload = "\"hello world\"";
//     //     example_index++;
//     // }
//     // else if (example_index == 6)
//     // {
//     //     /* Wrong Json Format */
//     //     property_payload = "hello world";
//     //     example_index = 0;
//     // }

//     res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
//                              (unsigned char *)property_payload, strlen(property_payload));

//     EXAMPLE_TRACE("Post Property Message ID: %d", res);
//     EXAMPLE_TRACE("上报数据：%d", !LED_STATE);
// }

// void user_post_event(void)
// {
//     static int example_index = 0;
//     int res = 0;
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();
//     char *event_id = "Error";
//     char *event_payload = "NULL";

//     if (example_index == 0)
//     {
//         /* Normal Example */
//         event_payload = "{\"ErrorCode\":0}";
//         example_index++;
//     }
//     else if (example_index == 1)
//     {
//         /* Wrong Property ID */
//         event_payload = "{\"ErrorCodexxx\":0}";
//         example_index++;
//     }
//     else if (example_index == 2)
//     {
//         /* Wrong Value Format */
//         event_payload = "{\"ErrorCode\":\"test\"}";
//         example_index++;
//     }
//     else if (example_index == 3)
//     {
//         /* Wrong Value Range */
//         event_payload = "{\"ErrorCode\":10}";
//         example_index++;
//     }
//     else if (example_index == 4)
//     {
//         /* Wrong Value Range */
//         event_payload = "\"hello world\"";
//         example_index++;
//     }
//     else if (example_index == 5)
//     {
//         /* Wrong Json Format */
//         event_payload = "hello world";
//         example_index = 0;
//     }

//     res = IOT_Linkkit_TriggerEvent(user_example_ctx->master_devid, event_id, strlen(event_id),
//                                    event_payload, strlen(event_payload));
//     EXAMPLE_TRACE("Post Event Message ID: %d", res);
// }

// void user_deviceinfo_update(void)
// {
//     int res = 0;
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();
//     char *device_info_update = "[{\"attrKey\":\"abc\",\"attrValue\":\"hello,world\"}]";

//     res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_DEVICEINFO_UPDATE,
//                              (unsigned char *)device_info_update, strlen(device_info_update));
//     EXAMPLE_TRACE("Device Info Update Message ID: %d", res);
// }

// void user_deviceinfo_delete(void)
// {
//     int res = 0;
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();
//     char *device_info_delete = "[{\"attrKey\":\"abc\"}]";

//     res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_DEVICEINFO_DELETE,
//                              (unsigned char *)device_info_delete, strlen(device_info_delete));
//     EXAMPLE_TRACE("Device Info Delete Message ID: %d", res);
// }

// void user_post_raw_data(void)
// {
//     int res = 0;
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();
//     unsigned char raw_data[7] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

//     res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_RAW_DATA,
//                              raw_data, 7);
//     EXAMPLE_TRACE("Post Raw Data Message ID: %d", res);
// }

// #if 0
// static int user_master_dev_available(void)
// {
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();

//     if (user_example_ctx->cloud_connected && user_example_ctx->master_initialized) {
//         return 1;
//     }

//     return 0;
// }
// #endif

// void set_iotx_info()
// {
//     HAL_SetProductKey(PRODUCT_KEY);
//     HAL_SetProductSecret(PRODUCT_SECRET);
//     HAL_SetDeviceName(DEVICE_NAME);
//     HAL_SetDeviceSecret(DEVICE_SECRET);
// }

// // static int max_running_seconds = 0;
// int linkkit_main(void *paras)
// {

//     // uint64_t                        time_prev_sec = 0, time_now_sec = 0;
//     // uint64_t                        time_begin_sec = 0;
//     int res = 0;
//     iotx_linkkit_dev_meta_info_t master_meta_info;
//     user_example_ctx_t *user_example_ctx = user_example_get_ctx();
// #if defined(__UBUNTU_SDK_DEMO__)
//     int argc = ((app_main_paras_t *)paras)->argc;
//     char **argv = ((app_main_paras_t *)paras)->argv;

//     if (argc > 1)
//     {
//         int tmp = atoi(argv[1]);

//         if (tmp >= 60)
//         {
//             max_running_seconds = tmp;
//             EXAMPLE_TRACE("set [max_running_seconds] = %d seconds\n", max_running_seconds);
//         }
//     }
// #endif

// #if !defined(WIFI_PROVISION_ENABLED) || !defined(BUILD_AOS)
//     set_iotx_info();
// #endif

//     memset(user_example_ctx, 0, sizeof(user_example_ctx_t));

//     IOT_SetLogLevel(IOT_LOG_DEBUG);

//     /* Register Callback */
//     IOT_RegisterCallback(ITE_CONNECT_SUCC, user_connected_event_handler);
//     IOT_RegisterCallback(ITE_DISCONNECTED, user_disconnected_event_handler);
//     IOT_RegisterCallback(ITE_RAWDATA_ARRIVED, user_down_raw_data_arrived_event_handler);
//     IOT_RegisterCallback(ITE_SERVICE_REQUST, user_service_request_event_handler);
//     IOT_RegisterCallback(ITE_PROPERTY_SET, user_property_set_event_handler);
//     IOT_RegisterCallback(ITE_PROPERTY_GET, user_property_get_event_handler);
//     IOT_RegisterCallback(ITE_REPORT_REPLY, user_report_reply_event_handler);
//     IOT_RegisterCallback(ITE_TRIGGER_EVENT_REPLY, user_trigger_event_reply_event_handler);
//     IOT_RegisterCallback(ITE_TIMESTAMP_REPLY, user_timestamp_reply_event_handler);
//     IOT_RegisterCallback(ITE_INITIALIZE_COMPLETED, user_initialized);
//     IOT_RegisterCallback(ITE_FOTA, user_fota_event_handler);
//     IOT_RegisterCallback(ITE_COTA, user_cota_event_handler);

//     memset(&master_meta_info, 0, sizeof(iotx_linkkit_dev_meta_info_t));
//     memcpy(master_meta_info.product_key, PRODUCT_KEY, strlen(PRODUCT_KEY));
//     memcpy(master_meta_info.product_secret, PRODUCT_SECRET, strlen(PRODUCT_SECRET));
//     memcpy(master_meta_info.device_name, DEVICE_NAME, strlen(DEVICE_NAME));
//     memcpy(master_meta_info.device_secret, DEVICE_SECRET, strlen(DEVICE_SECRET));

//     /* Choose Login Server, domain should be configured before IOT_Linkkit_Open() */
// #if USE_CUSTOME_DOMAIN
//     IOT_Ioctl(IOTX_IOCTL_SET_MQTT_DOMAIN, (void *)CUSTOME_DOMAIN_MQTT);
//     IOT_Ioctl(IOTX_IOCTL_SET_HTTP_DOMAIN, (void *)CUSTOME_DOMAIN_HTTP);
// #else
//     int domain_type = IOTX_CLOUD_REGION_SHANGHAI;
//     IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);
// #endif

//     /* Choose Login Method */
//     int dynamic_register = 0;
//     IOT_Ioctl(IOTX_IOCTL_SET_DYNAMIC_REGISTER, (void *)&dynamic_register);

//     /* Choose Whether You Need Post Property/Event Reply */
//     int post_event_reply = 1;
//     IOT_Ioctl(IOTX_IOCTL_RECV_EVENT_REPLY, (void *)&post_event_reply);

//     /* Create Master Device Resources */
//     user_example_ctx->master_devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_MASTER, &master_meta_info);
//     if (user_example_ctx->master_devid < 0)
//     {
//         EXAMPLE_TRACE("IOT_Linkkit_Open Failed\n");
//         IOT_Linkkit_Close(user_example_ctx->master_devid);
//         return -1;
//     }

//     /* Start Connect Aliyun Server */
//     res = IOT_Linkkit_Connect(user_example_ctx->master_devid);
//     if (res < 0)
//     {
//         EXAMPLE_TRACE("IOT_Linkkit_Connect Failed\n");
//         IOT_Linkkit_Close(user_example_ctx->master_devid);
//         return -1;
//     }

//     // time_begin_sec = user_update_sec();
//     static uint32_t TIME;
//     static uint32_t LAST_TIME = 0;
//     while (1)
//     {
//         IOT_Linkkit_Yield(USER_EXAMPLE_YIELD_TIMEOUT_MS);

        
// #if 0
//         time_now_sec = user_update_sec();
//         if (time_prev_sec == time_now_sec) {
//             continue;
//         }
//         if (max_running_seconds && (time_now_sec - time_begin_sec > max_running_seconds)) {
//             EXAMPLE_TRACE("Example Run for Over %d Seconds, Break Loop!\n", max_running_seconds);
//             break;
//         }

//         /* Post Proprety Example */
//         if (time_now_sec % 11 == 0 && user_master_dev_available()) 
//         {
//             user_post_property();
//         }
//         // /* Post Event Example */
//         // if (time_now_sec % 17 == 0 && user_master_dev_available()) {
//         //     user_post_event();
//         // }

//         // /* Device Info Update Example */
//         // if (time_now_sec % 23 == 0 && user_master_dev_available()) {
//         //     user_deviceinfo_update();
//         // }

//         // /* Device Info Delete Example */
//         // if (time_now_sec % 29 == 0 && user_master_dev_available()) {
//         //     user_deviceinfo_delete();
//         // }

//         // /* Post Raw Example */
//         // if (time_now_sec % 37 == 0 && user_master_dev_available()) {
//         //     user_post_raw_data();
//         // }

//         time_prev_sec = time_now_sec;
// #endif
//     }

//     IOT_Linkkit_Close(user_example_ctx->master_devid);

//     IOT_DumpMemoryStats(IOT_LOG_DEBUG);
//     IOT_SetLogLevel(IOT_LOG_NONE);

//     return 0;
// }
// #endif
