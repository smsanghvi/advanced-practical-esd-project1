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
*   Date Edited: 5 Nov 2017
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
#include <mqueue.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include "temp_sensor.h"
#include "light_sensor.h"
#include "led.h"
#include "messaging.h"

#define LOG_BUFFER_SIZE 10000
#define DAY				(120.0)
#define NIGHT			(5.0)
#define TEMP_LOWER		(5.0)
#define TEMP_UPPER		(50.0)

uint32_t read_config = 1;
int retval;

pthread_t temperature_thread;
pthread_t light_thread;
pthread_t logger_thread;

pthread_mutex_t mutex_log_temp;
pthread_mutex_t mutex_log_light;
pthread_mutex_t mutex_temp_main;
pthread_mutex_t mutex_light_main;
pthread_mutex_t mutex_rqst;


char buf[LOG_BUFFER_SIZE];
struct sigaction sig;

FILE *fp;

static volatile int count_temp = 0;
static volatile int count_temp_cpy = 0;
static volatile int count_light = 0;
static volatile int count_light_cpy = 0;
static volatile int count_rqst = 0;
static volatile int temp_hb_err = 0;
static volatile int light_hb_err = 0;

static volatile int loop_count = 0;

static volatile uint32_t temp_loop;
static volatile uint32_t light_loop;
char *filename_cmd;

struct mq_attr mq_attr_log;
static mqd_t mqd_temp, mqd_temp_cp, mqd_light, mqd_light_cp, mqd_req, mqd_temp_hb, mqd_light_hb;
sem_t temp_sem, light_sem;

const char* log_levels[] = {"LOG_CRITICAL_FAILURE", "LOG_SENSOR_EXTREME_DATA", \
 "LOG_MODULE_STARTUP", "LOG_INFO_DATA", "LOG_REQUEST", "LOG_HEARTBEAT", "LOG_LIGHT_TRANSITION"};

void signal_handler(int signum)
{
	if (signum == SIGINT)
	{
		printf("\nClosing mqueue and file...\n");
		printf("Temp queue error count: %d\n", temp_hb_err);
		printf("Light queue error count: %d\n", light_hb_err);
		LEDOff();
		mq_close(mqd_temp);
		mq_close(mqd_light);
		mq_close(mqd_temp_cp);
		mq_close(mqd_light_cp);
		mq_close(mqd_req);
		mq_close(mqd_temp_hb);
		mq_close(mqd_light_hb);
		fclose(fp);
		exit(0);
	}
}

//Inline Celcius to Kelvin/Fahrenheit conversion functions
static inline float temperature_kelvin(float temperature_celcius)
{
	return 	(temperature_celcius + 273.15);
}

static inline float temperature_fahrenheit(float temperature_celcius)
{
	return (temperature_celcius * 1.8 + 32);
}


