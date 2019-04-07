/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#ifdef DEPRECATED_LINKKIT
#include "deprecated/solo.c"
#else
#include "stdio.h"
#include "iot_export_linkkit.h"
#include "cJSON.h"
#include "app_entry.h"
#include "light_control.h"
#include "esp_log.h"
#include "iot_export_linkkit.h"
#include "esp_system.h"


#if defined(OTA_ENABLED) && defined(BUILD_AOS)
#include "ota_service.h"
#endif

static const char *TAG = "my";
#define USE_CUSTOME_DOMAIN (0)

// for demo only
#define PRODUCT_KEY "a1y0bg1jCBS"
#define PRODUCT_SECRET "tB1GZDwIeGNpav5q"
#define DEVICE_NAME "Watch1"
#define DEVICE_SECRET "mGQLNVMGaZpQKI9HImJFH8T5bUbx8TWY"

#if USE_CUSTOME_DOMAIN
#define CUSTOME_DOMAIN_MQTT "iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define CUSTOME_DOMAIN_HTTP "iot-auth.cn-shanghai.aliyuncs.com"
#endif

#define USER_EXAMPLE_YIELD_TIMEOUT_MS (200)

#define EXAMPLE_TRACE(...)                                      \
    do                                                          \
    {                                                           \
        HAL_Printf("\033[1;32;40m%s.%d: ", __func__, __LINE__); \
        HAL_Printf(__VA_ARGS__);                                \
        HAL_Printf("\033[0m\r\n");                              \
    } while (0)



/*****************MQTT相关***********************/
#define TOPIC_UPDATE "/" PRODUCT_KEY "/" DEVICE_NAME "/user/update"
#define TOPIC_DATA "/" PRODUCT_KEY "/" DEVICE_NAME "/user/data"
#define TOPIC_ENENT_PROPERTY_POST "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"
#define ALINK_BODY_FORMAT "{\"id\":\"123\",\"version\":\"1.0\",\"method\":\"thing.event.property.post\",\"params\":%s}"

#define MQTT_MSGLEN (1024)


void *g_p_mqtt_client;


static int user_argc;
static char **user_argv;

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
        ESP_LOGI("#事件回调--", "主题消息到达但没有任何相关句柄：\r\n主题=%.*s, 主题数据=%.*s",
                      topic_info->topic_len,
                      topic_info->ptopic,
                      topic_info->payload_len,
                      topic_info->payload);
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


































// const iotx_linkkit_dev_meta_info_t subdev = 
//     {
//         "a1vjaZ9ZGUx",
//         "RLOfJWbyHcLCN1vj",
//         "slave",
//         "4R0jJyO4ukQWoJMztsxLlwNd8IhBzAni"
//     };

// // 添加子设备
// static int example_add_subdev(iotx_linkkit_dev_meta_info_t *meta_info)
// {
//     int res = 0, devid = -1;
//     ESP_LOGI(TAG, "子设备准备打开\n");
//     devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_SLAVE, meta_info);
//     if (devid == FAIL_RETURN) {
//         ESP_LOGI(TAG, "子设备打开失败\n");
//         return FAIL_RETURN;
//     }
//     ESP_LOGI(TAG, "子设备打开成功, devid = %d\n", devid);

//     res = IOT_Linkkit_Connect(devid);
//     if (res == FAIL_RETURN) {
//         ESP_LOGI(TAG, "子设备连接失败\n");
//         return res;
//     }
//     ESP_LOGI(TAG, "子设备连接成功，devid = %d\n", devid);

//     res = IOT_Linkkit_Report(devid, ITM_MSG_LOGIN, NULL, 0);
//     if (res == FAIL_RETURN) {
//         ESP_LOGI(TAG, "子设备登陆失败\n");
//         return res;
//     }
//     ESP_LOGI(TAG, "子设备登陆成功，devid = %d\n", devid);
//     return res;
// }





typedef struct
{
    int master_devid;        // 设备句柄、主设备号
    int cloud_connected;     // 云端已连接标志
    int master_initialized;  // 主设备初始化标志
} user_example_ctx_t;

static user_example_ctx_t g_user_example_ctx;

static user_example_ctx_t *user_example_get_ctx(void)
{
    return &g_user_example_ctx;
}

void *example_malloc(size_t size)
{
    return HAL_Malloc(size);
}

void example_free(void *ptr)
{
    HAL_Free(ptr);
}

static int user_connected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    ESP_LOGI(TAG, "云端已连接");
    user_example_ctx->cloud_connected = 1;
// #if defined(OTA_ENABLED) && defined(BUILD_AOS)
//     ota_service_init(NULL);
// #endif
    return 0;
}

static int user_disconnected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    ESP_LOGI(TAG, "云端已断开连接");

    user_example_ctx->cloud_connected = 0;

    return 0;
}

static int user_down_raw_data_arrived_event_handler(const int devid, const unsigned char *payload,
                                                    const int payload_len)
{
    ESP_LOGI(TAG, "Down Raw Message, Devid: %d, Payload Length: %d", devid, payload_len);
    return 0;
}


