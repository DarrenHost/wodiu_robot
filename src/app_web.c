/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_tls.h"
#include "esp_heap_caps.h"
#include "app_web.h"
#include "cJSON.h"



#ifdef __cplusplus
extern "C" {
#endif

// 打印内存使用情况
#define MEMORY_MONITOR 1

// wifi 和 api 配置
#define APP_ESP_WIFI_SSID "AP-Darren"
#define APP_ESP_WIFI_PASS "1q2w3e4r"
#define APP_ESP_MAXIMUM_RETRY 10
#define APP_HTTP_SERVER "https://api.coze.cn/v3/chat"
#define APP_HTTP_TOKEN "pat_Me0ixkE37mtZqH7BzCGtOUF0VfNM8uIofKS5FbERVWrqBfnGrBssiE1wIgWCgTsF"

// 事件bit 定义
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define APP_CHAT_SUCCESS_BIT BIT21
#define APP_CHAT_FAILURE_BIT BIT22

// 静态变量开始
static const char *TAG = "app";

static bool s_wifi_connectd = 0;
static int s_retry_num = 0;

static EventGroupHandle_t s_wifi_event_group;

static app_chat s_chat = {
    .bot_id = "7413997652453113897",
    .user_id = "001",
    .chat_id = NULL,
    .conversation_id = NULL,
    .status = NULL,
    .mp3 = NULL,
    .res = NULL,
};

const static char* APP_CHAT_API_URL[4] = {
    "https://api.coze.cn/v3/chat",
    "https://api.coze.cn/v3/chat?conversation_id=%s",
    "https://api.coze.cn/v3/chat/retrieve?chat_id=%s&conversation_id=%s",
    "https://api.coze.cn/v3/chat/message/list?chat_id=%s&conversation_id=%s",
};

// https ssl 证书配置
extern const uint8_t server_root_cert_pem_start[] asm("_binary_www_coze_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[] asm("_binary_www_coze_cert_pem_end");


bool app_post_chat(APP_API api)
{

    app_memory_monitor("1");

    bool result = false;
    // 预处理请求相关数据的。
    char *json_data = (char *)malloc(200);

    app_memory_monitor("2");
    char *url = app_handle_request(api, json_data);

    app_memory_monitor("3");
    ESP_ERROR_CHECK(esp_tls_set_global_ca_store(server_root_cert_pem_start, server_root_cert_pem_end - server_root_cert_pem_start));

    ESP_LOGI(TAG, "url = %s", url);
    app_memory_monitor("4");
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .use_global_ca_store = true,
    };
    app_memory_monitor("5");
    esp_http_client_handle_t client = esp_http_client_init(&config);
    app_memory_monitor("6");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    app_memory_monitor("7");
    char token[100] = "Bearer ";
    strcat(token, APP_HTTP_TOKEN);
    app_memory_monitor("8");
    esp_http_client_set_header(client, "Authorization", token);


    app_memory_monitor("9");
    esp_err_t err;
    if (strlen(json_data) > 0)
    {
        app_memory_monitor("10");
        err = esp_http_client_open(client, strlen(json_data));
    }
    else
    {
        app_memory_monitor("12");
        err = esp_http_client_open(client, 0);
    }

    if (err == ESP_OK)
    {
        app_memory_monitor("13");
        esp_http_client_write(client, json_data, strlen(json_data));
        app_memory_monitor("14");
        esp_http_client_fetch_headers(client);
        app_memory_monitor("15");
        int status_code = esp_http_client_get_status_code(client);
        app_memory_monitor("16");
        if (status_code >= 200 && status_code < 300)
        {
            app_memory_monitor("17");
            size_t content_length = esp_http_client_get_content_length(client);
            app_memory_monitor("18");
            char *response_data = (char *)malloc(content_length + 1);
            app_memory_monitor("19");
            if (response_data == NULL)
            {
                ESP_LOGE(TAG, "Memory allocation failed.");
            }
            esp_http_client_read(client, response_data, content_length);
            app_memory_monitor("20");
            response_data[content_length] = '\0';
            ESP_LOGI(TAG, "Response: %s", response_data);

            // 处理返回结果
            app_memory_monitor("21");
            app_handle_respone(api, response_data);
            app_memory_monitor("22");

            // 回收资源
            free(response_data);
            app_memory_monitor("23");
            result = true;
        }
        else
        {
            ESP_LOGE(TAG, "HTTP request failed with status code: %d", status_code);
        }
    }
    else
    {
        xEventGroupSetBits(s_wifi_event_group, APP_CHAT_FAILURE_BIT);
        ESP_LOGE(TAG, "Error opening HTTP connection: %s", esp_err_to_name(err));
    }

    // 回收资源
     
    free(url);app_memory_monitor("24");
    free(json_data);app_memory_monitor("25");
    esp_http_client_close(client);app_memory_monitor("26");
    esp_tls_free_global_ca_store();app_memory_monitor("27");
    esp_http_client_cleanup(client);app_memory_monitor("28");
    return result;
}

void app_chat_start(char *message)
{

    

    vTaskDelay(pdMS_TO_TICKS(100));

    // clear chat
    s_chat.mp3 = NULL;
    s_chat.res = NULL;
    s_chat.status = NULL;
    s_chat.req = message;

    ESP_LOGI(TAG, "create chat...");
    if (app_post_chat(APP_API_CHAT))
    {
        ESP_LOGI(TAG, "create chat success.");
        int32_t timeOut = 10;
        
        // 服务端处理中 status != completed
        while (strcmp(s_chat.status, "completed")!=0 && timeOut > 0)
        { // update chat status
            ESP_LOGI(TAG, "chat status polling... %s",s_chat.status);
            vTaskDelay(pdMS_TO_TICKS(500));
            app_post_chat(APP_API_RETRIEVE);
            timeOut--;
        }
        if (strcmp(s_chat.status, "completed")==0)
        { //

            if (app_post_chat(APP_API_MESSAGE))
            {
                ESP_LOGI(TAG,"-------");
                ESP_LOGI(TAG, "subtitle%s",s_chat.res);
                ESP_LOGI(TAG, "mp3%s",s_chat.mp3);
            }
        }
        else
        {
            ESP_LOGE(TAG, "chat error");
        }
    }
}

char *app_handle_request(APP_API api, char *json)
{

    char *url = (char *)malloc(150 * sizeof(char));
    switch (api)
    {
    case APP_API_CHAT:
        sprintf(json, "{\"bot_id\":\"%s\",\"user_id\":\"%s\",\"stream\":false,\"auto_save_history\":true,\"additional_messages\":[{\"role\":\"user\",\"content\":\"%s\",\"content_type\":\"text\"}]}\n", s_chat.bot_id, s_chat.user_id, s_chat.req);

        sprintf(url, APP_CHAT_API_URL[APP_API_CHAT]);
        break;
    case APP_API_RETRIEVE:
        sprintf(json, " ");
        sprintf(url, APP_CHAT_API_URL[APP_API_RETRIEVE], s_chat.chat_id, s_chat.conversation_id);
        break;
    case APP_API_MESSAGE:
        sprintf(json, " ");
        sprintf(url, APP_CHAT_API_URL[APP_API_MESSAGE], s_chat.chat_id, s_chat.conversation_id);
        break;
    default:
        break;
    }

    return url;
}

void app_handle_respone(APP_API api, char *res)
{

    switch (api)
    {
    case APP_API_CHAT:
        //释放上一次动态分配的内存空间
        if(s_chat.chat_id!=NULL){
            ESP_LOGE(TAG, "chat chat_id"); 
            free(s_chat.chat_id);
        }
        if(s_chat.status!=NULL){
            ESP_LOGE(TAG, "chat status");
            free(s_chat.status);
        }
        if(s_chat.conversation_id!=NULL){
            ESP_LOGE(TAG, "chat conversation_id");
            free(s_chat.conversation_id);
        }
        //提取json 中的字段值，*注意动态内存分配
        s_chat.chat_id = json_extract_id(res);
        s_chat.status = json_extract_status(res);
        s_chat.conversation_id = json_extract_conversation_id(res);
        break;
    case APP_API_RETRIEVE:
        //释放上一次动态分配的内存空间
        if(s_chat.status!=NULL){
            free(s_chat.status);
        }
        //提取json 中的字段值，*注意动态内存分配
        s_chat.status = json_extract_status(res); 

        break;
    case APP_API_MESSAGE:
        //释放上一次动态分配的内存空间
        if(s_chat.mp3!=NULL){
            free(s_chat.mp3);
        }
        if(s_chat.res!=NULL){
            free(s_chat.res);
        }
        //提取json 中的字段值，*注意动态内存分配
        json_extract_subtitle(res);
    
        break;
    default:
        break;
    }
}

char *json_extract_id(const char *json)
{
    char *start = strstr(json, "\"id\":\"");
    if (!start)
        return NULL;
    start += 6;
    char *end = strstr(start, "\"");
    if (!end)
        return NULL;
    int len = end - start;
    char *id = (char *)malloc(len + 1);
    strlcpy(id, start, len + 1);
    return id;
}

char *json_extract_conversation_id(const char *json)
{
    char *start = strstr(json, "\"conversation_id\":\"");
    if (!start)
        return NULL;
    start += 19;
    char *end = strstr(start, "\"");
    if (!end)
        return NULL;
    int len = end - start;
    char *conversationId = (char *)malloc(len + 1);
    strlcpy(conversationId, start, len + 1);
    return conversationId;
}

char *json_extract_status(const char *json)
{
    char *start = strstr(json, "\"status\":\"");
    if (!start)
        return NULL;
    start += 10;
    char *end = strstr(start, "\"");
    if (!end)
        return NULL;
    int len = end - start;
    char *status = (char *)malloc(len + 1);
    strlcpy(status, start, len+1);
    return status;
}

char *json_extract_subtitle(const char *json)
{
   
   cJSON *root = cJSON_Parse(json);

   if (root) {
       cJSON *data = cJSON_GetObjectItem(root, "data");

       if (data && cJSON_IsArray(data)) {
            int data_size = cJSON_GetArraySize(data);
            for (int i = 0; i < data_size; i++) {
                
                cJSON *element = cJSON_GetArrayItem(data, i);
                cJSON *type =  cJSON_GetObjectItem(element,"type");
                 ESP_LOGE(TAG, "11111");
                char* value = type->valuestring;
                if(strcmp(value,"tool_response")==0){
                     
                 

                    cJSON *content =  cJSON_GetObjectItem(element,"content");

                    if(content){
                            char * text = content->valuestring;
                            ESP_LOGE(TAG, "2222.0 %s",text);
                            cJSON *subNode = cJSON_Parse(text);
                            ESP_LOGE(TAG, "2222.1");
                            cJSON *node_subtitle =  cJSON_GetObjectItem(subNode,"subtitle");
                            char *subtitle = (char *)malloc(sizeof(char) * 2000);
                            ESP_LOGE(TAG, "2222 %s",node_subtitle->valuestring);

                            int len = strlen(node_subtitle->valuestring);
                            
                            ESP_LOGE(TAG, "3333");

                            strlcpy(subtitle,node_subtitle->valuestring,len); 

                            s_chat.res = subtitle;

                            cJSON *node_mp3 =  cJSON_GetObjectItem(subNode,"mp3");
                            char *mp3 = (char *)malloc(sizeof(char) * 255);
                            len = strlen(node_subtitle->valuestring);
                            strlcpy(mp3,node_mp3->valuestring,len); 
                            s_chat.mp3 = mp3;
                            
                            cJSON_Delete(subNode); 
                            break;      
                    }
                   
      
                }
            }
        }
       
       
   }
    cJSON_Delete(root); 
    return NULL;
}

void app_memory_monitor(const char* tag)
{

    #if MEMORY_MONITOR
        printf("%sCurrent Free Memory\t%d\t\t%d\n",
            tag,
            heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
            heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    #endif
}


#ifdef __cplusplus
}
#endif