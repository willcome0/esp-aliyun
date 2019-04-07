/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "iot_import.h"
#include "iot_export.h"
#include "app_entry.h"
#include "esp_system.h"
#include "esp_log.h"


#define PRODUCT_KEY "a1y0bg1jCBS"
#define PRODUCT_SECRET "tB1GZDwIeGNpav5q"
#define DEVICE_NAME "Watch1"
#define DEVICE_SECRET "mGQLNVMGaZpQKI9HImJFH8T5bUbx8TWY"

/* These are pre-defined topics */
#define TOPIC_UPDATE "/" PRODUCT_KEY "/" DEVICE_NAME "/user/update"
// #define TOPIC_ERROR "/" PRODUCT_KEY "/" DEVICE_NAME "/user/update/error"
// #define TOPIC_GET "/" PRODUCT_KEY "/" DEVICE_NAME "/user/get"
#define TOPIC_DATA "/" PRODUCT_KEY "/" DEVICE_NAME "/user/data"

#define MQTT_MSGLEN (1024)



static int user_argc;
static char **user_argv;


void event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
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

    mqtt_params.handle_event.h_fp = event_handle;
    mqtt_params.handle_event.pcontext = NULL;

    /* Construct a MQTT client with specify parameter */
    void *pclient;
    pclient = IOT_MQTT_Construct(&mqtt_params);
    if (NULL == pclient)
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

    rc = IOT_MQTT_Publish(pclient, TOPIC_UPDATE, &topic_msg);
    if (rc < 0)
    {
        IOT_MQTT_Destroy(&pclient);
        ESP_LOGI("#my", "发送消息时发生错误");
        return -1;
    }
    ESP_LOGI("#my", "主题名：%s\n 载荷数据：%s\n 返回值：%d", TOPIC_UPDATE, topic_msg.payload, rc);
    
    
    
    
    /*********************订阅主题**********************************/
    ESP_LOGI("#my", "订阅主题：%s", TOPIC_DATA);
    rc = IOT_MQTT_Subscribe(pclient, TOPIC_DATA, IOTX_MQTT_QOS1, _demo_message_arrive, NULL);
    if (rc < 0)
    {
        IOT_MQTT_Destroy(&pclient);
        ESP_LOGI("#my", "主题订阅失败，返回值为%d", rc);
        return -1;
    }

    IOT_MQTT_Yield(pclient, 200);

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

    rc = IOT_MQTT_Publish(pclient, TOPIC_DATA, &topic_msg);
    ESP_LOGI("#my", "主题名：%s\n 载荷数据：%s\n 返回值：%d", TOPIC_DATA, topic_msg.payload, rc);
    IOT_MQTT_Yield(pclient, 200);

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

        rc = IOT_MQTT_Publish(pclient, TOPIC_DATA, &topic_msg);
        if (rc < 0)
        {
            ESP_LOGI("#循环--", "发送消息时发生错误");
        }
        ESP_LOGI("#循环--", "信息包：ID%u", (uint32_t)rc);
        ESP_LOGI("#循环--", "载荷数据：%s", msg_pub);

        /* handle the MQTT packet received from TCP or SSL connection */
        IOT_MQTT_Yield(pclient, 3000);

        /* infinite loop if running with 'loop' argument */
        if (user_argc >= 2 && !strcmp("loop", user_argv[1]))
        {
            // HAL_SleepMs(2000);
            // cnt = 0;
        }
        ESP_LOGI("#循环--", "最小空闲字节数：%u 空闲字节数：%u", esp_get_minimum_free_heap_size(), esp_get_free_heap_size());
    } while (cnt);

    IOT_MQTT_Yield(pclient, 200);

    IOT_MQTT_Unsubscribe(pclient, TOPIC_DATA);

    IOT_MQTT_Yield(pclient, 200);

    IOT_MQTT_Destroy(&pclient);

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
