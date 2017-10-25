/******************************************************
*   File: tasks.c
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
*   Description: Source file for tasks
*
*
********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <fcntl.h>            /* Defines O_* constants */
#include <sys/stat.h> 
#include "messaging.h"
#include <mqueue.h>

#define MSG_QUEUE_LOG "/my_queue_log"

pthread_t temperature_thread;
pthread_t light_thread;

static struct mq_attr mq_attr_log;
static mqd_t mqd_log;


void *temp_thread_fn(void *args){
	printf("In temperature thread function.\n");
	pthread_exit(NULL);
}

void *light_thread_fn(void *args){
	printf("In light thread function.\n");
	pthread_exit(NULL);
}


int main(){

	pthread_attr_t attr;
	uint32_t length_msg_struct = 0;
	message msg;
	length_msg_struct = sizeof(msg);

	//initializing message queue attributes
	mq_attr_log.mq_maxmsg = 100;
	mq_attr_log.mq_msgsize = length_msg_struct;

	mqd_log = mq_open(MSG_QUEUE_LOG, \
						O_CREAT | O_RDWR | O_NONBLOCK, \
						0666, \
						&mq_attr_log);

	pthread_attr_init(&attr);

	//spawn temperature thread
	if(pthread_create(&temperature_thread, &attr, (void*)&temp_thread_fn, NULL)){
        printf("Failed to create temperature thread.\n");
	}
 
 	printf("Temperature thread spawned.\n");

	//spawn light thread
	if(pthread_create(&light_thread, &attr, (void*)&light_thread_fn, NULL)){
        printf("Failed to create light thread.\n");
	}
 
 	printf("Light thread spawned.\n");
	

	//join temperature and light threads
 	pthread_join(temperature_thread, NULL);
 	pthread_join(light_thread, NULL);

	return 0;
}
