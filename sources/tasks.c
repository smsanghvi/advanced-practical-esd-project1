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

#define LOG_BUFFER_SIZE 10000

pthread_t temperature_thread;
pthread_t light_thread;
pthread_t logger_thread;

pthread_mutex_t mutex_log_temp;
pthread_mutex_t mutex_log_light;
pthread_mutex_t mutex_temp_main;
pthread_mutex_t mutex_light_main;
pthread_mutex_t mutex_rqst;

//struct timeval tv;

char buf[LOG_BUFFER_SIZE];
struct sigaction sig;

FILE *fp;

static volatile int count_temp = 0;
static volatile int count_temp_cpy = 0;
static volatile int count_light = 0;
static volatile int count_light_cpy = 0;
static volatile int count_rqst = 0;

static uint32_t counter1 = 0;
static uint32_t counter1_copy = 0;
static uint32_t counter2 = 1000;
static uint32_t counter2_copy = 1000;

struct mq_attr mq_attr_log;
static mqd_t mqd_temp, mqd_temp_cp, mqd_light, mqd_light_cp, mqd_req;
sem_t temp_sem, light_sem;


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
	//message_type type = LOG_MESSAGE;

	static message msg_temp, msg_temp_cp;

	while(1){
		
		/*
			Constructing message that contains temp data and is sent to logger
		*/
		msg_temp.source_task = id;
		msg_temp.type = LOG_MESSAGE;
		msg_temp.data = &counter1;
		gettimeofday(&msg_temp.t, NULL);

		counter1_copy = counter1;
		//printf("timestamp in secs is %ld\n", msg1.t.tv_sec);
		//printf("timestamp in usecs is %ld\n", msg1.t.tv_usec);

		if(!pthread_mutex_lock(&mutex_log_temp)){
			if(count_temp<10 && mq_send(mqd_temp, (const char *)&msg_temp, sizeof(msg_temp), 1)){
				printf("Temperature thread could not send data to logger.\n");
			}
			else if(count_temp<10){
				printf("Sent %d\n", counter1++);
				count_temp++;
			}
		}
		pthread_mutex_unlock(&mutex_log_temp);

		/*
			Constructing message that contains copy of temp data and is sent to main
		*/
		msg_temp_cp.source_task = id;
		msg_temp_cp.type = LOG_MESSAGE;
		msg_temp_cp.data = &counter1_copy;
		gettimeofday(&msg_temp_cp.t, NULL);

		if(!pthread_mutex_lock(&mutex_temp_main)){

			if(mq_send(mqd_temp_cp, (const char *)&msg_temp_cp, sizeof(msg_temp_cp), 1)){
				printf("Temperature thread could not send data to main.\n");
			}
			else{
				printf("Sent %d\n", counter1_copy);
			}
		}
		pthread_mutex_unlock(&mutex_temp_main);		

		usleep(1500);
	}
}



void *light_thread_fn(void *threadid){
	printf("In light thread function.\n");
	task_id id =  lght_thread;
	//message_type type = LOG_MESSAGE;
	
	static message msg_light, msg_light_cp;

	while(1){

		/*
			Constructing message that contains light data and is sent to logger
		*/
		msg_light.source_task = id;
		msg_light.type = LOG_MESSAGE;
		msg_light.data = &counter2;
		gettimeofday(&msg_light.t, NULL);

		counter2_copy = counter2;

		if(!pthread_mutex_lock(&mutex_log_light)){
			if(count_light<10 && mq_send(mqd_light, (const char *)&msg_light, sizeof(msg_light), 1)){
				printf("Light thread could not send data to logger.\n");
			}
			else if(count_light<10){
				printf("Sent %d\n", counter2++);
				count_light++;
			}	
		}
		pthread_mutex_unlock(&mutex_log_light);


		/*
			Constructing message that contains copy of light data and is sent to main
		*/
		msg_light_cp.source_task = id;
		msg_light_cp.type = LOG_MESSAGE;
		msg_light_cp.data = &counter2_copy;
		gettimeofday(&msg_light_cp.t, NULL);

		if(!pthread_mutex_lock(&mutex_light_main)){

			if(mq_send(mqd_temp_cp, (const char *)&msg_light_cp, sizeof(msg_light_cp), 1)){
				printf("Light thread could not send data to main.\n");
			}
			else{
				printf("Sent %d\n", counter2_copy);
			}
		}
		pthread_mutex_unlock(&mutex_light_main);		

		usleep(1500);
	}
}


