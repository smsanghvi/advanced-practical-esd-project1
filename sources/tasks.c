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
#include <unistd.h>
#include <semaphore.h>

#define MSG_QUEUE_LOG "/my_queue_log"

pthread_t temperature_thread;
pthread_t light_thread;
pthread_t logger_thread;

static volatile uint32_t counter1 = 0;
static volatile uint32_t counter2 = 1000;
static volatile uint32_t counter3 = 0;
static struct mq_attr mq_attr_log, *attrp;
static mqd_t mqd_log;
sem_t temp_sem, light_sem;


void *temp_thread_fn(void *threadid){
	printf("In temperature thread function.\n");

	while(1){
		sem_wait(&temp_sem);
		mqd_log = mq_open(MSG_QUEUE_LOG, \
						O_WRONLY, \
						0666, \
						NULL);
		printf("Mq open for temperature is %d\n", mqd_log);
		if(mq_send(mqd_log, (const char *)&counter1, sizeof(counter1), 1)){
			printf("Temperature thread could not send data.\n");
		}
		printf("Sent %d\n", counter1++);
		mq_close(mqd_log);
		sem_post(&light_sem);
	}
	//pthread_exit(NULL);
}

void *light_thread_fn(void *threadid){
	printf("In light thread function.\n");
	while(1){
		sem_wait(&light_sem);
		mqd_log = mq_open(MSG_QUEUE_LOG, \
						O_WRONLY, \
						0666, \
						NULL);
		printf("Mq open for light is %d\n", mqd_log);
		if(mq_send(mqd_log, (const char *)&counter2, sizeof(counter2), 1)){
			printf("Light thread could not send data.\n");
		}	
		printf("Sent %d\n", counter2++);
		mq_close(mqd_log);
		sem_post(&temp_sem);
	}
	//pthread_exit(NULL);
}


void *logger_thread_fn(void *threadid){
	printf("In logger thread function.\n");
	while(1){
		printf("In logger thread function.\n");

		mqd_log = mq_open(MSG_QUEUE_LOG, \
						O_RDONLY, \
						0666, \
						NULL);
		if(!mq_receive(mqd_log, (char *)&counter3, sizeof(counter3), NULL)){
			printf("Logger thread could not receive data.\n");
		}	
		printf("Received message is %d\n", counter3);
		mq_close(mqd_log);
		usleep(1000);
	}
	//pthread_exit(NULL);
}


int main(){

	pthread_attr_t attr;
	uint32_t length_msg_struct = 0;
	message msg;
	length_msg_struct = sizeof(msg);
	sem_init (& temp_sem , 0 , 1 );
	sem_init (& light_sem , 0 , 0 );
	attrp = NULL;

	//initializing message queue attributes
	mq_attr_log.mq_maxmsg = 50;						// msg_max of Linux system
	mq_attr_log.mq_msgsize = sizeof(uint32_t);
	mq_attr_log.mq_flags = O_NONBLOCK;

	attrp = &mq_attr_log;
	mq_unlink(MSG_QUEUE_LOG);

	mqd_log = mq_open(MSG_QUEUE_LOG, O_RDWR|O_CREAT, 0666, attrp);

	if(mqd_log < 0)
  	{
    	perror("sender mq_open");
    	exit(1);
  	}
  	else
  	{
  		printf("message queue value is %d\n", mqd_log);
    	printf("Log message queue created.\n");
  	}

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
	
/*
	//spawn logger thread
	if(pthread_create(&logger_thread, &attr, (void*)&logger_thread_fn, NULL)){
        printf("Failed to create light thread.\n");
	}
 
 	printf("Logger thread spawned.\n");
*/
 	printf("still in main\n");

	//join temperature, logger and light threads
 	pthread_join(temperature_thread, NULL);
 	pthread_join(light_thread, NULL);
 	//pthread_join(logger_thread, NULL);

	return 0;
}
