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
#include <fcntl.h>
#include <sys/stat.h> 
#include "messaging.h"
#include <mqueue.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#define MSG_QUEUE_LOG "/my_queue_log"

pthread_t temperature_thread;
pthread_t light_thread;
pthread_t logger_thread;
pthread_mutex_t mutex_log;

struct timeval tv;

char buf[10000];
struct sigaction sig;

FILE *fp;

static volatile int count = 0;

static uint32_t counter1 = 0;
static uint32_t counter2 = 1000;

struct mq_attr mq_attr_log;
static mqd_t mqd_temp, mqd_temp_cp, mqd_light, mqd_light_cp, mqd_req;
sem_t temp_sem, light_sem;

message msg1, msg2, msg3;

void signal_handler(int signum)
{
	if (signum == SIGINT)
	{
		printf("\nClosing mqueue and file...\n");
		mq_close(mqd_temp);
		fclose(fp);
		exit(0);
	}
}


void *temp_thread_fn(void *threadid){
	printf("In temperature thread function.\n");
	task_id id =  temp_thread;
	message_type type = LOG_MESSAGE;

	while(1){
		msg1.source_task = id;
		msg1.type = type;
		msg1.data = &counter1;
		gettimeofday(&msg1.t, NULL);
		//printf("timestamp in secs is %ld\n", msg1.t.tv_sec);
		//printf("timestamp in usecs is %ld\n", msg1.t.tv_usec);

		if(!pthread_mutex_lock(&mutex_log)){

			if(count<10 && mq_send(mqd_temp, (const char *)&msg1, sizeof(msg1), 1)){
				printf("Temperature thread could not send data.\n");
			}
			else if(count<10){
				printf("Sent %d\n", counter1++);
				count++;
			}
		}
		pthread_mutex_unlock(&mutex_log);
		usleep(1500);
	}
}

void *light_thread_fn(void *threadid){
	printf("In light thread function.\n");
	task_id id =  lght_thread;
	message_type type = LOG_MESSAGE;
	while(1){
		msg2.source_task = id;
		msg2.type = type;
		msg2.data = &counter2;
		gettimeofday(&msg2.t, NULL);

		if(!pthread_mutex_lock(&mutex_log)){
			if(mq_send(mqd_light, (const char *)&msg2, sizeof(msg2), 1)){
				printf("Light thread could not send data.\n");
			}
			else if(count<10){
				printf("Sent %d\n", counter2++);
				count++;
			}	
		}
		pthread_mutex_unlock(&mutex_log);
		usleep(1500);
	}
}


void *logger_thread_fn(void *threadid){
	printf("In logger thread function.\n");
	task_id id;
	fp = fopen("log.txt", "a+");

	while(1){
		if(!mq_receive(mqd_temp, (char *)&msg3, sizeof(msg3), NULL)){
			printf("Logger thread could not receive data.\n");
		}	
		else if(count>0 && count<11){
			if(msg3.source_task == (id = temp_thread)){
				sprintf(buf, "[%ld secs, %ld usecs] Source: Temperature thread; Data: %d\n", \
					msg3.t.tv_sec, msg3.t.tv_usec, *(uint32_t *)msg3.data);
				fwrite(buf, sizeof(char), strlen(buf), fp);
				memset(buf, 0, 10000);
			}
			
			if(msg3.source_task == (id = lght_thread)){
				sprintf(buf, "[%ld secs, %ld usecs] Source: Light thread; Data: %d\n", \
					msg3.t.tv_sec, msg3.t.tv_usec, *(uint32_t *)msg3.data);
				fwrite(buf, sizeof(char), strlen(buf), fp);
				memset(buf, 0, 10000);
			}

			if(msg3.source_task == (id = main_thread)){			
				sprintf(buf, "[%ld secs, %ld usecs] Source: Main thread; Data: %s\n", \
					msg3.t.tv_sec, msg3.t.tv_usec, (char *)msg3.data);
				fwrite(buf, sizeof(char), strlen(buf), fp);
				memset(buf, 0, 10000);
			}

			count--;
		}
		usleep(500);
	}
}