/*
    当收到服务调用请求时, 会进入如下回调函数, 以下代码使用了cJSON来解析服务请求的参数值
*/
static int user_service_request_event_handler(const int devid, const char *serviceid, const int serviceid_len,
                                              const char *request, const int request_len,
                                              char **response, int *response_len)
{
    ESP_LOGI(TAG, "接收到服务调用请求");
    int contrastratio = 0, to_cloud = 0;
    cJSON *root = NULL, *item_transparency = NULL, *item_from_cloud = NULL;
    ESP_LOGI(TAG, "Service Request Received, Devid: %d, Service ID: %.*s, Payload: %s", devid, serviceid_len,
                  serviceid,
                  request);

    /* Parse Root */
    root = cJSON_Parse(request);
    if (root == NULL || !cJSON_IsObject(root))
    {
        ESP_LOGI(TAG, "JSON Parse Error");
        return -1;
    }

    if (strlen("Custom") == serviceid_len && memcmp("Custom", serviceid, serviceid_len) == 0)
    {
        /* Parse Item */
        const char *response_fmt = "{\"Contrastratio\":%d}";
        item_transparency = cJSON_GetObjectItem(root, "transparency");
        if (item_transparency == NULL || !cJSON_IsNumber(item_transparency))
        {
            cJSON_Delete(root);
            return -1;
        }
        ESP_LOGI(TAG, "transparency: %d", item_transparency->valueint);
        contrastratio = item_transparency->valueint + 1;

        /* Send Service Response To Cloud */
        *response_len = strlen(response_fmt) + 10 + 1;
        *response = HAL_Malloc(*response_len);
        if (*response == NULL)
        {
            ESP_LOGI(TAG, "Memory Not Enough");
            return -1;
        }
        memset(*response, 0, *response_len);
        HAL_Snprintf(*response, *response_len, response_fmt, contrastratio);
        *response_len = strlen(*response);
    }
    else if (strlen("SyncService") == serviceid_len && memcmp("SyncService", serviceid, serviceid_len) == 0)
    {
        /* Parse Item */
        const char *response_fmt = "{\"ToCloud\":%d}";
        item_from_cloud = cJSON_GetObjectItem(root, "FromCloud");
        if (item_from_cloud == NULL || !cJSON_IsNumber(item_from_cloud))
        {
            cJSON_Delete(root);
            return -1;
        }
        ESP_LOGI(TAG, "FromCloud: %d", item_from_cloud->valueint);
        to_cloud = item_from_cloud->valueint + 1;

        /* Send Service Response To Cloud */
        *response_len = strlen(response_fmt) + 10 + 1;
        *response = HAL_Malloc(*response_len);
        if (*response == NULL)
        {
            ESP_LOGI(TAG, "Memory Not Enough");
            return -1;
        }
        memset(*response, 0, *response_len);
        HAL_Snprintf(*response, *response_len, response_fmt, to_cloud);
        *response_len = strlen(*response);
    }
    cJSON_Delete(root);

    return 0;
}


static bool LED_STATE = 0;
static void user_parse_cloud_cmd(const char *request, const int request_len)
{
    if (request == NULL || request_len <= 0)
    {
        return;
    }

    char *ptr = NULL;
    // TODO: parse it as fixed format JSON string
    if (strstr(request, "LightStatus") != NULL)
    { // {"LightSwitch":0}    // 修改
        ptr = strstr(request, ":");
        ptr++;
        bool on = atoi(ptr);
        LED_STATE = on;
        if (on)
        {
            ESP_LOGI(TAG, "LED 开");
            set_light_status(on);
        }
        else
        {

            ESP_LOGI(TAG, "LED 关");
            set_light_status(on);
        }
    }
    else
    {
        ESP_LOGW(TAG, "不支持的命令");
    }
}

////////////////////////////////////////////////////////////////////////////////
#define PROPERTY_POWERSWITCH            "PowerSwitch"
#define PROPERTY_COUNTDOWN              "CountDown"
#define PROPERTY_ITEM_TIMELEFT          "TimeLeft"
#define PROPERTY_ITEM_ISRUNNING         "IsRunning"
#define PROPERTY_ITEM_POWERSWITCH       "PowerSwitch"
#define PROPERTY_ITEM_TIMESTAMP         "Timestamp"

typedef struct _app_context {
    int         devid;
    int         inited;
    int         connected;

    void        *timerHandle;
    char        timestamp[20];
    int         powerSwitch_Actual;
    int         powerSwitch_Target;
} app_context_t;
static app_context_t *app_context;

static void app_set_context(app_context_t *ctx)
{
    app_context = ctx;
}

static void *app_get_context(void)
{
    return app_context;
}
static int app_post_countdown(int isrun, int timelf, int pwrsw, char *timestamp, app_context_t *app_ctx)
{
    int ret = -1;
    char *payload = NULL;
    cJSON *root, *prop;

    root = cJSON_CreateObject();
    if (root == NULL) {
        return ret;
    }

    prop = cJSON_CreateObject();
    if (prop == NULL) {
        cJSON_Delete(root);
        return ret;
    }

    cJSON_AddItemToObject(root, PROPERTY_COUNTDOWN, prop);
    cJSON_AddNumberToObject(prop, PROPERTY_ITEM_ISRUNNING, isrun);
    cJSON_AddNumberToObject(prop, PROPERTY_ITEM_TIMELEFT, timelf);
    cJSON_AddNumberToObject(prop, PROPERTY_ITEM_POWERSWITCH, pwrsw);
    cJSON_AddStringToObject(prop, PROPERTY_ITEM_TIMESTAMP, timestamp);

    payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    /* Post the CountDown property */
    if (payload == NULL) {
        return ret;
    }
    ret = IOT_Linkkit_Report(app_ctx->devid, ITM_MSG_POST_PROPERTY, (unsigned char *)payload, strlen(payload));
    HAL_Free(payload);
    if (ret < 0) {
        EXAMPLE_TRACE("app post property \"CountDown\" failed");
        return ret;
    }
    EXAMPLE_TRACE("app post property \"CountDown\" succeed, msgID = %d\r\n", ret);

    return ret;
}

/*
 * Property PowerSwitch paylaod construction and post
 */
