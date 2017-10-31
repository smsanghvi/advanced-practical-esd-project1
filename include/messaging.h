/******************************************************
*   File: messaging.h
*
​* ​ ​ The MIT License (MIT)
*	Copyright (C) 2017 by Snehal Sanghvi and Rishi Soni
*   Redistribution,​ ​ modification​ ​ or​ ​ use​ ​ of​ ​ this​ ​ software​ ​ in​ ​ source​ ​ or​ ​ binary
​* ​ ​ forms​ ​ is​ ​ permitted​ ​ as​ ​ long​ ​ as​ ​ the​ ​ files​ ​ maintain​ ​ this​ ​ copyright.​ ​ Users​ ​ are
​* ​ ​ permitted​ ​ to​ ​ modify​ ​ this​ ​ and​ ​ use​ ​ it​ ​ to​ ​ learn​ ​ about​ ​ the​ ​ field​ ​ of​ ​ embedded
​* ​ ​ software.​ ​ The authors​ and​ ​ the​ ​ University​ ​ of​ ​ Colorado​ ​ are​ ​ not​ ​ liable​ ​ for
​* ​ ​ any​ ​ misuse​ ​ of​ ​ this​ ​ material.
*
*
*   Authors: Snehal Sanghvi and Rishi Soni
*   Date Edited: 24 Oct 2017
*
*   Description: Header file for messaging interface
*
*
********************************************************/

#ifndef _MESSAGING_H
#define _MESSAGING_H

#include <time.h>

#define MSG_QUEUE_RESPONSE "/log_queue_response"
#define MSG_QUEUE_RESPONSE_COPY "/log_queue_response_copy"
#define MSG_QUEUE_REQUEST "/queue_request"

#define MAX_MESSAGE_LENGTH 1000
#define ERROR (-1)
#define OK (0)

/*Enumerating the threads*/
typedef enum task_id_t{
    main_thread = 1,
    temp_thread = 2,
    lght_thread = 3,
    log_thread = 4,
    dec_thread = 5
}task_id;


/*enumerating the message type */
typedef enum message_type_t{
    HEARTBEAT_MESSAGE,
    SYSTEM_FAILURE_MESSAGE,
    LOG_MESSAGE,
    SYSTEM_INIT_MESSAGE,
    REQUEST_MESSAGE
}message_type;


/*Enumerating different logging levels*/
typedef enum log_level_t{
    LOG_CRITICAL_FAILURE,
    LOG_SYSTEM_FAILURE,
    LOG_MODULE_STARTUP,
    LOG_INFO_DATA
}log_l;


/*Struct of the message to be sent or received*/
typedef struct message_t{
    log_l log_level;
    task_id source_task;
    message_type type;
    struct timeval t;   //for timestamps
    void *data;
}message;


/*Enumerating the errors for checksum*/
typedef enum ERR_t {
	CHKSUM_FAIL = 1,
	CHKSUM_PASS = 2
}ERR;



/* Function to decode the incoming message packet*/
void decode_message (void);

/*Function to validate the checksum at the end of each message using addition algorithm*/
ERR checksum_validate (void);

#endif