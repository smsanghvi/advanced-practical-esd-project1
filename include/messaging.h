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

#define MAX_MESSAGE_LENGTH 1000

/*Enumerating the threads*/
typedef enum task_id_t{
    main_thread = 1,
    temperature_thread = 2,
    light_thread = 3,
    logger_thread = 4,
    decision_thread = 5
}task_id;


/*Struct of the message to be sent or received*/
typedef struct message_t{
    task_id task;
    uint32_t length;
    uint32_t data[MAX_MESSAGE_LENGTH];
    uint32_t checksum;
}message;


/*Enumerating different logging levels*/
typedef enum log_level_t{
	LOG_CRITICAL_FAILURE,
	LOG_SYSTEM_FAILURE,
	LOG_MODULE_STARTUP,
	LOG_INFO_DATA
}log_level;

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