static int app_post_powerswitch(int pwrsw, app_context_t *app_ctx)
{
    int ret = -1;
    char *payload = NULL;
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ret;
    }

    cJSON_AddNumberToObject(root, PROPERTY_POWERSWITCH, pwrsw);
    payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    /* Post the PowerSwitch property */
    if (payload == NULL) {
        return ret;
    }

    ret = IOT_Linkkit_Report(app_ctx->devid, ITM_MSG_POST_PROPERTY, (unsigned char *)payload, strlen(payload));
    HAL_Free(payload);
    if (ret < 0) {
        EXAMPLE_TRACE("app post property \"PowerSwitch\" failed");
        return ret;
    }
    EXAMPLE_TRACE("app post property \"PowerSwitch\" secceed, msgID = %d\r\n", ret);

    return ret;
}

static void app_timer_expired_handle(void *ctx)
{
    app_context_t *app_ctx = app_get_context();
    if (NULL == app_ctx) {
        EXAMPLE_TRACE("can't get app context, just return");
        return;
    }

    EXAMPLE_TRACE("app timer expired!\r\n");

    /* Trigger powerswitch action, just set value here */
    app_ctx->powerSwitch_Actual = app_ctx->powerSwitch_Target;

    /* Set PowerSwitch property then post */
    app_post_powerswitch(app_ctx->powerSwitch_Actual, app_ctx);

    /* Set CountDown property then post */
    app_post_countdown(0, 0, app_ctx->powerSwitch_Target, app_ctx->timestamp, app_ctx);
}

static void *app_timer_open(void *ctx)
{
    return HAL_Timer_Create("App_CountDown", app_timer_expired_handle, ctx);
}

static void app_timer_close(void *timer)
{
    HAL_Timer_Delete(timer);
}

static void app_timer_stop(void *timer)
{
    HAL_Timer_Stop(timer);
    EXAMPLE_TRACE("app timer stop");
}

static void app_timer_start(void *timer, int s)
{
    HAL_Timer_Start(timer, s * 1000);
    EXAMPLE_TRACE("app timer start");
}


static int property_set_handle(const int devid, const char *payload, const int payload_len)
{
    ESP_LOGI(TAG, "--------新加的开始-----------");
    int ret = -1;
    int powerSwitch = 0;
    int timeLeft = 0;
    int isRunning = 0;
    cJSON *root, *prop, *item;
    app_context_t *app_ctx = app_get_context();
    if (app_ctx == NULL) {
        return ret;
    }

    ESP_LOGI(TAG, "property set, payload: \"%s\"", payload);

    root = cJSON_Parse(payload);
    if (root == NULL) {
        ESP_LOGI(TAG, "property set payload is not JSON format");
        return ret;
    }

    prop = cJSON_GetObjectItem(root, PROPERTY_COUNTDOWN);
    if (prop == NULL || !cJSON_IsObject(prop)) {
        prop = cJSON_GetObjectItem(root, PROPERTY_POWERSWITCH);
        if (prop == NULL || !cJSON_IsNumber(prop)) {
            cJSON_Delete(root);
            return ret;
        }
        ESP_LOGI(TAG, "property is PowerSwitch");

        app_ctx->powerSwitch_Actual = prop->valueint;
        ESP_LOGI(TAG, "PowerSwitch actual value set to %d\r\n", app_ctx->powerSwitch_Actual);
        cJSON_Delete(root);

        /* Post PowerSwitch value */
        app_post_powerswitch(app_ctx->powerSwitch_Actual, app_ctx);

        return ret;
    }

    ESP_LOGI(TAG, "property is CountDown");

    /* Get powerSwitch value */
    item = cJSON_GetObjectItem(prop, PROPERTY_ITEM_POWERSWITCH);
    if (item != NULL && cJSON_IsNumber(item)) {
        powerSwitch = item->valueint;
        ESP_LOGI(TAG, "PowerSwitch target value is %d", powerSwitch);
    } else {
        cJSON_Delete(root);
        return ret;
    }

    /* Get timeLeft value */
    item = cJSON_GetObjectItem(prop, PROPERTY_ITEM_TIMELEFT);
    if (item != NULL && cJSON_IsNumber(item)) {
        timeLeft = item->valueint;
        ESP_LOGI(TAG, "TimeLeft is %d", timeLeft);
    } else {
        cJSON_Delete(root);
        return ret;
    }

    /* Get isRunning value */
    item = cJSON_GetObjectItem(prop, PROPERTY_ITEM_ISRUNNING);
    if (item != NULL && cJSON_IsNumber(item)) {
        isRunning = item->valueint;
        ESP_LOGI(TAG, "IsRunning is %d", isRunning);
    } else {
        cJSON_Delete(root);
        return ret;
    }

    /* Get timeStamp value */
    item = cJSON_GetObjectItem(prop, PROPERTY_ITEM_TIMESTAMP);
    if (item != NULL && cJSON_IsString(item)) {
        int len = strlen(item->valuestring);
        if (len < 20) {
            memset(app_ctx->timestamp, 0, sizeof(app_ctx->timestamp));
            memcpy(app_ctx->timestamp, item->valuestring, len);
            ESP_LOGI(TAG, "Timestamp is %s", app_ctx->timestamp);
        }
        else {
            ESP_LOGI(TAG, "Timestamp string error");
            cJSON_Delete(root);
            return ret;                    
        }
    } else {
        cJSON_Delete(root);
        return ret;
    }

    /* Delete Json */
    cJSON_Delete(root);

    /* Start or stop timer according to "IsRunning" */
    if (isRunning == 1) {
        app_timer_start(app_ctx->timerHandle, timeLeft);
        /* temp powerswitch value to app context */
        app_ctx->powerSwitch_Target = powerSwitch;
    } else if (isRunning == 0) {
        app_timer_stop(app_ctx->timerHandle);
    }

    /* Just echo the CountDown property */
    ret = IOT_Linkkit_Report(app_ctx->devid, ITM_MSG_POST_PROPERTY, (unsigned char *)payload, payload_len);
    if (ret < 0) {
        ESP_LOGI(TAG, "app post property \"CountDown\" failed");
        return ret;
    }
    ESP_LOGI(TAG, "app post property \"CountDown\" succeed, msgID = %d\r\n", ret);

    /* Post the PowerSwitch property, powerSwitch_Actual used */
    /* Post PowerSwitch value */
    app_post_powerswitch(app_ctx->powerSwitch_Actual, app_ctx);

    

    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* 
    获取云端设置的属性值, 并原样将收到的数据发回给云端, 这样可以更新在云端的设备属性值, 
    用户可在此处对收到的属性值进行处理。
*/
static int user_property_set_event_handler(const int devid, const char *request, const int request_len)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    ESP_LOGI(TAG, "Property Set Received, Devid: %d, Request: %s", devid, request);

    ESP_LOGI(TAG, "收到的消息：%s", request);
    ESP_LOGI(TAG, "收到的消息长度：%d", request_len);

    user_parse_cloud_cmd(request, request_len);

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
                             (unsigned char *)request, request_len);
    ESP_LOGI(TAG, "Post Property Message ID: %d", res);
    ESP_LOGI(TAG, "上报的属性消息ID: %d", res);

    property_set_handle(devid, request, request_len);
    ESP_LOGI(TAG, "--------新加的结束-----------");

    return 0;
}