//Start function of temperature thread
void *temp_thread_fn(void *threadid){
	printf("In temperature thread function.\n");
	task_id id =  temp_thread;
	uint32_t config_read;
	// uint32_t abs_time = 0;
	//message_type type = LOG_MESSAGE;
	// struct timeval now;


	static message msg_temp, msg_temp_cp, msg_rqst, msg_heartbeat;

	//writing the defaults to the configuration register
	temp_sensor_config_regwrite(0x60A0);
	float temperature, temperature_copy;
	
	// gettimeofday(&now, NULL);
	// abs_time = now.tv_usec + 1000000;		//1000ms heartbeat timeout

	while(1){
		temp_loop++;
		//reading the sensor for temperature data
		temperature = temp_sensor_read();
		/*
			Constructing message that contains temp data and is sent to logger
		*/
		msg_temp.source_task = id;
		msg_temp.type = LOG_MESSAGE;
		msg_temp.data = &temperature;
		msg_temp.request_type = NOT_REQUEST;
		msg_temp.log_level = LOG_INFO_DATA;
		gettimeofday(&msg_temp.t, NULL);

		temperature_copy = temperature;
		//printf("timestamp in secs is %ld\n", msg1.t.tv_sec);
		//printf("timestamp in usecs is %ld\n", msg1.t.tv_usec);

		if(!pthread_mutex_lock(&mutex_log_temp)){
			if(count_temp<10 && mq_send(mqd_temp, (const char *)&msg_temp, sizeof(msg_temp), 1)){
				printf("Temperature thread could not send data to logger.\n");
			}
			else if(count_temp<10){
				//printf("Sent temperature: %f deg C\n", temperature);
				count_temp++;
			}
		}
		pthread_mutex_unlock(&mutex_log_temp);

		/*
			Constructing message that contains copy of temp data and is sent to main
		*/
		msg_temp_cp.source_task = id;
		msg_temp_cp.type = LOG_MESSAGE;
		msg_temp_cp.data = &temperature_copy;
		msg_temp_cp.request_type = NOT_REQUEST;
		msg_temp_cp.log_level = LOG_INFO_DATA;
		gettimeofday(&msg_temp_cp.t, NULL);

		if(!pthread_mutex_lock(&mutex_temp_main)){

			if(mq_send(mqd_temp_cp, (const char *)&msg_temp_cp, sizeof(msg_temp_cp), 1)){
				printf("Temperature thread could not send data to main.\n");
			}
			else{
				//printf("Sent %f\n", temperature_copy);
			}
		}
		pthread_mutex_unlock(&mutex_temp_main);	

		
		/*
			Read/write for request messages from temperature thread
		*/	
		if(!pthread_mutex_lock(&mutex_rqst)){
			//triggering a request - read light control register
			if(temp_loop % 25 == 0){
				msg_rqst.source_task = id;
				msg_rqst.type = REQUEST_MESSAGE;
				msg_rqst.data = &read_config;
				msg_rqst.request_type = LIGHT_REQUEST;
				msg_rqst.msg_rqst_type = LIGHT_CONTROL_REG_READ;
				msg_rqst.log_level = LOG_REQUEST;
				gettimeofday(&msg_rqst.t, NULL);
				if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
					printf("Temperature thread could not send data to light thread.\n");
				}
				else{
					//printf("Sent temperature thread 1: %s\n", (char *)msg_rqst.data);
				}		
			}
			//triggering a request - send power on command
			else if(temp_loop == 10 || temp_loop == 280 || temp_loop == 315){
				msg_rqst.source_task = id;
				msg_rqst.type = REQUEST_MESSAGE;
				msg_rqst.data = &read_config;
				msg_rqst.request_type = LIGHT_REQUEST;
				msg_rqst.msg_rqst_type = LIGHT_POWER_ON;
				msg_rqst.log_level = LOG_REQUEST;
				gettimeofday(&msg_rqst.t, NULL);
				if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
					printf("Temperature thread could not send data to light thread.\n");
				}
				else{
					//printf("Sent light thread 1: %s\n", (char *)msg_rqst.data);
				}		
			}
			//triggering a request - send power off command
			else if(temp_loop == 155){
				msg_rqst.source_task = id;
				msg_rqst.type = REQUEST_MESSAGE;
				msg_rqst.data = &read_config;
				msg_rqst.request_type = LIGHT_REQUEST;
				msg_rqst.msg_rqst_type = LIGHT_POWER_OFF;
				msg_rqst.log_level = LOG_REQUEST;
				gettimeofday(&msg_rqst.t, NULL);
				if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
					printf("Temperature thread could not send data to light thread.\n");
				}
				else{
					//printf("Sent light thread 1: %s\n", (char *)msg_rqst.data);
				}	
			}	
			//triggering a request - reading light id register
			else if(temp_loop == 35 || temp_loop == 375){
				msg_rqst.source_task = id;
				msg_rqst.type = REQUEST_MESSAGE;
				msg_rqst.data = &read_config;
				msg_rqst.request_type = LIGHT_REQUEST;
				msg_rqst.msg_rqst_type = LIGHT_ID_READ;
				msg_rqst.log_level = LOG_REQUEST;
				gettimeofday(&msg_rqst.t, NULL);
				if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
					printf("Temperature thread could not send data to light thread.\n");
				}
				else{
					//printf("Sent light thread 1: %s\n", (char *)msg_rqst.data);
				}	
			}
			//triggering a request - set integration time of light sensor
			else if(temp_loop == 15 || temp_loop == 45){
				msg_rqst.source_task = id;
				msg_rqst.type = REQUEST_MESSAGE;
				msg_rqst.data = &read_config;
				msg_rqst.request_type = LIGHT_REQUEST;
				msg_rqst.msg_rqst_type = LIGHT_SET_INTEGRATION_TIME;
				msg_rqst.log_level = LOG_REQUEST;
				gettimeofday(&msg_rqst.t, NULL);
				if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
					printf("Temperature thread could not send data to light thread.\n");
				}
				else{
					//printf("Sent light thread 1: %s\n", (char *)msg_rqst.data);
				}	
			}
			//triggering a request - enable interrupt mode
			else if(temp_loop == 27 || temp_loop == 253){
				msg_rqst.source_task = id;
				msg_rqst.type = REQUEST_MESSAGE;
				msg_rqst.data = &read_config;
				msg_rqst.request_type = LIGHT_REQUEST;
				msg_rqst.msg_rqst_type = LIGHT_INTERRUPT_ENABLE;
				msg_rqst.log_level = LOG_REQUEST;
				gettimeofday(&msg_rqst.t, NULL);
				if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
					printf("Temperature thread could not send data to light thread.\n");
				}
				else{
					//printf("Sent light thread 1: %s\n", (char *)msg_rqst.data);
				}	
			}
			//triggering a request - disable interrupt mode
			else if(temp_loop == 85 || temp_loop == 356){
				msg_rqst.source_task = id;
				msg_rqst.type = REQUEST_MESSAGE;
				msg_rqst.data = &read_config;
				msg_rqst.request_type = LIGHT_REQUEST;
				msg_rqst.msg_rqst_type = LIGHT_INTERRUPT_DISABLE;
				msg_rqst.log_level = LOG_REQUEST;
				gettimeofday(&msg_rqst.t, NULL);
				if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
					printf("Temperature thread could not send data to light thread.\n");
				}
				else{
					//printf("Sent light thread 1: %s\n", (char *)msg_rqst.data);
				}	

			}	
			else if(!mq_receive(mqd_req, (char *)&msg_rqst, sizeof(msg_rqst), NULL)){
				//no messages in queue
			}
			else{
				//ideal case of sending response
				if(msg_rqst.request_type == TEMP_REQUEST && msg_rqst.msg_rqst_type == TEMP_CONFIG_READ){
					config_read = temp_sensor_config_regread();
					msg_rqst.source_task = id;
					msg_rqst.type = LOG_MESSAGE;
					//sprintf((char *)msg_rqst.data, "Configuration register read value: %x", config_read);
					msg_rqst.data = &config_read;
					msg_rqst.request_type = NOT_REQUEST;
					msg_rqst.log_level = LOG_INFO_DATA;
					gettimeofday(&msg_rqst.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
						printf("Temp thread could not send data to light thread.\n");
					}
					else{
						//printf("Sent temp thread ideal: %x and loop count is: %d\n", *(uint32_t *)msg_rqst.data, temp_loop);
					}	
					
					msg_rqst.source_task = id;
					msg_rqst.type = RESPONSE_MESSAGE;
					sprintf((char *)msg_rqst.data, "Configuration register read value: %x", config_read);
					gettimeofday(&msg_rqst.t, NULL);

					if(!pthread_mutex_lock(&mutex_log_temp)){
						if(count_temp<10 && mq_send(mqd_temp, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
							printf("Temperature thread could not send data to logger.\n");
						}
						else if(count_temp<10){
							//printf("Sent config read: %x\n", config_read);
							count_temp++;
						}
					}
					pthread_mutex_unlock(&mutex_log_temp);

				}
				else if(msg_rqst.request_type == TEMP_REQUEST && msg_rqst.msg_rqst_type == TEMP_SHUTDOWN_ENABLE){
					//config_read = temp_sensor_config_regread();
					temp_sensor_sd(1);
					msg_rqst.source_task = id;
					msg_rqst.type = LOG_MESSAGE;
					//sprintf((char *)msg_rqst.data, "Configuration register read value: %x", config_read);
					msg_rqst.data = "In shutdown mode";
					msg_rqst.request_type = NOT_REQUEST;
					msg_rqst.log_level = LOG_INFO_DATA;
					gettimeofday(&msg_rqst.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
						printf("Temp thread could not send data to light thread.\n");
					}
					else{
						//printf("Sent temp thread ideal: %s and loop count is: %d\n", (char *)msg_rqst.data, temp_loop);
					}	
					
					msg_rqst.source_task = id;
					msg_rqst.type = RESPONSE_MESSAGE;
					msg_rqst.data = "Temperature sensor shutdown";
					gettimeofday(&msg_rqst.t, NULL);

					if(!pthread_mutex_lock(&mutex_log_temp)){
						if(count_temp<10 && mq_send(mqd_temp, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
							printf("Temperature thread could not send data to logger.\n");
						}
						else if(count_temp<10){
						//	printf("Temperature sensor in shutdown mode.\n");
							count_temp++;
						}
					}
					pthread_mutex_unlock(&mutex_log_temp);
				}
				else if(msg_rqst.request_type == TEMP_REQUEST && msg_rqst.msg_rqst_type == TEMP_SHUTDOWN_DISABLE){
					//config_read = temp_sensor_config_regread();
					temp_sensor_sd(0);
					msg_rqst.source_task = id;
					msg_rqst.type = LOG_MESSAGE;
					//sprintf((char *)msg_rqst.data, "Configuration register read value: %x", config_read);
					msg_rqst.data = "Out of shutdown";
					msg_rqst.request_type = NOT_REQUEST;
					msg_rqst.log_level = LOG_INFO_DATA;
					gettimeofday(&msg_rqst.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
						printf("Temp thread could not send data to light thread.\n");
					}
					else{
						//printf("Sent temp thread ideal: %s and loop count is: %d\n", (char *)msg_rqst.data, temp_loop);
					}	
					
					msg_rqst.source_task = id;
					msg_rqst.type = RESPONSE_MESSAGE;
					msg_rqst.data = "Temperature sensor out of shutdown";
					gettimeofday(&msg_rqst.t, NULL);

					if(!pthread_mutex_lock(&mutex_log_temp)){
						if(count_temp<10 && mq_send(mqd_temp, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
							printf("Temperature thread could not send data to logger.\n");
						}
						else if(count_temp<10){
							//printf("Temperature sensor out of shutdown mode.\n");
							count_temp++;
						}
					}
					pthread_mutex_unlock(&mutex_log_temp);

				}	
				else if(msg_rqst.request_type == TEMP_REQUEST && msg_rqst.msg_rqst_type == TEMP_CHANGE_CONVERSION_RATE){
					//changing conversion rate to 0x10 (default)
					temp_sensor_config_conversion_rate(2);
					msg_rqst.source_task = id;
					msg_rqst.type = LOG_MESSAGE;
					//sprintf((char *)msg_rqst.data, "Configuration register read value: %x", config_read);
					msg_rqst.data = "Changing conversion rate to 4Hz";
					msg_rqst.request_type = NOT_REQUEST;
					msg_rqst.log_level = LOG_INFO_DATA;
					gettimeofday(&msg_rqst.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
						printf("Temp thread could not send data to light thread.\n");
					}
					else{
						//printf("Sent temp thread ideal: %s and loop count is: %d\n", (char *)msg_rqst.data, temp_loop);
					}	
					
					msg_rqst.source_task = id;
					msg_rqst.type = RESPONSE_MESSAGE;
					msg_rqst.data = "Conversion rate of temperature sensor is 4Hz";
					gettimeofday(&msg_rqst.t, NULL);

					if(!pthread_mutex_lock(&mutex_log_temp)){
						if(count_temp<10 && mq_send(mqd_temp, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
							printf("Temperature thread could not send data to logger.\n");
						}
						else if(count_temp<10){
							//printf("Temperature sensor conversion rate is 4Hz.\n");
							count_temp++;
						}
					}
					pthread_mutex_unlock(&mutex_log_temp);

				}	
				//you are reading your own data
				else if(msg_rqst.request_type == LIGHT_REQUEST && msg_rqst.msg_rqst_type == LIGHT_CONTROL_REG_READ){
					msg_rqst.source_task = id;
					msg_rqst.type = REQUEST_MESSAGE;
					msg_rqst.data = &read_config;
					msg_rqst.msg_rqst_type = LIGHT_CONTROL_REG_READ;
					msg_rqst.request_type = LIGHT_REQUEST;
					msg_rqst.log_level = LOG_REQUEST;
					gettimeofday(&msg_rqst.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
						printf("Temp thread could not send data to light thread.\n");
					}
					else{
						//printf("Sent light thread self: %s\n", (char *)msg_rqst_light.data);
					}	
				}
				else if(msg_rqst.request_type == LIGHT_REQUEST && msg_rqst.msg_rqst_type == LIGHT_POWER_ON){
					msg_rqst.source_task = id;
					msg_rqst.type = REQUEST_MESSAGE;
					msg_rqst.data = &read_config;
					msg_rqst.msg_rqst_type = LIGHT_POWER_ON;
					msg_rqst.request_type = LIGHT_REQUEST;
					msg_rqst.log_level = LOG_REQUEST;
					gettimeofday(&msg_rqst.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
						printf("Temp thread could not send data to light thread.\n");
					}
					else{
						//printf("Sent light thread self: %s\n", (char *)msg_rqst_light.data);
					}	
				}
				else if(msg_rqst.request_type == LIGHT_REQUEST && msg_rqst.msg_rqst_type == LIGHT_POWER_OFF){
					msg_rqst.source_task = id;
					msg_rqst.type = REQUEST_MESSAGE;
					msg_rqst.data = &read_config;
					msg_rqst.msg_rqst_type = LIGHT_POWER_OFF;
					msg_rqst.request_type = LIGHT_REQUEST;
					msg_rqst.log_level = LOG_REQUEST;
					gettimeofday(&msg_rqst.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
						printf("Temp thread could not send data to light thread.\n");
					}
					else{
						//printf("Sent light thread self: %s\n", (char *)msg_rqst_light.data);
					}
				}	
				else if(msg_rqst.request_type == LIGHT_REQUEST && msg_rqst.msg_rqst_type == LIGHT_SET_INTEGRATION_TIME){
					msg_rqst.source_task = id;
					msg_rqst.type = REQUEST_MESSAGE;
					msg_rqst.data = &read_config;
					msg_rqst.msg_rqst_type = LIGHT_SET_INTEGRATION_TIME;
					msg_rqst.request_type = LIGHT_REQUEST;
					msg_rqst.log_level = LOG_REQUEST;
					gettimeofday(&msg_rqst.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
						printf("Temp thread could not send data to light thread.\n");
					}
					else{
						//printf("Sent light thread self: %s\n", (char *)msg_rqst_light.data);
					}
				}
				else if(msg_rqst.request_type == LIGHT_REQUEST && msg_rqst.msg_rqst_type == LIGHT_ID_READ){
					msg_rqst.source_task = id;
					msg_rqst.type = REQUEST_MESSAGE;
					msg_rqst.data = &read_config;
					msg_rqst.msg_rqst_type = LIGHT_ID_READ;
					msg_rqst.request_type = LIGHT_REQUEST;
					msg_rqst.log_level = LOG_REQUEST;
					gettimeofday(&msg_rqst.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
						printf("Temp thread could not send data to light thread.\n");
					}
					else{
						//printf("Sent light thread self: %s\n", (char *)msg_rqst_light.data);
					}
				}
				else if(msg_rqst.request_type == LIGHT_REQUEST && msg_rqst.msg_rqst_type == LIGHT_INTERRUPT_ENABLE){
					msg_rqst.source_task = id;
					msg_rqst.type = REQUEST_MESSAGE;
					msg_rqst.data = &read_config;
					msg_rqst.msg_rqst_type = LIGHT_INTERRUPT_ENABLE;
					msg_rqst.request_type = LIGHT_REQUEST;
					msg_rqst.log_level = LOG_REQUEST;
					gettimeofday(&msg_rqst.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
						printf("Temp thread could not send data to light thread.\n");
					}
					else{
						//printf("Sent light thread self: %s\n", (char *)msg_rqst_light.data);
					}
				}
				else if(msg_rqst.request_type == LIGHT_REQUEST && msg_rqst.msg_rqst_type == LIGHT_INTERRUPT_DISABLE){
					msg_rqst.source_task = id;
					msg_rqst.type = REQUEST_MESSAGE;
					msg_rqst.data = &read_config;
					msg_rqst.msg_rqst_type = LIGHT_INTERRUPT_DISABLE;
					msg_rqst.request_type = LIGHT_REQUEST;
					msg_rqst.log_level = LOG_REQUEST;
					gettimeofday(&msg_rqst.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst, sizeof(msg_rqst), 1)){
						printf("Temp thread could not send data to light thread.\n");
					}
					else{
						//printf("Sent light thread self: %s\n", (char *)msg_rqst_light.data);
					}
				}
				//you have a response to be read
				else if(msg_rqst.type == LOG_MESSAGE && msg_rqst.request_type == NOT_REQUEST){
					//printf("Data in temp thread is: %s\n", (char *)msg_rqst.data);		
					msg_rqst.type = DUMMY_MESSAGE;
				}
			}
		}
		pthread_mutex_unlock(&mutex_rqst);	


		/*
			Heartbeat message to be sent to main
		*/
		msg_heartbeat.source_task = id;
		msg_heartbeat.type = LOG_MESSAGE;
		msg_heartbeat.data = &temperature_copy;
		msg_heartbeat.request_type = NOT_REQUEST;
		msg_heartbeat.log_level = LOG_HEARTBEAT;

		gettimeofday(&msg_heartbeat.t, NULL);

		if(!pthread_mutex_lock(&mutex_temp_main))
		{	
			//gettimeofday(&current, NULL);
			//if(current.tv_usec < abs_time + 100000 && current.tv_usec > abs_time - 100000)
			//{
				if(mq_send(mqd_temp_hb, (const char *)&msg_heartbeat, sizeof(msg_heartbeat), 1) < 0)
					printf("Temperature thread could not send heartbeat to main.\n");
				//abs_time = current.tv_usec + 1000000;
				//printf("temp Heartbeat sent\n");
			//}
			else{
				// printf("Error in sending HB\n");
			}


		}
		pthread_mutex_unlock(&mutex_temp_main);
		usleep(1800);
	}
}


