/**
 *
 * 用于连接web服务，与服务器通讯
 *
 * @author Darren.X
 * @date 2024-01-01 (creation date)
 * @date 2024-09-14 (last modification date)
 */

#ifndef APP_WEB_H
#define APP_WEB_H

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* */
typedef struct 
{    

    
    const char *bot_id;  //机器人ID
    const char* user_id; //用户ID

    char* chat_id; //聊天ID
    char* conversation_id; //会话ID
    char* message_id; //消息ID
    char* req;  //请求
    char* res;  //返回
    char* mp3;  //音频
    char* status; //状态

} app_chat;


typedef enum{

    APP_API_CHAT,
    APP_API_CHAT_HIS,
    APP_API_RETRIEVE,
    APP_API_MESSAGE,

} APP_API;


/*
* @brief 开始聊天会话
* @param chat 基本信息
* @param message 发送的文字
*/ 
void app_chat_start(char* message);

bool app_post_chat(APP_API api);

char* app_handle_request(APP_API api,char* json);

void app_handle_respone(APP_API api,char* res);

char* json_extract_id(const char* json);

char* json_extract_conversation_id (const char* json);

char* json_extract_status(const char* json);

char* json_extract_subtitle (const char* json);

void app_memory_monitor(const char* tag);

#ifdef __cplusplus
}
#endif

#endif