static int user_property_get_event_handler(const int devid, const char *request, const int request_len, char **response,
                                           int *response_len)
{
    ESP_LOGI(TAG, "user_property_get_event_handler");
    cJSON *request_root = NULL, *item_propertyid = NULL;
    cJSON *response_root = NULL;
    int index = 0;
    ESP_LOGI(TAG, "Property Get Received, Devid: %d, Request: %s", devid, request);

    /* Parse Request */
    request_root = cJSON_Parse(request);
    if (request_root == NULL || !cJSON_IsArray(request_root))
    {
        ESP_LOGI(TAG, "JSON Parse Error");
        return -1;
    }

    /* Prepare Response */
    response_root = cJSON_CreateObject();
    if (response_root == NULL)
    {
        ESP_LOGI(TAG, "No Enough Memory");
        cJSON_Delete(request_root);
        return -1;
    }

    for (index = 0; index < cJSON_GetArraySize(request_root); index++)
    {
        item_propertyid = cJSON_GetArrayItem(request_root, index);
        if (item_propertyid == NULL || !cJSON_IsString(item_propertyid))
        {
            ESP_LOGI(TAG, "JSON Parse Error");
            cJSON_Delete(request_root);
            cJSON_Delete(response_root);
            return -1;
        }

        ESP_LOGI(TAG, "Property ID, index: %d, Value: %s", index, item_propertyid->valuestring);

        if (strcmp("WIFI_Tx_Rate", item_propertyid->valuestring) == 0)
        {
            cJSON_AddNumberToObject(response_root, "WIFI_Tx_Rate", 1111);
        }
        else if (strcmp("WIFI_Rx_Rate", item_propertyid->valuestring) == 0)
        {
            cJSON_AddNumberToObject(response_root, "WIFI_Rx_Rate", 2222);
        }
        else if (strcmp("RGBColor", item_propertyid->valuestring) == 0)
        {
            cJSON *item_rgbcolor = cJSON_CreateObject();
            if (item_rgbcolor == NULL)
            {
                cJSON_Delete(request_root);
                cJSON_Delete(response_root);
                return -1;
            }
            cJSON_AddNumberToObject(item_rgbcolor, "Red", 100);
            cJSON_AddNumberToObject(item_rgbcolor, "Green", 100);
            cJSON_AddNumberToObject(item_rgbcolor, "Blue", 100);
            cJSON_AddItemToObject(response_root, "RGBColor", item_rgbcolor);
        }
        else if (strcmp("HSVColor", item_propertyid->valuestring) == 0)
        {
            cJSON *item_hsvcolor = cJSON_CreateObject();
            if (item_hsvcolor == NULL)
            {
                cJSON_Delete(request_root);
                cJSON_Delete(response_root);
                return -1;
            }
            cJSON_AddNumberToObject(item_hsvcolor, "Hue", 50);
            cJSON_AddNumberToObject(item_hsvcolor, "Saturation", 50);
            cJSON_AddNumberToObject(item_hsvcolor, "Value", 50);
            cJSON_AddItemToObject(response_root, "HSVColor", item_hsvcolor);
        }
        else if (strcmp("HSLColor", item_propertyid->valuestring) == 0)
        {
            cJSON *item_hslcolor = cJSON_CreateObject();
            if (item_hslcolor == NULL)
            {
                cJSON_Delete(request_root);
                cJSON_Delete(response_root);
                return -1;
            }
            cJSON_AddNumberToObject(item_hslcolor, "Hue", 70);
            cJSON_AddNumberToObject(item_hslcolor, "Saturation", 70);
            cJSON_AddNumberToObject(item_hslcolor, "Lightness", 70);
            cJSON_AddItemToObject(response_root, "HSLColor", item_hslcolor);
        }
        else if (strcmp("WorkMode", item_propertyid->valuestring) == 0)
        {
            cJSON_AddNumberToObject(response_root, "WorkMode", 4);
        }
        else if (strcmp("NightLightSwitch", item_propertyid->valuestring) == 0)
        {
            cJSON_AddNumberToObject(response_root, "NightLightSwitch", 1);
        }
        else if (strcmp("Brightness", item_propertyid->valuestring) == 0)
        {
            cJSON_AddNumberToObject(response_root, "Brightness", 30);
        }
        else if (strcmp("LightSwitch", item_propertyid->valuestring) == 0)
        {
            cJSON_AddNumberToObject(response_root, "LightSwitch", 1);
        }
        else if (strcmp("ColorTemperature", item_propertyid->valuestring) == 0)
        {
            cJSON_AddNumberToObject(response_root, "ColorTemperature", 2800);
        }
        else if (strcmp("PropertyCharacter", item_propertyid->valuestring) == 0)
        {
            cJSON_AddStringToObject(response_root, "PropertyCharacter", "testprop");
        }
        else if (strcmp("Propertypoint", item_propertyid->valuestring) == 0)
        {
            cJSON_AddNumberToObject(response_root, "Propertypoint", 50);
        }
        else if (strcmp("LocalTimer", item_propertyid->valuestring) == 0)
        {
            cJSON *array_localtimer = cJSON_CreateArray();
            if (array_localtimer == NULL)
            {
                cJSON_Delete(request_root);
                cJSON_Delete(response_root);
                return -1;
            }

            cJSON *item_localtimer = cJSON_CreateObject();
            if (item_localtimer == NULL)
            {
                cJSON_Delete(request_root);
                cJSON_Delete(response_root);
                cJSON_Delete(array_localtimer);
                return -1;
            }
            cJSON_AddStringToObject(item_localtimer, "Timer", "10 11 * * * 1 2 3 4 5");
            cJSON_AddNumberToObject(item_localtimer, "Enable", 1);
            cJSON_AddNumberToObject(item_localtimer, "IsValid", 1);
            cJSON_AddItemToArray(array_localtimer, item_localtimer);
            cJSON_AddItemToObject(response_root, "LocalTimer", array_localtimer);
        }
    }
    cJSON_Delete(request_root);

    *response = cJSON_PrintUnformatted(response_root);
    if (*response == NULL)
    {
        ESP_LOGI(TAG, "No Enough Memory");
        cJSON_Delete(response_root);
        return -1;
    }
    cJSON_Delete(response_root);
    *response_len = strlen(*response);

    ESP_LOGI(TAG, "Property Get Response: %s", *response);

    return SUCCESS_RETURN;
}