void *logger_thread_fn(void *threadid){
	printf("In logger thread function.\n");
	//task_id id;
	static message msg_t;

	fp = fopen("log.txt", "a+");

	while(1){
		//checking the temperature queue
		if(!mq_receive(mqd_temp, (char *)&msg_t, sizeof(msg_t), NULL)){
			printf("Logger thread could not receive data from temp thread.\n");
		}	
		else if(count_temp>0 && count_temp<11){
			if(msg_t.type == (LOG_MESSAGE)){
				sprintf(buf, "[%ld secs, %ld usecs] Source: Temperature thread; Data: %d\n", \
				msg_t.t.tv_sec, msg_t.t.tv_usec, *(uint32_t *)msg_t.data);
				fwrite(buf, sizeof(char), strlen(buf), fp);
				memset(buf, 0, LOG_BUFFER_SIZE);
				count_temp--;
			}
			else if(msg_t.type == (SYSTEM_INIT_MESSAGE)){
				sprintf(buf, "[%ld secs, %ld usecs] Source: Temperature thread; Data: %s\n", \
				msg_t.t.tv_sec, msg_t.t.tv_usec, (char *)msg_t.data);
				fwrite(buf, sizeof(char), strlen(buf), fp);
				memset(buf, 0, LOG_BUFFER_SIZE);
				count_temp--;
			}				
		}


		//checking the light queue
		if(!mq_receive(mqd_light, (char *)&msg_t, sizeof(msg_t), NULL)){
			printf("Logger thread could not receive data from light thread.\n");
		}	
		else if(count_light>0 && count_light<11){
			sprintf(buf, "[%ld secs, %ld usecs] Source: Light thread; Data: %d\n", \
			msg_t.t.tv_sec, msg_t.t.tv_usec, *(uint32_t *)msg_t.data);
			fwrite(buf, sizeof(char), strlen(buf), fp);
			memset(buf, 0, LOG_BUFFER_SIZE);	
			count_light--;		
		}
		
		usleep(500);
	}
}


int main(){

	pthread_attr_t attr;
	static message msg3;

   if(pthread_mutex_init(&mutex_log_temp, NULL)){
      printf("Error in initializing log temp mutex.\n");  
      exit(1);
   }

   if(pthread_mutex_init(&mutex_log_light, NULL)){
      printf("Error in initializing log light mutex.\n");  
      exit(1);
   }

   if(pthread_mutex_init(&mutex_light_main, NULL)){
      printf("Error in initializing light main mutex.\n");  
      exit(1);
   }

   if(pthread_mutex_init(&mutex_temp_main, NULL)){
      printf("Error in initializing temp main mutex.\n");  
      exit(1);
   }

   if(pthread_mutex_init(&mutex_rqst, NULL)){
      printf("Error in initializing request mutex.\n");  
      exit(1);
   }

	//initializing message queue attributes
	mq_attr_log.mq_maxmsg = 10;
	mq_attr_log.mq_msgsize = sizeof(message);
	mq_attr_log.mq_flags = 0;

	mq_unlink(TEMP_MSG_QUEUE_RESPONSE);
	mq_unlink(TEMP_MSG_QUEUE_RESPONSE_COPY);
	mq_unlink(LIGHT_MSG_QUEUE_RESPONSE);
	mq_unlink(LIGHT_MSG_QUEUE_RESPONSE_COPY);
	mq_unlink(MSG_QUEUE_REQUEST);

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

 	if(!pthread_mutex_lock(&mutex_log_temp)){
		if(mq_send(mqd_temp, (const char *)&msg3, sizeof(msg3), 1)){
			printf("Main thread could not send data to temp thread.\n");
		}
		else if(count_temp<10){
			printf("Sent %s\n", (char *)msg3.data);
			count_temp++;
		}	
	}
	pthread_mutex_unlock(&mutex_log_temp);

	//spawn light thread
	if(pthread_create(&light_thread, &attr, (void*)&light_thread_fn, NULL)){
        printf("Failed to create light thread.\n");
	}
 
	msg3.data = "Light thread spawned.";
	gettimeofday(&msg3.t, NULL);

	if(!pthread_mutex_lock(&mutex_log_light)){
		if(mq_send(mqd_light, (const char *)&msg3, sizeof(msg3), 1)){
			printf("Main thread could not send data to light thread.\n");
		}
		else if(count_light<10){
			printf("Sent %s\n", (char *)msg3.data);
			count_light++;
		}	
	}
	pthread_mutex_unlock(&mutex_log_light);

	//spawn logger thread
	if(pthread_create(&logger_thread, &attr, (void*)&logger_thread_fn, NULL)){
        printf("Failed to create light thread.\n");
	}
 
 	msg3.data = "Logger thread spawned.";
	gettimeofday(&msg3.t, NULL);

	if(!pthread_mutex_lock(&mutex_log_temp)){
		if(mq_send(mqd_temp, (const char *)&msg3, sizeof(msg3), 1)){
			printf("Main thread could not send data to temp thread.\n");
		}
		else if(count_temp<10){
			printf("Sent %s\n", (char *)msg3.data);
			count_temp++;
		}	
	}
	pthread_mutex_unlock(&mutex_log_temp);

	//join temperature, logger and light threads
 	pthread_join(temperature_thread, NULL);
 	pthread_join(light_thread, NULL);
 	pthread_join(logger_thread, NULL);

	return 0;
}
