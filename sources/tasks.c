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
#include <string.h>

#define MSG_QUEUE_LOG "/my_queue_log"

pthread_t temperature_thread;
pthread_t light_thread;
pthread_t logger_thread;
pthread_mutex_t mutex_log;

static volatile int count = 0;

static uint32_t counter1 = 0;
static uint32_t counter2 = 1000;

struct mq_attr mq_attr_log;
static mqd_t mqd_log;
sem_t temp_sem, light_sem;

message msg1;
message msg2;
message msg3;


void *temp_thread_fn(void *threadid){
	printf("In temperature thread function.\n");
	task_id id =  temp_thread;
	message_type type = LOG_MESSAGE;

	while(1){
		//sem_wait(&temp_sem);
		/*mqd_log = mq_open(MSG_QUEUE_LOG, \
						O_WRONLY, \
						0666, \
						&mq_attr_log);*/
		msg1.source_task = id;
		msg1.type = type;
		msg1.data = &counter1;

		if(!pthread_mutex_lock(&mutex_log)){

			if(count<10 && mq_send(mqd_log, (const char *)&msg1, sizeof(msg1), 1)){
				printf("Temperature thread could not send data.\n");
			}
			else if(count<10){
				printf("Sent %d\n", counter1++);
				//printf("Count is %d\n", count);
				count++;
			}
		}
		pthread_mutex_unlock(&mutex_log);
		//mq_close(mqd_log);
		//sem_post(&light_sem);
		usleep(1500);
	}
	//pthread_exit(NULL);
}

void *light_thread_fn(void *threadid){
	printf("In light thread function.\n");
	task_id id =  lght_thread;
	message_type type = LOG_MESSAGE;
	while(1){
		//sem_wait(&light_sem);
		/*mqd_log = mq_open(MSG_QUEUE_LOG, \
						O_WRONLY, \
						0666, \
						&mq_attr_log);*/
		//printf("Thread id of light thread is %ld\n", pthread_self());
		msg2.source_task = id;
		msg2.type = type;
		msg2.data = &counter2;

		if(!pthread_mutex_lock(&mutex_log)){
			if(mq_send(mqd_log, (const char *)&msg2, sizeof(msg2), 1)){
				printf("Light thread could not send data.\n");
			}
			else if(count<10){
				printf("Sent %d\n", counter2++);
				//printf("Count is %d\n", count);
				count++;
			}	
		}
		pthread_mutex_unlock(&mutex_log);
		//mq_close(mqd_log);
		//sem_post(&temp_sem);
		usleep(1500);
	}
	//pthread_exit(NULL);
}


void *logger_thread_fn(void *threadid){
	printf("In logger thread function.\n");
	task_id id;
	while(1){
		/*mqd_log = mq_open(MSG_QUEUE_LOG, \
						O_RDONLY, \
						0666, \
						NULL);*/
		if(!mq_receive(mqd_log, (char *)&msg3, sizeof(msg3), NULL)){
			printf("Logger thread could not receive data.\n");
		}	
		else if(count>0 && count<11){
			if(msg3.source_task == (id = temp_thread)){
				printf("Received %d from temp thread.\n", *(uint32_t *)msg3.data);
			}
			
			if(msg3.source_task == (id = lght_thread)){
				printf("Received %d from light thread.\n", *(uint32_t *)msg3.data);
			}

			if(msg3.source_task == (id = main_thread)){
				printf("Received %s from main thread.\n", (char *)msg3.data);				
			}

			count--;
		}
		//mq_close(mqd_log);
		usleep(500);
	}
	//pthread_exit(NULL);
}


int main(){

	pthread_attr_t attr;
	//uint32_t length_msg_struct = 0;
	//message msg;
	//length_msg_struct = sizeof(msg);
	sem_init (& temp_sem , 0 , 1 );
	sem_init (& light_sem , 0 , 0 );

   if(pthread_mutex_init(&mutex_log, NULL)){
      printf("Error in locking mutex.\n");  
      exit(1);
   }

   printf("Log queue mutex initialized.\n");

	//initializing message queue attributes
	mq_attr_log.mq_maxmsg = 10;
	mq_attr_log.mq_msgsize = sizeof(message);
	mq_attr_log.mq_flags = 0;

	mq_unlink(MSG_QUEUE_LOG);

	mqd_log = mq_open(MSG_QUEUE_LOG, \
						O_CREAT|O_RDWR|O_NONBLOCK, \
						0666, \
						&mq_attr_log);

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
	
	//exit(0);

	task_id id =  main_thread;
	message_type type = SYSTEM_INIT_MESSAGE;
	msg3.source_task = id;
	msg3.type = type;

	//spawn temperature thread
	if(pthread_create(&temperature_thread, &attr, (void*)&temp_thread_fn, NULL)){
        printf("Failed to create temperature thread.\n");
	}
 
	msg3.data = "Temperature thread is spawned.";

 	if(!pthread_mutex_lock(&mutex_log)){
		if(mq_send(mqd_log, (const char *)&msg3, sizeof(msg3), 1)){
			printf("Main thread could not send data.\n");
		}
		else if(count<10){
			printf("Sent %s\n", (char *)msg3.data);
			//printf("Count is %d\n", count);
			count++;
		}	
	}
	pthread_mutex_unlock(&mutex_log);
 	
 	//printf("Temperature thread spawned.\n");


	//spawn light thread
	if(pthread_create(&light_thread, &attr, (void*)&light_thread_fn, NULL)){
        printf("Failed to create light thread.\n");
	}
 
	msg3.data = "Light thread spawned.";

	if(!pthread_mutex_lock(&mutex_log)){
		if(mq_send(mqd_log, (const char *)&msg3, sizeof(msg3), 1)){
			printf("Main thread could not send data.\n");
		}
		else if(count<10){
			printf("Sent %s\n", (char *)msg3.data);
			//printf("Count is %d\n", count);
			count++;
		}	
	}
	pthread_mutex_unlock(&mutex_log);

 	//printf("Light thread spawned.\n");
	
	//spawn logger thread
	if(pthread_create(&logger_thread, &attr, (void*)&logger_thread_fn, NULL)){
        printf("Failed to create light thread.\n");
	}
 
 	printf("Logger thread spawned.\n");

 	printf("still in main\n");

	//join temperature, logger and light threads
 	pthread_join(temperature_thread, NULL);
 	pthread_join(light_thread, NULL);
 	pthread_join(logger_thread, NULL);

	return 0;
}