static int user_report_reply_event_handler(const int devid, const int msgid, const int code, const char *reply,
                                           const int reply_len)
{
    ESP_LOGI(TAG, "接收到属性信息上报回复");
    // const char *reply_value = (reply == NULL) ? ("NULL") : (reply);
    // const int reply_value_len = (reply_len == 0) ? (strlen("NULL")) : (reply_len);

    ESP_LOGI(TAG, "设备句柄：%d, 信息ID：%d, 代码：%d, 回应信息：%.*s", devid, msgid, code,
                  reply_len,
                  reply);
    return 0;
}

static int user_trigger_event_reply_event_handler(const int devid, const int msgid, const int code, const char *eventid,
                                                  const int eventid_len, const char *message, const int message_len)
{
    ESP_LOGI(TAG, "接收到事件信息上报回复");
    ESP_LOGI(TAG, "设备句柄：%d, 消息ID：%d, 代码：%d, 事件ID：%.*s, 回应信息: %.*s", devid,
                  msgid, code,
                  eventid_len, eventid,
                  message_len, message);

    return 0;
}

static int user_timestamp_reply_event_handler(const char *timestamp)
{
    ESP_LOGI(TAG, "当前时间戳：%s", timestamp);

    return 0;
}

static int user_initialized(const int devid)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    ESP_LOGI(TAG, "设备已初始化, 设备ID: %d", devid);

    if (user_example_ctx->master_devid == devid)
    {
        user_example_ctx->master_initialized = 1;
    }

    return 0;
}

/** type:
  *
  * 0 - new firmware exist
  *
  */
static int user_fota_event_handler(int type, const char *version)
{
    char buffer[128] = {0};
    int buffer_length = 128;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    if (type == 0)
    {
        ESP_LOGI(TAG, "新固件版本为: %s", version);

        IOT_Linkkit_Query(user_example_ctx->master_devid, ITM_MSG_QUERY_FOTA_DATA, (unsigned char *)buffer, buffer_length);
    }

    return 0;
}

/** type:
  *
  * 0 - new config exist
  *
  */
static int user_cota_event_handler(int type, const char *config_id, int config_size, const char *get_type,
                                   const char *sign, const char *sign_method, const char *url)
{
    char buffer[128] = {0};
    int buffer_length = 128;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    if (type == 0)
    {
        ESP_LOGI(TAG, "New Config ID: %s", config_id);
        ESP_LOGI(TAG, "New Config Size: %d", config_size);
        ESP_LOGI(TAG, "New Config Type: %s", get_type);
        ESP_LOGI(TAG, "New Config Sign: %s", sign);
        ESP_LOGI(TAG, "New Config Sign Method: %s", sign_method);
        ESP_LOGI(TAG, "New Config URL: %s", url);

        IOT_Linkkit_Query(user_example_ctx->master_devid, ITM_MSG_QUERY_COTA_DATA, (unsigned char *)buffer, buffer_length);
    }

    return 0;
}

#if 1
static uint64_t user_update_sec(void)
{
    static uint64_t time_start_ms = 0;

    if (time_start_ms == 0) {
        time_start_ms = HAL_UptimeMs();
    }

    return (HAL_UptimeMs() - time_start_ms) / 1000;
}
#endif