//Start function of light thread
void *light_thread_fn(void *threadid){
	printf("In light thread function.\n");
	task_id id =  lght_thread;
	//message_type type = LOG_MESSAGE;
	
	//power on the light sensor
	light_sensor_control_regwrite(1);
	float lux_value, lux_value_copy;
	static message msg_light, msg_light_cp, msg_rqst_light, msg_heartbeat_L;

	while(1){
		light_loop++;
		
		//reading the lux value
		lux_value = light_sensor_read();
		/*
			Constructing message that contains light data and is sent to logger
		*/
		msg_light.source_task = id;
		msg_light.type = LOG_MESSAGE;
		msg_light.data = &lux_value;
		msg_light.log_level = LOG_INFO_DATA;
		gettimeofday(&msg_light.t, NULL);
		msg_light.request_type = NOT_REQUEST;

		lux_value_copy = lux_value;

		if(!pthread_mutex_lock(&mutex_log_light)){
			if(count_light<10 && mq_send(mqd_light, (const char *)&msg_light, sizeof(msg_light), 1)){
				printf("Light thread could not send data to logger.\n");
			}
			else if(count_light<10){
				//printf("Sent %f\n", lux_value);
				count_light++;
			}	
		}
		pthread_mutex_unlock(&mutex_log_light);


		/*
			Constructing message that contains copy of light data and is sent to main
		*/
		msg_light_cp.source_task = id;
		msg_light_cp.type = LOG_MESSAGE;
		msg_light_cp.data = &lux_value_copy;
		gettimeofday(&msg_light_cp.t, NULL);
		msg_light_cp.request_type = NOT_REQUEST;
		msg_light_cp.log_level = LOG_INFO_DATA;

		if(!pthread_mutex_lock(&mutex_light_main)){

			if(mq_send(mqd_light_cp, (const char *)&msg_light_cp, sizeof(msg_light_cp), 1)){
				printf("Light thread could not send data to main.\n");
			}
			else{
				//printf("Sent %d\n", counter2_copy);
			}
		}
		pthread_mutex_unlock(&mutex_light_main);

		/*
			Read/write for request messages from light thread
		*/	
		if(!pthread_mutex_lock(&mutex_rqst)){
			//triggering a request - read temperature config register
			if(light_loop % 40 == 0){
				msg_rqst_light.source_task = id;
				msg_rqst_light.type = REQUEST_MESSAGE;
				msg_rqst_light.data = &read_config;
				msg_rqst_light.request_type = TEMP_REQUEST;
				msg_rqst_light.msg_rqst_type = TEMP_CONFIG_READ;
				msg_rqst_light.log_level = LOG_REQUEST;
				gettimeofday(&msg_rqst_light.t, NULL);
				if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
					printf("Light thread could not send data to temperature thread.\n");
				}
				else{
					//printf("Sent light thread 1: %s\n", (char *)msg_rqst_light.data);
				}		
			}
			//triggering a request - send shutdown command
			else if(light_loop == 90){
				msg_rqst_light.source_task = id;
				msg_rqst_light.type = REQUEST_MESSAGE;
				msg_rqst_light.data = &read_config;
				msg_rqst_light.request_type = TEMP_REQUEST;
				msg_rqst_light.msg_rqst_type = TEMP_SHUTDOWN_ENABLE;
				msg_rqst_light.log_level = LOG_REQUEST;
				gettimeofday(&msg_rqst_light.t, NULL);
				if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
					printf("Light thread could not send data to temperature thread.\n");
				}
				else{
					//printf("Sent light thread 1: %s\n", (char *)msg_rqst_light.data);
				}		
			}
			//triggering a request - get out of shutdown mode
			else if(light_loop == 175){
				msg_rqst_light.source_task = id;
				msg_rqst_light.type = REQUEST_MESSAGE;
				msg_rqst_light.data = &read_config;
				msg_rqst_light.request_type = TEMP_REQUEST;
				msg_rqst_light.msg_rqst_type = TEMP_SHUTDOWN_DISABLE;
				msg_rqst_light.log_level = LOG_REQUEST;
				gettimeofday(&msg_rqst_light.t, NULL);
				if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
					printf("Light thread could not send data to temperature thread.\n");
				}
				else{
					//printf("Sent light thread 1: %s\n", (char *)msg_rqst_light.data);
				}		
			}	
			//triggering a request - changing conversion rate to 0x10
			else if(light_loop == 275){
				msg_rqst_light.source_task = id;
				msg_rqst_light.type = REQUEST_MESSAGE;
				msg_rqst_light.data = &read_config;
				msg_rqst_light.request_type = TEMP_REQUEST;
				msg_rqst_light.msg_rqst_type = TEMP_CHANGE_CONVERSION_RATE;
				msg_rqst_light.log_level = LOG_REQUEST;
				gettimeofday(&msg_rqst_light.t, NULL);
				if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
					printf("Light thread could not send data to temperature thread.\n");
				}
				else{
					//printf("Sent light thread 1: %s\n", (char *)msg_rqst_light.data);
				}		
			}	

			else if(!mq_receive(mqd_req, (char *)&msg_rqst_light, sizeof(msg_rqst_light), NULL)){
				//no messages in queue
			}
			else{
				//ideal case of sending request
				//sending response to reading control register
				if(msg_rqst_light.request_type == LIGHT_REQUEST && msg_rqst_light.msg_rqst_type == LIGHT_CONTROL_REG_READ){
					uint8_t control_read = light_sensor_control_regread();
					msg_rqst_light.source_task = id;
					msg_rqst_light.type = LOG_MESSAGE;
					msg_rqst_light.data = &control_read;
					msg_rqst_light.request_type = NOT_REQUEST;
					msg_rqst_light.log_level = LOG_INFO_DATA;
					gettimeofday(&msg_rqst_light.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
						printf("Light thread could not send data to temp thread.\n");
					}
					else{
						//printf("Sent light thread ideal: %s\n", (char *)msg_rqst_light.data);
					}	

					msg_rqst_light.source_task = id;
					msg_rqst_light.type = RESPONSE_MESSAGE;
					sprintf((char *)msg_rqst_light.data, "Control register read value: %x", control_read);
					gettimeofday(&msg_rqst_light.t, NULL);

					if(!pthread_mutex_lock(&mutex_log_light)){
						if(count_temp<10 && mq_send(mqd_light, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
							printf("Temperature thread could not send data to logger.\n");
						}
						else if(count_light<10){
							//printf("Sent: %d\n", counter1++);
							count_light++;
						}
					}
					pthread_mutex_unlock(&mutex_log_light);
				}

				//sending response to power off sensor
				if(msg_rqst_light.request_type == LIGHT_REQUEST && msg_rqst_light.msg_rqst_type == LIGHT_POWER_OFF){
					//power off sensor
					light_sensor_control_regwrite(0);
					msg_rqst_light.source_task = id;
					msg_rqst_light.type = LOG_MESSAGE;
					msg_rqst_light.data = "Power off";
					msg_rqst_light.request_type = NOT_REQUEST;
					msg_rqst_light.log_level = LOG_INFO_DATA;
					gettimeofday(&msg_rqst_light.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
						printf("Light thread could not send data to temp thread.\n");
					}
					else{
						//printf("Sent light thread ideal: %s\n", (char *)msg_rqst_light.data);
					}	

					msg_rqst_light.source_task = id;
					msg_rqst_light.type = RESPONSE_MESSAGE;
					msg_rqst_light.data = "Power off light sensor";
					gettimeofday(&msg_rqst_light.t, NULL);

					if(!pthread_mutex_lock(&mutex_log_light)){
						if(count_temp<10 && mq_send(mqd_light, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
							printf("Light thread could not send data to logger.\n");
						}
						else if(count_light<10){
							//printf("Sent: %d\n", counter1++);
							count_light++;
						}
					}
					pthread_mutex_unlock(&mutex_log_light);
				}

				//sending response to power on sensor
				else if(msg_rqst_light.request_type == LIGHT_REQUEST && msg_rqst_light.msg_rqst_type == LIGHT_POWER_ON){
					//power on sensor
					light_sensor_control_regwrite(1);
					msg_rqst_light.source_task = id;
					msg_rqst_light.type = LOG_MESSAGE;
					msg_rqst_light.data = "Power on";
					msg_rqst_light.request_type = NOT_REQUEST;
					msg_rqst_light.log_level = LOG_INFO_DATA;
					gettimeofday(&msg_rqst_light.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
						printf("Light thread could not send data to temp thread.\n");
					}
					else{
						//printf("Sent light thread ideal: %s\n", (char *)msg_rqst_light.data);
					}	

					msg_rqst_light.source_task = id;
					msg_rqst_light.type = RESPONSE_MESSAGE;
					msg_rqst_light.data = "Power on light sensor";
					gettimeofday(&msg_rqst_light.t, NULL);

					if(!pthread_mutex_lock(&mutex_log_light)){
						if(count_temp<10 && mq_send(mqd_light, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
							printf("Light thread could not send data to logger.\n");
						}
						else if(count_light<10){
							//printf("Sent: %d\n", counter1++);
							count_light++;
						}
					}
					pthread_mutex_unlock(&mutex_log_light);
				}

				//sending response to set integration time
				else if(msg_rqst_light.request_type == LIGHT_REQUEST && msg_rqst_light.msg_rqst_type == LIGHT_SET_INTEGRATION_TIME){
					//setting integration time
					light_sensor_integtime_regwrite(0);
					msg_rqst_light.source_task = id;
					msg_rqst_light.type = LOG_MESSAGE;
					msg_rqst_light.data = "Set integration time to 0x00";
					msg_rqst_light.request_type = NOT_REQUEST;
					msg_rqst_light.log_level = LOG_INFO_DATA;
					gettimeofday(&msg_rqst_light.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
						printf("Light thread could not send data to temp thread.\n");
					}
					else{
						//printf("Sent light thread ideal: %s\n", (char *)msg_rqst_light.data);
					}	

					msg_rqst_light.source_task = id;
					msg_rqst_light.type = RESPONSE_MESSAGE;
					msg_rqst_light.data = "Set integration time to 0x00 on light sensor";
					gettimeofday(&msg_rqst_light.t, NULL);

					if(!pthread_mutex_lock(&mutex_log_light)){
						if(count_temp<10 && mq_send(mqd_light, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
							printf("Light thread could not send data to logger.\n");
						}
						else if(count_light<10){
							//printf("Sent: %d\n", counter1++);
							count_light++;
						}
					}
					pthread_mutex_unlock(&mutex_log_light);
				}

				//sending response to read ID
				else if(msg_rqst_light.request_type == LIGHT_REQUEST && msg_rqst_light.msg_rqst_type == LIGHT_ID_READ){
					//reading ID
					uint8_t id = light_sensor_id_regread();
					msg_rqst_light.source_task = id;
					msg_rqst_light.type = LOG_MESSAGE;
					msg_rqst_light.data = &id;
					msg_rqst_light.request_type = NOT_REQUEST;
					msg_rqst_light.log_level = LOG_INFO_DATA;
					gettimeofday(&msg_rqst_light.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
						printf("Light thread could not send data to temp thread.\n");
					}
					else{
						//printf("Sent light thread ideal: %s\n", (char *)msg_rqst_light.data);
					}	

					msg_rqst_light.source_task = id;
					msg_rqst_light.type = RESPONSE_MESSAGE;
					sprintf((char *)msg_rqst_light.data, "ID read value: %d", id);
					gettimeofday(&msg_rqst_light.t, NULL);

					if(!pthread_mutex_lock(&mutex_log_light)){
						if(count_temp<10 && mq_send(mqd_light, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
							printf("Light thread could not send data to logger.\n");
						}
						else if(count_light<10){
							//printf("Sent: %d\n", counter1++);
							count_light++;
						}
					}
					pthread_mutex_unlock(&mutex_log_light);
				}

				//sending response to enable interrupts
				else if(msg_rqst_light.request_type == LIGHT_REQUEST && msg_rqst_light.msg_rqst_type == LIGHT_INTERRUPT_ENABLE){
					//setting integration time
					light_sensor_integtime_regwrite(1);
					msg_rqst_light.source_task = id;
					msg_rqst_light.type = LOG_MESSAGE;
					msg_rqst_light.data = "Set integration time to 0x00";
					msg_rqst_light.request_type = NOT_REQUEST;
					msg_rqst_light.log_level = LOG_INFO_DATA;
					gettimeofday(&msg_rqst_light.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
						printf("Light thread could not send data to temp thread.\n");
					}
					else{
						//printf("Sent light thread ideal: %s\n", (char *)msg_rqst_light.data);
					}	

					msg_rqst_light.source_task = id;
					msg_rqst_light.type = RESPONSE_MESSAGE;
					msg_rqst_light.data = "Set integration time to 0x00 on light sensor";
					gettimeofday(&msg_rqst_light.t, NULL);

					if(!pthread_mutex_lock(&mutex_log_light)){
						if(count_temp<10 && mq_send(mqd_light, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
							printf("Light thread could not send data to logger.\n");
						}
						else if(count_light<10){
							//printf("Sent: %d\n", counter1++);
							count_light++;
						}
					}
					pthread_mutex_unlock(&mutex_log_light);
				}


				//sending response to disable interrupts
				else if(msg_rqst_light.request_type == LIGHT_REQUEST && msg_rqst_light.msg_rqst_type == LIGHT_INTERRUPT_DISABLE){
					//setting integration time
					light_sensor_integtime_regwrite(0);
					msg_rqst_light.source_task = id;
					msg_rqst_light.type = LOG_MESSAGE;
					msg_rqst_light.data = "Set integration time to 0x00";
					msg_rqst_light.request_type = NOT_REQUEST;
					msg_rqst_light.log_level = LOG_INFO_DATA;
					gettimeofday(&msg_rqst_light.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
						printf("Light thread could not send data to temp thread.\n");
					}
					else{
						//printf("Sent light thread ideal: %s\n", (char *)msg_rqst_light.data);
					}	

					msg_rqst_light.source_task = id;
					msg_rqst_light.type = RESPONSE_MESSAGE;
					msg_rqst_light.data = "Set integration time to 0x00 on light sensor";
					gettimeofday(&msg_rqst_light.t, NULL);

					if(!pthread_mutex_lock(&mutex_log_light)){
						if(count_temp<10 && mq_send(mqd_light, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
							printf("Light thread could not send data to logger.\n");
						}
						else if(count_light<10){
							//printf("Sent: %d\n", counter1++);
							count_light++;
						}
					}
					pthread_mutex_unlock(&mutex_log_light);
				}

				//you are reading your own data
				else if(msg_rqst_light.request_type == TEMP_REQUEST && msg_rqst_light.msg_rqst_type == TEMP_CONFIG_READ){
					msg_rqst_light.source_task = id;
					msg_rqst_light.type = REQUEST_MESSAGE;
					msg_rqst_light.data = &read_config;
					msg_rqst_light.msg_rqst_type = TEMP_CONFIG_READ;
					msg_rqst_light.request_type = TEMP_REQUEST;
					msg_rqst_light.log_level = LOG_REQUEST;
					gettimeofday(&msg_rqst_light.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
						printf("Light thread could not send data to temperature thread.\n");
					}
					else{
						//printf("Sent light thread self: %s\n", (char *)msg_rqst_light.data);
					}	

				}
				else if(msg_rqst_light.request_type == TEMP_REQUEST && msg_rqst_light.msg_rqst_type == TEMP_SHUTDOWN_ENABLE){
					msg_rqst_light.source_task = id;
					msg_rqst_light.type = REQUEST_MESSAGE;
					msg_rqst_light.data = &read_config;
					msg_rqst_light.msg_rqst_type = TEMP_SHUTDOWN_ENABLE;
					msg_rqst_light.request_type = TEMP_REQUEST;
					msg_rqst_light.log_level = LOG_REQUEST;
					gettimeofday(&msg_rqst_light.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
						printf("Light thread could not send data to temperature thread.\n");
					}
					else{
						//printf("Sent light thread self: %s\n", (char *)msg_rqst_light.data);
					}	

				}
				else if(msg_rqst_light.request_type == TEMP_REQUEST && msg_rqst_light.msg_rqst_type == TEMP_SHUTDOWN_DISABLE){
					msg_rqst_light.source_task = id;
					msg_rqst_light.type = REQUEST_MESSAGE;
					msg_rqst_light.data = &read_config;
					msg_rqst_light.msg_rqst_type = TEMP_SHUTDOWN_DISABLE;
					msg_rqst_light.request_type = TEMP_REQUEST;
					msg_rqst_light.log_level = LOG_REQUEST;
					gettimeofday(&msg_rqst_light.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
						printf("Light thread could not send data to temperature thread.\n");
					}
					else{
						//printf("Sent light thread self: %s\n", (char *)msg_rqst_light.data);
					}	

				}
				else if(msg_rqst_light.request_type == TEMP_REQUEST && msg_rqst_light.msg_rqst_type == TEMP_CHANGE_CONVERSION_RATE){
					msg_rqst_light.source_task = id;
					msg_rqst_light.type = REQUEST_MESSAGE;
					msg_rqst_light.data = &read_config;
					msg_rqst_light.msg_rqst_type = TEMP_CHANGE_CONVERSION_RATE;
					msg_rqst_light.request_type = TEMP_REQUEST;
					msg_rqst_light.log_level = LOG_REQUEST;
					gettimeofday(&msg_rqst_light.t, NULL);
					if(mq_send(mqd_req, (const char *)&msg_rqst_light, sizeof(msg_rqst_light), 1)){
						printf("Light thread could not send data to temperature thread.\n");
					}
					else{
						//printf("Sent light thread self: %s\n", (char *)msg_rqst_light.data);
					}	

				}

				//you have a response to be read
				else if(msg_rqst_light.type == LOG_MESSAGE && msg_rqst_light.request_type == NOT_REQUEST){
					//printf("Data in light thread is: %x and loop count is: %d\n", *(uint32_t *)msg_rqst_light.data, light_loop);				
					msg_rqst_light.type = DUMMY_MESSAGE;
				}
			}
		pthread_mutex_unlock(&mutex_rqst);	
		}

		/*
			Heartbeat message to be sent to main
		*/
		msg_heartbeat_L.source_task = id;
		msg_heartbeat_L.type = LOG_MESSAGE;
		msg_heartbeat_L.data = &lux_value_copy;
		msg_heartbeat_L.request_type = NOT_REQUEST;
		msg_heartbeat_L.log_level = LOG_HEARTBEAT;
		gettimeofday(&msg_heartbeat_L.t, NULL);

		if(!pthread_mutex_lock(&mutex_temp_main))
		{	
			
			if(mq_send(mqd_light_hb, (const char *)&msg_heartbeat_L, sizeof(msg_heartbeat_L), 1) < 0)
				printf("Light thread could not send heartbeat to main.\n");
			else
			{
				// printf("Error in sending HB\n");
			}
			
		}
		pthread_mutex_unlock(&mutex_temp_main);
		usleep(750);
	}
}

//Start function of logger thread
void *logger_thread_fn(void *threadid){
	printf("In logger thread function.\n");
	//task_id id;
	static message msg_t;

	fp = fopen(filename_cmd, "a+");

	while(1){
		//checking the temperature queue
		if(!mq_receive(mqd_temp, (char *)&msg_t, sizeof(msg_t), NULL)){
			printf("Logger thread could not receive data from temp thread.\n");
		}	
		else if(count_temp>0 && count_temp<11){
			if(msg_t.type == (LOG_MESSAGE)){
				sprintf(buf, "[%ld secs, %ld usecs] [%s] Source: Temperature thread; Data: %f\n", \
				msg_t.t.tv_sec, msg_t.t.tv_usec, log_levels[msg_t.log_level], *(float *)msg_t.data);
				fwrite(buf, sizeof(char), strlen(buf), fp);
				memset(buf, 0, LOG_BUFFER_SIZE);
				count_temp--;
			}
			else if(msg_t.type == (SYSTEM_INIT_MESSAGE)){
				sprintf(buf, "[%ld secs, %ld usecs] [%s] Source: Temperature thread; Data: %s\n", \
				msg_t.t.tv_sec, msg_t.t.tv_usec, log_levels[msg_t.log_level], (char *)msg_t.data);
				fwrite(buf, sizeof(char), strlen(buf), fp);
				memset(buf, 0, LOG_BUFFER_SIZE);
				count_temp--;
			}			
			else if(msg_t.type == (RESPONSE_MESSAGE)){
				sprintf(buf, "[%ld secs, %ld usecs] [%s] Response from light to temperature thread: %s\n", \
				msg_t.t.tv_sec, msg_t.t.tv_usec, log_levels[msg_t.log_level], (char *)msg_t.data);
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
			if(msg_t.type == (LOG_MESSAGE)){
				sprintf(buf, "[%ld secs, %ld usecs] [%s] Source: Light thread; Data: %f\n", \
				msg_t.t.tv_sec, msg_t.t.tv_usec, log_levels[msg_t.log_level], *(float *)msg_t.data);
				fwrite(buf, sizeof(char), strlen(buf), fp);
				memset(buf, 0, LOG_BUFFER_SIZE);
				count_light--;
			}
			else if(msg_t.type == (SYSTEM_INIT_MESSAGE)){
				sprintf(buf, "[%ld secs, %ld usecs] [%s] Source: Light thread; Data: %s\n", \
				msg_t.t.tv_sec, msg_t.t.tv_usec, log_levels[msg_t.log_level], (char *)msg_t.data);
				fwrite(buf, sizeof(char), strlen(buf), fp);
				memset(buf, 0, LOG_BUFFER_SIZE);
				count_light--;
			}	
			else if(msg_t.type == (RESPONSE_MESSAGE)){
				sprintf(buf, "[%ld secs, %ld usecs] [%s] Response from temperature to light thread: %s\n", \
				msg_t.t.tv_sec, msg_t.t.tv_usec, log_levels[msg_t.log_level], (char *)msg_t.data);
				fwrite(buf, sizeof(char), strlen(buf), fp);
				memset(buf, 0, LOG_BUFFER_SIZE);
				count_light--;
			}					
		}
		usleep(1200);
	}
}


int main(int argc, char const *argv[]){

	pthread_attr_t attr;
	static message msg3;
	// uint32_t temp_heartbeat;
	//uint8_t light_heartbeat;
	struct timeval current_temp, current_light;
	float prev_light_data = 0.0, current_light_data, celcius, kelvin, fahrenheit;
	// clock_t temp_time;

	//checking command line options
	if(argc != 2){
		printf ("USAGE: <filename.txt>\n");
		exit(1);
	}
	else{	
		filename_cmd = argv[1];
	}

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
	mq_unlink(MSG_QUEUE_TEMP_HB);
	mq_unlink(MSG_QUEUE_LIGHT_HB);

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

	mqd_temp_hb = mq_open(MSG_QUEUE_TEMP_HB, \
						O_CREAT|O_RDWR|O_NONBLOCK, \
						0666, \
						&mq_attr_log);

	mqd_light_hb = mq_open(MSG_QUEUE_LIGHT_HB, \
						O_CREAT|O_RDWR|O_NONBLOCK, \
						0666, \
						&mq_attr_log);

  	signal(SIGINT, signal_handler);
	
	pthread_attr_init(&attr);

	// task_id id =  main_thread;
	// message_type type = SYSTEM_INIT_MESSAGE;
	msg3.source_task = main_thread;
	msg3.type = SYSTEM_INIT_MESSAGE;
	msg3.request_type = NOT_REQUEST;
	msg3.log_level = LOG_MODULE_STARTUP;
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
			//printf("Sent %s\n", (char *)msg3.data);
			count_temp++;
		}	
	}
	pthread_mutex_unlock(&mutex_log_temp);

	//spawn light thread
	if(pthread_create(&light_thread, &attr, (void*)&light_thread_fn, NULL)){
        printf("Failed to create light thread.\n");
	}
 
 	msg3.request_type = NOT_REQUEST;
	msg3.data = "Light thread spawned.";
	msg3.log_level = LOG_MODULE_STARTUP;
	gettimeofday(&msg3.t, NULL);

	if(!pthread_mutex_lock(&mutex_log_light)){
		if(mq_send(mqd_light, (const char *)&msg3, sizeof(msg3), 1)){
			printf("Main thread could not send data to light thread.\n");
		}
		else if(count_light<10){
			//printf("Sent %s\n", (char *)msg3.data);
			count_light++;
		}	
	}
	pthread_mutex_unlock(&mutex_log_light);

	//spawn logger thread
	if(pthread_create(&logger_thread, &attr, (void*)&logger_thread_fn, NULL)){
        printf("Failed to create light thread.\n");
	}
 
 	msg3.request_type = NOT_REQUEST;
 	msg3.data = "Logger thread spawned.";
 	msg3.log_level = LOG_MODULE_STARTUP;
	gettimeofday(&msg3.t, NULL);

	if(!pthread_mutex_lock(&mutex_log_temp)){
		if(mq_send(mqd_temp, (const char *)&msg3, sizeof(msg3), 1)){
			printf("Main thread could not send data to temp thread.\n");
		}
		else if(count_temp<10){
			//printf("Sent %s\n", (char *)msg3.data);
			count_temp++;
		}	
	}
	pthread_mutex_unlock(&mutex_log_temp);

	static message msg_te, msg_hb, msg_le, msg_le_send;

	gettimeofday(&current_temp, NULL);
	gettimeofday(&current_light, NULL);
	
	
	while(1)
	{
		// printf("Entered main while loop\n");
		
		if(!pthread_mutex_lock(&mutex_temp_main))
		{	
			//Checking for temp task heartbeat message
			gettimeofday(&current_temp, NULL);
			// temp_heartbeat = current_temp.tv_sec + 1;	//1s heartbeat 
			if(mq_receive(mqd_temp_hb, (char *)&msg_hb, sizeof(msg_hb), NULL) < 0)
			{
				// printf("Main thread could not receive data from temp thread.\n");
			}
			else if(current_temp.tv_usec < msg_hb.t.tv_usec + 10000) //&& current_temp.tv_usec >= msg_hb.t.tv_usec)
			{
				if(msg_hb.log_level == LOG_HEARTBEAT)
				{
					// printf("Temp heartbeat: %d secs %d usecs\n", msg_hb.t.tv_sec, msg_hb.t.tv_usec);
					// printf("Main task heartbeat: %d secs %d usecs\n", current_temp.tv_sec, current_temp.tv_usec);
					// printf("Temp HB found\n");
					if(mq_send(mqd_temp, (const char *)&msg_hb, sizeof(msg_hb), 1) < 0)
						printf("Main thread could not send temp heartbeat to logger.\n");
				}
			}

			else
			{
				printf("Temp heartbeat not found\n");
				temp_hb_err++;
				if(temp_hb_err > 5)
					signal_handler(SIGINT);
			}

			//checking the temp queue
			if(mq_receive(mqd_temp_cp, (char *)&msg_te, sizeof(msg_te), NULL) < 0)
			{
				// printf("Main thread could not receive data from temp thread.\n");
			}	
			else
			{
				if(*(float *)msg_te.data > TEMP_UPPER - 10 && *(float *)msg_te.data < TEMP_UPPER + 10)
				{
					printf("TOO HOT!!!\n");
					LEDBlink();	//LED1 Blinking
					msg_te.log_level = LOG_SENSOR_EXTREME_DATA;
					if(mq_send(mqd_temp, (const char *)&msg_te, sizeof(msg_te), 1) < 0)
						printf("Main thread could not send temp extreme data to logger.\n");
				}
				if(*(float *)msg_te.data > TEMP_LOWER - 10 && *(float *)msg_te.data < TEMP_LOWER + 10)
				{
					printf("TOO COLD!!!\n");
					LEDBlink();	//LED1 Blinking					
					msg_te.log_level = LOG_SENSOR_EXTREME_DATA;
					if(mq_send(mqd_temp, (const char *)&msg_te, sizeof(msg_te), 1) < 0)
						printf("Main thread could not send temp extreme data to logger.\n");
					// signal_handler(SIGINT);
				}
				
				//converting temperature data
				celcius = *(float *)msg_te.data;
				// kelvin = temperature_kelvin(celcius);
				// fahrenheit = temperature_fahrenheit(celcius);
			}
			pthread_mutex_unlock(&mutex_temp_main);
		}
		
		if(!pthread_mutex_lock(&mutex_light_main))
		{
			//Checking for light task heartbeat message
			gettimeofday(&current_light, NULL);
			if(mq_receive(mqd_light_hb, (char *)&msg_hb, sizeof(msg_hb), NULL) < 0)
			{
				// printf("Main thread could not receive data from temp thread.\n");
			}
			else if(current_light.tv_usec < msg_hb.t.tv_usec + 6000) //&& current_temp.tv_usec >= msg_hb.t.tv_usec)
			{
				if(msg_hb.log_level == LOG_HEARTBEAT)
				{
					// printf("Light heartbeat: %d secs %d usecs\n", msg_hb.t.tv_sec, msg_hb.t.tv_usec);
					// printf("Main task heartbeat: %d secs %d usecs\n", current_temp.tv_sec, current_temp.tv_usec);
					if(mq_send(mqd_light, (const char *)&msg_hb, sizeof(msg_hb), 1) < 0)
						printf("Main thread could not send light heartbeat to logger.\n");
				}
				// else
				// {
				// 	printf("Light heartbeat not found\n");
				// 	light_hb_err++;
				// 	if(light_hb_err > 5)
				// 		signal_handler(SIGINT);
				// }
			}
			else
			{
				printf("Light heartbeat not found\n");
				light_hb_err++;
				if(light_hb_err > 5)
					signal_handler(SIGINT);
			}

			//checking the light queue
			if(mq_receive(mqd_light_cp, (char *)&msg_le, sizeof(msg_le), NULL) < 0)
			{
				// printf("Main thread could not receive data from light thread.\n");
			}	
			else
			{
				current_light_data = *(float *)msg_le.data;
				if(current_light_data == 4841462298831705888555710545920.000000 || current_light_data == 264532997862441615360.000000)
				{
					//Discard these garbage values and assign current value to zero
					current_light_data = 0;
				}
				printf("Main thread: Light data is %f\n", current_light_data);

				if(current_light_data  > prev_light_data + 15.0)
				{
					// printf("Change from dark to light\n");
					msg_le_send.log_level = LOG_LIGHT_TRANSITION;
					msg_le_send.request_type = NOT_REQUEST;
					msg_le_send.source_task = lght_thread;
					msg_le_send.type = LOG_MESSAGE;
					msg_le_send.data = &current_light_data;
					gettimeofday(&msg_le_send.t, NULL);					
					if(mq_send(mqd_light, (const char *)&msg_le_send, sizeof(msg_le_send), 1) < 0)
						printf("Main thread could not send light transition data to logger.\n");
				}

				if(current_light_data + 15.0 < prev_light_data)
				{
					// printf("Change from light to dark\n");
					msg_le_send.log_level = LOG_LIGHT_TRANSITION;
					msg_le_send.request_type = NOT_REQUEST;
					msg_le_send.source_task = lght_thread;
					msg_le_send.type = LOG_MESSAGE;
					msg_le_send.data = &current_light_data;
					gettimeofday(&msg_le_send.t, NULL);					
					if(mq_send(mqd_light, (const char *)&msg_le_send, sizeof(msg_le_send), 1) < 0)
						printf("Main thread could not send light transition data to logger.\n");
				}

				if(current_light_data > DAY - 30.0 && current_light_data < DAY + 30.0)
				{
					// printf("TOO BRIGHT!!!\n");
					LEDOn();	//LED1 ON
					msg_le_send.log_level = LOG_SENSOR_EXTREME_DATA;
					msg_le_send.request_type = NOT_REQUEST;
					msg_le_send.source_task = lght_thread;
					msg_le_send.type = LOG_MESSAGE;
					msg_le_send.data = &current_light_data;
					gettimeofday(&msg_le_send.t, NULL);					
					if(mq_send(mqd_light, (const char *)&msg_le_send, sizeof(msg_le_send), 1) < 0)
						printf("Main thread could not send light extreme data to logger.\n");
					// signal_handler(SIGINT);
				}
				else
					LEDOff(); //LED1 OFF

				if(current_light_data < NIGHT + 0.2 && current_light_data > NIGHT)
				{
					// printf("TOO DARK!!!\n");
					LEDOn();	//LED1 Blinking
					msg_le_send.log_level = LOG_SENSOR_EXTREME_DATA;
					msg_le_send.request_type = NOT_REQUEST;
					msg_le_send.source_task = lght_thread;
					msg_le_send.type = LOG_MESSAGE;
					msg_le_send.data = &current_light_data;
					gettimeofday(&msg_le_send.t, NULL);					
					if(mq_send(mqd_light, (const char *)&msg_le_send, sizeof(msg_le_send), 1) < 0)
						printf("Main thread could not send light extreme data to logger.\n");
					// signal_handler(SIGINT);
				}
				else
					LEDOff(); //LED1 OFF
				// prev_light_data = current_light_data;
			}

			prev_light_data = current_light_data;
			pthread_mutex_unlock(&mutex_light_main);
		}
		usleep(1000);
	}
	
	//join temperature, logger and light threads
 	pthread_join(temperature_thread, NULL);
 	pthread_join(light_thread, NULL);
 	pthread_join(logger_thread, NULL);

	return 0;
}
