/******************************************************
*   File: i2c.h
*
* ​ ​ The MIT License (MIT)
*	Copyright (C) 2017 by Snehal Sanghvi and Rishi Soni
*   Redistribution,​ ​ modification​ ​ or​ ​ use​ ​ of​ ​ this​ ​ software​ ​ in​ ​ source​ ​ or​ ​ binary
* ​ ​ forms​ ​ is​ ​ permitted​ ​ as​ ​ long​ ​ as​ ​ the​ ​ files​ ​ maintain​ ​ this​ ​ copyright.​ ​ Users​ ​ are
* ​ ​ permitted​ ​ to​ ​ modify​ ​ this​ ​ and​ ​ use​ ​ it​ ​ to​ ​ learn​ ​ about​ ​ the​ ​ field​ ​ of​ ​ embedded
* ​ ​ software.​ ​ The authors​ and​ ​ the​ ​ University​ ​ of​ ​ Colorado​ ​ are​ ​ not​ ​ liable​ ​ for
* ​ ​ any​ ​ misuse​ ​ of​ ​ this​ ​ material.
*
*
*   Authors: Snehal Sanghvi and Rishi Soni
*   Date Edited: 24 Oct 2017
*
*   Description: Header file for i2c interface
*
*
********************************************************/
#ifndef _I2C_H
#define _I2C_H

#include <stdint.h>

/*Enumerating I2C operation*/
typedef enum 
{
	READ,
	WRITE,
	REPEATED_START
}operation;

/*Struct of the I2C command*/
typedef struct 
{
	uint8_t i2c_addr;
	uint8_t i2c_bus;
	uint8_t send_count;			//Number of bytes to be sent
	uint8_t recv_count;			//Number of bytes to be received after sending
	uint8_t *send_data;			//Data to be sent
	uint8_t *received_data;		//Data to be received from I2C device
	operation op;				//Type of operation
}i2c_cmd;

uint8_t* i2c_rw(i2c_cmd);

#endif