/*
    用户可以调用IOT_Linkkit_Report()函数来上报属性，属性上报时需要
    按照云端定义的属性格式使用JSON编码后进行上报
*/
void user_post_property(void)
{
    ESP_LOGI(TAG, "上报属性信息");
    static int example_index = 0;
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    
    char *property_payload = "NULL";

    // if (example_index == 0)
    // {
        /* Normal Example */
        static bool led_state = 0;

        led_state = !led_state;
        if (led_state)
        {
            property_payload = "{\"LightStatus\":0}";
            set_light_status(0);
        }
        else
        {
            property_payload = "{\"LightStatus\":1}";
            set_light_status(1);
        }
            
        // example_index++;
    // }
    // else if (example_index == 1)
    // {
    //     /* Wrong Property ID */
    //     property_payload = "{\"LightSwitchxxxx\":1}";
    //     example_index++;
    // }
    // else if (example_index == 2)
    // {
    //     /* Wrong Value Format */
    //     property_payload = "{\"LightSwitch\":\"test\"}";
    //     example_index++;
    // }
    // else if (example_index == 3)
    // {
    //     /* Wrong Value Range */
    //     property_payload = "{\"LightSwitch\":10}";
    //     example_index++;
    // }
    // else if (example_index == 4)
    // {
    //     /* Missing Property Item */
    //     property_payload = "{\"RGBColor\":{\"Red\":45,\"Green\":30}}";
    //     example_index++;
    // }
    // else if (example_index == 5)
    // {
    //     /* Wrong Params Format */
    //     property_payload = "\"hello world\"";
    //     example_index++;
    // }
    // else if (example_index == 6)
    // {
    //     /* Wrong Json Format */
    //     property_payload = "hello world";
    //     example_index = 0;
    // }
    
    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
                             (unsigned char *)property_payload, strlen(property_payload));

    ESP_LOGI(TAG, "主设备 ID: %d", user_example_ctx->master_devid);                    
    ESP_LOGI(TAG, "主设备信息 ID: %d", res);
    ESP_LOGI(TAG, "主设备上报数据：%s", property_payload);



    
    //     if (led_state)
    //     {
    //         property_payload = "{\"LightSwitch\":0}";
    //         // set_light_status(0);
    //     }
    //     else
    //     {
    //         property_payload = "{\"LightSwitch\":1}";
    //         // set_light_status(1);
    //     }
    // int devid = -1;
    // devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_SLAVE, (iotx_linkkit_dev_meta_info_t *)&subdev);
    // // devid = _iotx_linkkit_slave_open((iotx_linkkit_dev_meta_info_t *)&subdev);

    //ESP_LOGI(TAG, "子设备返回信息 ID: %d", devid);

    // devid = 0;
    // devid = IOT_Linkkit_Report(devid, ITM_MSG_POST_PROPERTY,
    //                          (unsigned char *)property_payload, strlen(property_payload));
    // ESP_LOGI(TAG, "子设备强制信息 ID: %d", devid);
    // ESP_LOGI(TAG, "子设备上报数据：%d", led_state);
}


/*
    设备事件上报
*/
void user_post_event(void)
{
    ESP_LOGI(TAG, "上报事件信息");
    // static int example_index = 0;
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *event_id = "Error";
    char *event_payload = "NULL";

    // if (example_index == 0)
    // {
        /* Normal Example */
        event_payload = "{\"ErrorCode\":0}";
    //     example_index++;
    // }
    // else if (example_index == 1)
    // {
    //     /* Wrong Property ID */
    //     event_payload = "{\"ErrorCodexxx\":0}";
    //     example_index++;
    // }
    // else if (example_index == 2)
    // {
    //     /* Wrong Value Format */
    //     event_payload = "{\"ErrorCode\":\"test\"}";
    //     example_index++;
    // }
    // else if (example_index == 3)
    // {
    //     /* Wrong Value Range */
    //     event_payload = "{\"ErrorCode\":10}";
    //     example_index++;
    // }
    // else if (example_index == 4)
    // {
    //     /* Wrong Value Range */
    //     event_payload = "\"hello world\"";
    //     example_index++;
    // }
    // else if (example_index == 5)
    // {
    //     /* Wrong Json Format */
    //     event_payload = "hello world";
    //     example_index = 0;
    // }

    res = IOT_Linkkit_TriggerEvent(user_example_ctx->master_devid, event_id, strlen(event_id),
                                   event_payload, strlen(event_payload));
    ESP_LOGI(TAG, "主设备 ID: %d", user_example_ctx->master_devid);
    ESP_LOGI(TAG, "上报事件 ID: %d", res);
    ESP_LOGI(TAG, "上报内容 ID: %s", event_payload);
}

void user_deviceinfo_update(void)
{
    ESP_LOGI(TAG, "设备信息更新");
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *device_info_update = "[{\"attrKey\":\"abc\",\"attrValue\":\"hello,world\"}]";

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_DEVICEINFO_UPDATE,
                             (unsigned char *)device_info_update, strlen(device_info_update));
    ESP_LOGI(TAG, "Device Info Update Message ID: %d", res);
}

void user_deviceinfo_delete(void)
{
    ESP_LOGI(TAG, "设备信息删除");
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *device_info_delete = "[{\"attrKey\":\"abc\"}]";

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_DEVICEINFO_DELETE,
                             (unsigned char *)device_info_delete, strlen(device_info_delete));
    ESP_LOGI(TAG, "Device Info Delete Message ID: %d", res);
}

void user_post_raw_data(void)
{
    ESP_LOGI(TAG, "上报原始信息");
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    unsigned char raw_data[7] = {'1', '2', '3', '4', '5', '6', '7'};

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_RAW_DATA,
                             raw_data, 7);
    ESP_LOGI(TAG, "上报原始信息ID: %d", res);
}

#if 0
static int user_master_dev_available(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    if (user_example_ctx->cloud_connected && user_example_ctx->master_initialized) {
        return 1;
    }

    return 0;
}
#endif

