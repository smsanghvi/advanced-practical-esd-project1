
/**************************************************************************************************
*   File: messaging.h
*	
* 	The MIT License
*
​* ​ ​ Copyright​ ​ (C)​ ​ 2017​ ​ by​ ​ Snehal Sanghvi & Rishi Soni 
​*
​* ​ ​ Redistribution,​ ​ modification​ ​ or​ ​ use​ ​ of​ ​ this​ ​ software​ ​ in​ ​ source​ ​ or​ ​ binary
​* ​ ​ forms​ ​ is​ ​ permitted​ ​ as​ ​ long​ ​ as​ ​ the​ ​ files​ ​ maintain​ ​ this​ ​ copyright.​ ​ Users​ ​ are
​* ​ ​ permitted​ ​ to​ ​ modify​ ​ this​ ​ and​ ​ use​ ​ it​ ​ to​ ​ learn​ ​ about​ ​ the​ ​ field​ ​ of​ ​ embedded
​* ​ ​ software.​ ​ Snehal Sanghvi​ ​ and​ ​ Rishi Soni​ ​ are​ ​ not​ ​ liable​ ​ for
​* ​ ​ any​ ​ misuse​ ​ of​ ​ this​ ​ material.
*
*
*   Authors: Snehal Sanghvi and Rishi Soni
*   Date Edited: 9 Nov 2016
*
*   Description: I2C firmware wrapper for BeagleBone Green Linux distribution
*
**************************************************************************************************/


// I2C1 INIT
uint8_t i2c1_init(void);

// I2C1 SEND
uint8_t i2c1_send(uint8_t i2c_addr, uint8_t *wbuf, uint64_t byte_count);

// I2C1 RECV POLLED
uint8_t i2c1_recv(uint8_t i2c_addr, uint8_t *rbuf, uint8_t byte_count);

// I2C1 REPEATED START SEND and RECV
uint8_t i2c1_send_rs_recv(uint8_t i2c_addr, uint8_t *wbuf, uint64_t send_count, uint8_t *rbuf, uint64_t recv_count);


// I2C1 PING
uint8_t ping_i2c1(uint8_t i2c_addr);