int main(){

	pthread_attr_t attr;

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

	mqd_temp = mq_open(TEMP_MSG_QUEUE_RESPONSE, \
						O_CREAT|O_RDWR|O_NONBLOCK, \
						0666, \
						&mq_attr_log);

	mqd_temp_cp = mq_open(TEMP_MSG_QUEUE_RESPONSE_COPY, \
						O_CREAT|O_RDWR|O_NONBLOCK, \
						0666, \
						&mq_attr_log);

	mqd_light = mq_open(LIGHT_MSG_QUEUE_RESPONSE, \
						O_CREAT|O_RDWR|O_NONBLOCK, \
						0666, \
						&mq_attr_log);

	mqd_light_cp = mq_open(LIGHT_MSG_QUEUE_RESPONSE_COPY, \
						O_CREAT|O_RDWR|O_NONBLOCK, \
						0666, \
						&mq_attr_log);

	mqd_req = mq_open(MSG_QUEUE_REQUEST, \
						O_CREAT|O_RDWR|O_NONBLOCK, \
						0666, \
						&mq_attr_log);

	// if(mqd_log < 0)
 //  	{
 //    	perror("sender mq_open");
 //    	exit(1);
 //  	}
 //  	else
 //  	{
 //  		printf("message queue value is %d\n", mqd_log);
 //    	printf("Log message queue created.\n");
 //  	}

  	signal(SIGINT, signal_handler);
	
	pthread_attr_init(&attr);

	// task_id id =  main_thread;
	// message_type type = SYSTEM_INIT_MESSAGE;
	msg3.source_task = main_thread;
	msg3.type = SYSTEM_INIT_MESSAGE;
	gettimeofday(&msg3.t, NULL);

	//spawn temperature thread
	if(pthread_create(&temperature_thread, &attr, (void*)&temp_thread_fn, NULL)){
        printf("Failed to create temperature thread.\n");
	}
 
	msg3.data = "Temperature thread is spawned.";

 	if(!pthread_mutex_lock(&mutex_log)){
		if(mq_send(mqd_temp, (const char *)&msg3, sizeof(msg3), 1)){
			printf("Main thread could not send data.\n");
		}
		else if(count<10){
			printf("Sent %s\n", (char *)msg3.data);
			count++;
		}	
	}
	pthread_mutex_unlock(&mutex_log);

	//spawn light thread
	if(pthread_create(&light_thread, &attr, (void*)&light_thread_fn, NULL)){
        printf("Failed to create light thread.\n");
	}
 
	msg3.data = "Light thread spawned.";
	gettimeofday(&msg3.t, NULL);

	if(!pthread_mutex_lock(&mutex_log)){
		if(mq_send(mqd_light, (const char *)&msg3, sizeof(msg3), 1)){
			printf("Main thread could not send data.\n");
		}
		else if(count<10){
			printf("Sent %s\n", (char *)msg3.data);
			count++;
		}	
	}
	pthread_mutex_unlock(&mutex_log);

	//spawn logger thread
	if(pthread_create(&logger_thread, &attr, (void*)&logger_thread_fn, NULL)){
        printf("Failed to create light thread.\n");
	}
 
 	printf("Logger thread spawned.\n");


	if(!pthread_mutex_lock(&mutex_log)){
		if(mq_send(mqd_light, (const char *)&msg3, sizeof(msg3), 1)){
			printf("Main thread could not send data.\n");
		}
		else if(count<10){
			printf("Sent %s\n", (char *)msg3.data);
			count++;
		}	
	}
	pthread_mutex_unlock(&mutex_log);

	//join temperature, logger and light threads
 	pthread_join(temperature_thread, NULL);
 	pthread_join(light_thread, NULL);
 	pthread_join(logger_thread, NULL);

	return 0;
}