void set_iotx_info()
{
    HAL_SetProductKey(PRODUCT_KEY);
    HAL_SetProductSecret(PRODUCT_SECRET);
    HAL_SetDeviceName(DEVICE_NAME);
    HAL_SetDeviceSecret(DEVICE_SECRET);
}

// static int max_running_seconds = 0;
int linkkit_main(void *paras)
{


    

    /* init app context */
    app_context_t app_ctx = {0};
    app_ctx.timerHandle = app_timer_open(NULL);
    app_ctx.powerSwitch_Actual = 0;     /* assume initial status of PowerSwitch is 0 */
    app_ctx.powerSwitch_Target = 0;
    app_set_context(&app_ctx);




    int res = 0;
    iotx_linkkit_dev_meta_info_t master_meta_info;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

#if !defined(WIFI_PROVISION_ENABLED) || !defined(BUILD_AOS)
    set_iotx_info();
#endif

    memset(user_example_ctx, 0, sizeof(user_example_ctx_t));

    IOT_SetLogLevel(IOT_LOG_DEBUG);

    /* 注册回调函数 */
    IOT_RegisterCallback(ITE_CONNECT_SUCC, user_connected_event_handler);           // 连接成功
    IOT_RegisterCallback(ITE_DISCONNECTED, user_disconnected_event_handler);        // 断开连接
    IOT_RegisterCallback(ITE_RAWDATA_ARRIVED, user_down_raw_data_arrived_event_handler);
    IOT_RegisterCallback(ITE_SERVICE_REQUST, user_service_request_event_handler);   // 接收到服务调用请求
    IOT_RegisterCallback(ITE_PROPERTY_SET, user_property_set_event_handler);        // 消息接收
    IOT_RegisterCallback(ITE_PROPERTY_GET, user_property_get_event_handler);
    IOT_RegisterCallback(ITE_REPORT_REPLY, user_report_reply_event_handler);        // 接收到信息上报回复
    IOT_RegisterCallback(ITE_TRIGGER_EVENT_REPLY, user_trigger_event_reply_event_handler);
    IOT_RegisterCallback(ITE_TIMESTAMP_REPLY, user_timestamp_reply_event_handler);
    IOT_RegisterCallback(ITE_INITIALIZE_COMPLETED, user_initialized);               // 初始化完成
    IOT_RegisterCallback(ITE_FOTA, user_fota_event_handler);
    IOT_RegisterCallback(ITE_COTA, user_cota_event_handler);
    ESP_LOGI(TAG, "注册回调函数完成");

    memset(&master_meta_info, 0, sizeof(iotx_linkkit_dev_meta_info_t));
    memcpy(master_meta_info.product_key, PRODUCT_KEY, strlen(PRODUCT_KEY));
    memcpy(master_meta_info.product_secret, PRODUCT_SECRET, strlen(PRODUCT_SECRET));
    memcpy(master_meta_info.device_name, DEVICE_NAME, strlen(DEVICE_NAME));
    memcpy(master_meta_info.device_secret, DEVICE_SECRET, strlen(DEVICE_SECRET));
    ESP_LOGI(TAG, "复制三元数据完成");





/*************MQTT开始***************************/
    int rc;
    

    char msg_pub[128];

    // /* Device AUTH */
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
    ESP_LOGI("#MQTT--", "发送消息：");
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
        ESP_LOGI("#MQTT--", "发送消息时发生错误");
        return -1;
    }
    ESP_LOGI("#MQTT--", "主题名：%s\n 载荷数据：%s\n 返回值：%d", TOPIC_UPDATE, topic_msg.payload, rc);

    /*********************订阅主题**********************************/
    ESP_LOGI("#MQTT--", "订阅主题：%s", TOPIC_DATA);
    rc = IOT_MQTT_Subscribe(g_p_mqtt_client, TOPIC_DATA, IOTX_MQTT_QOS1, _demo_message_arrive, NULL);
    if (rc < 0)
    {
        IOT_MQTT_Destroy(&g_p_mqtt_client);
        ESP_LOGI("#MQTT--", "主题订阅失败，返回值为%d", rc);
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
    ESP_LOGI("#MQTT--", "主题名：%s\n 载荷数据：%s\n 返回值：%d", TOPIC_DATA, topic_msg.payload, rc);
    IOT_MQTT_Yield(g_p_mqtt_client, 200);




    /* Choose Login Server, domain should be configured before IOT_Linkkit_Open() */
// #if USE_CUSTOME_DOMAIN
//     IOT_Ioctl(IOTX_IOCTL_SET_MQTT_DOMAIN, (void *)CUSTOME_DOMAIN_MQTT);
//     IOT_Ioctl(IOTX_IOCTL_SET_HTTP_DOMAIN, (void *)CUSTOME_DOMAIN_HTTP);
// #else
    int domain_type = IOTX_CLOUD_REGION_SHANGHAI;
    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);
// #endif

    /* Choose Login Method */
    int dynamic_register = 0;
    IOT_Ioctl(IOTX_IOCTL_SET_DYNAMIC_REGISTER, (void *)&dynamic_register);

    /* Choose Whether You Need Post Property/Event Reply */
    int post_event_reply = 1;
    IOT_Ioctl(IOTX_IOCTL_RECV_EVENT_REPLY, (void *)&post_event_reply);

    /* 初始化主设备资源成功 */
    // user_example_ctx->master_devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_MASTER, &master_meta_info);
    // if (user_example_ctx->master_devid < 0)
    // {
    //     ESP_LOGI(TAG, "初始化主设备资源失败");
    //     IOT_Linkkit_Close(user_example_ctx->master_devid);
    //     return -1;
    // }
    // ESP_LOGI(TAG, "初始化主设备资源成功");
    // ESP_LOGI(TAG, "获得的设备句柄为：%d", user_example_ctx->master_devid);

    // /* 开始连接阿里云服务器 */
    // ESP_LOGI(TAG, "准备连接阿里云服务器");
    // res = IOT_Linkkit_Connect(user_example_ctx->master_devid);
    // if (res < 0)
    // {
    //     ESP_LOGI(TAG, "连接阿里云服务器失败");
    //     IOT_Linkkit_Close(user_example_ctx->master_devid);
    //     return -1;
    // }
    // ESP_LOGI(TAG, "连接阿里云服务器成功");


    static uint32_t TIME;
    static uint32_t LAST_TIME = 0;
    ESP_LOGI(TAG, "初始化完成，进入循环");


    
    int cnt = 0;
    while (1)
    {
        IOT_Linkkit_Yield(USER_EXAMPLE_YIELD_TIMEOUT_MS);

        TIME = user_update_sec();
        if (TIME-LAST_TIME>5)
        {
            LAST_TIME = TIME;

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

        /* handle the MQTT packet received from TCP or SSL connection */
        IOT_MQTT_Yield(g_p_mqtt_client, 3000);

        /* infinite loop if running with 'loop' argument */
        if (user_argc >= 2 && !strcmp("loop", user_argv[1]))
        {
            // HAL_SleepMs(2000);
            // cnt = 0;
        }
        ESP_LOGI("#循环--", "最小空闲字节数：%u 空闲字节数：%u", esp_get_minimum_free_heap_size(), esp_get_free_heap_size());
    }


    {
        memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
        if (cnt%2)
        {
            // strcpy(msg_pub, "{\"LightStatus\":1}");
            sprintf(msg_pub, ALINK_BODY_FORMAT, "{\"LightStatus\":1}");
        }
        else
        {
            // strcpy(msg_pub, "{\"LightStatus\":0}");
            sprintf(msg_pub, ALINK_BODY_FORMAT, "{\"LightStatus\":0}");
        }
            


        // if (cnt%2)
        //     strcpy(msg_pub, "{\"aliyunPk\":\"1131546874869194\",\"id\":\"2\",\"iotId\":\"PGxkpFFo19N8cdDn9OsY000100\",\"method\":\"thing.event.property.post\",\"params\":{\"LightStatus\":1},\"proxyName\":\"thing.event.property.post\",\"topic\":\"/sys/a1y0bg1jCBS/Watch1/thing/event/property/post\",\"uniMsgId\":\"1114871732860094464\",\"version\":\"1.0\"}");
        // else
        //     strcpy(msg_pub, "{\"aliyunPk\":\"1131546874869194\",\"id\":\"2\",\"iotId\":\"PGxkpFFo19N8cdDn9OsY000100\",\"method\":\"thing.event.property.post\",\"params\":{\"LightStatus\":0},\"proxyName\":\"thing.event.property.post\",\"topic\":\"/sys/a1y0bg1jCBS/Watch1/thing/event/property/post\",\"uniMsgId\":\"1114871732860094464\",\"version\":\"1.0\"}");



        topic_msg.qos = IOTX_MQTT_QOS1;
        topic_msg.retain = 0;
        topic_msg.dup = 0;
        topic_msg.payload = (void *)msg_pub;
        topic_msg.payload_len = strlen(msg_pub);

        rc = IOT_MQTT_Publish(g_p_mqtt_client, TOPIC_ENENT_PROPERTY_POST, &topic_msg);
        if (rc < 0)
        {
            IOT_MQTT_Destroy(&g_p_mqtt_client);
            ESP_LOGI("#MQTT--", "发送消息时发生错误");
            return -1;
        }
        ESP_LOGI("#MQTT--", "主题名：%s\n 载荷数据：%s\n 返回值：%d", TOPIC_ENENT_PROPERTY_POST, topic_msg.payload, rc);
    }
            // user_post_property();
            

            // user_post_event();


            // user_deviceinfo_update();
            // user_deviceinfo_delete();
            // user_post_raw_data();


            // if (0)
            // {
            //     /* Add next subdev */
            //     if (example_add_subdev((iotx_linkkit_dev_meta_info_t *)&subdev) == SUCCESS_RETURN)
            //     {
            //         ESP_LOGI(TAG, "subdev %s add succeed", subdev.device_name);
            //     }
            //     else
            //     {
            //         ESP_LOGI(TAG, "subdev %s add failed", subdev.device_name);
            //     }
            // }
        }
        


#if 0
        time_now_sec = user_update_sec();
        if (time_prev_sec == time_now_sec) {
            continue;
        }
        if (max_running_seconds && (time_now_sec - time_begin_sec > max_running_seconds)) {
            ESP_LOGI(TAG, "Example Run for Over %d Seconds, Break Loop!\n", max_running_seconds);
            break;
        }

        /* Post Proprety Example */
        if (time_now_sec % 11 == 0 && user_master_dev_available()) 
        {
            user_post_property();
        }
        /* Post Event Example */
        if (time_now_sec % 17 == 0 && user_master_dev_available()) {
            user_post_event();
        }

        /* Device Info Update Example */
        if (time_now_sec % 23 == 0 && user_master_dev_available()) {
            user_deviceinfo_update();
        }

        /* Device Info Delete Example */
        if (time_now_sec % 29 == 0 && user_master_dev_available()) {
            user_deviceinfo_delete();
        }

        /* Post Raw Example */
        if (time_now_sec % 37 == 0 && user_master_dev_available()) {
            user_post_raw_data();
        }

        time_prev_sec = time_now_sec;
#endif
    }

    IOT_Linkkit_Close(user_example_ctx->master_devid);

    IOT_DumpMemoryStats(IOT_LOG_DEBUG);
    IOT_SetLogLevel(IOT_LOG_NONE);

    return 0;
}
#endif
