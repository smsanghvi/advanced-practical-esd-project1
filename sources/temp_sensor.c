#include "i2c.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "temp_sensor.h"
float temp_sensor_read(void)
{
        i2c_cmd cmd;
        uint16_t bit_shift;
	uint8_t em_bit;
	float temp;
        uint8_t *returned_data = NULL;
        cmd.i2c_addr = 0x48;
        cmd.i2c_bus = 2;
        cmd.send_count = 1;
        cmd.recv_count = 2;
        //cmd.op = READ;

        returned_data = (uint8_t *)malloc(cmd.recv_count);
        cmd.send_data = (uint8_t *)malloc(cmd.send_count);
	//Checking the config register first
        cmd.send_data[0] = 0x01;
        returned_data = (uint8_t *)i2c_rw(cmd);
        bit_shift = ((returned_data[0] << 8) | returned_data[1]);
	//Reading from temperature register
	cmd.send_data[0] = 0x00;
	returned_data = (uint8_t*)i2c_rw(cmd);
	em_bit = (bit_shift & 0x0010) >> 4;
	//Bit shift pattern for temperature depends on config register reading
 	if(em_bit == 1)
	{
            printf("Value of em_bit is %d\n", em_bit);
            bit_shift = ((returned_data[0] << 8) | returned_data[1]) >> 3;
//            bit_shift >>= 3;
	}
	else if(em_bit == 0)
	{
	    printf("Value of em_bit is %d\n", em_bit);
            bit_shift = ((returned_data[0] << 8) | returned_data[1]) >> 4;
//	    bit_shift >>= 3;
	}

	//printf("Bit shift value is %x\n", bit_shift);
        //printf("Raw config reg value: 0x%x%x\n", returned_data[0], returned_data[1]);
	temp = bit_shift * 0.0625;		//Multiplier value for temp sensor
	printf("Float temperature is %f\n", temp);
        return temp;
}

void temp_sensor_config_regwrite(uint16_t data)
{
	i2c_cmd cmd;
	uint8_t mask_byte_MSB = 0, mask_byte_LSB = 0;
        uint32_t bit_shift;
        uint8_t *returned_data = NULL;
        cmd.i2c_addr = 0x48;
        cmd.i2c_bus = 2;
        cmd.send_count = 3;
        cmd.recv_count = 2;

        returned_data = (uint8_t *)malloc(cmd.recv_count);
        cmd.send_data = (uint8_t *)malloc(cmd.send_count);
//	printf("Data to be written in config reg is %x\n", data);
	mask_byte_MSB = (data & 0xFF00) >> 8;
//	printf("MSB is %x\n", mask_byte_MSB);
	mask_byte_LSB = (data & 0x00FF);
//	printf("LSB is %x\n", mask_byte_LSB);

	uint8_t temp_array[3] = {0x01, mask_byte_MSB, mask_byte_LSB};
        memcpy(cmd.send_data, temp_array, 3);
        returned_data = (uint8_t *)i2c_rw(cmd);
        bit_shift = ((returned_data[0] << 8) | returned_data[1]);

  //      printf("Bit shift value is %x\n", bit_shift);
        //printf("Raw config reg value: 0x%x%x\n", returned_data[0], returned_data[1]);
        return;
}

uint32_t temp_sensor_config_regread(void)
{
        i2c_cmd cmd;
        uint32_t bit_shift;
        uint8_t *returned_data = NULL;
        cmd.i2c_addr = 0x48;
        cmd.i2c_bus = 2;
        cmd.send_count = 1;
        cmd.recv_count = 2;

        returned_data = (uint8_t *)malloc(cmd.recv_count);
        cmd.send_data = (uint8_t *)malloc(cmd.send_count);
        cmd.send_data[0] = 0x01;
        returned_data = (uint8_t *)i2c_rw(cmd);
        bit_shift = ((returned_data[0] << 8) | returned_data[1]);

//        printf("Bit shift value is %x\n", bit_shift);
        //printf("Raw config reg value: 0x%x%x\n", returned_data[0], returned_data[1]);
        return bit_shift;
}

void temp_sensor_sd(uint8_t sd)
{
            i2c_cmd cmd;
            uint32_t bit_shift;
	    uint8_t mask_byte_MSB = 0, mask_byte_LSB = 0;
            uint8_t *returned_data = NULL;
            cmd.i2c_addr = 0x48;
            cmd.i2c_bus = 2;
            cmd.send_count = 3;
            cmd.recv_count = 2;

            bit_shift = temp_sensor_config_regread();

            returned_data = (uint8_t *)malloc(cmd.recv_count);
            cmd.send_data = (uint8_t *)malloc(cmd.send_count);
    //      printf("Data to be written in config reg is %x\n", data);
	    if(sd == 1)
	{
            mask_byte_MSB = ((bit_shift & 0xFF00) >> 8) | (0x01);
            //printf("MSB is %x\n", mask_byte_MSB);
            mask_byte_LSB = (bit_shift & 0x00FF);
            //printf("LSB is %x\n", mask_byte_LSB);
	}
	    else if (sd == 0)
	{
	    mask_byte_MSB = (bit_shift & 0xFF00) >> 8;
	    mask_byte_MSB &= ~(0x01);
            printf("MSB is %x\n", mask_byte_MSB);
            mask_byte_LSB = (bit_shift & 0x00FF);
            printf("LSB is %x\n", mask_byte_LSB);
	}

            uint8_t temp_array[3] = {0x01, mask_byte_MSB, mask_byte_LSB};
            memcpy(cmd.send_data, temp_array, 3);
            returned_data = (uint8_t *)i2c_rw(cmd);
            //bit_shift = ((returned_data[0] << 8) | returned_data[1]);
	    return;
}


void temp_sensor_em(uint8_t em)
{
            i2c_cmd cmd;
            uint32_t bit_shift;
            uint8_t mask_byte_MSB = 0, mask_byte_LSB = 0;
            uint8_t *returned_data = NULL;
            cmd.i2c_addr = 0x48;
            cmd.i2c_bus = 2;
            cmd.send_count = 3;
            cmd.recv_count = 2;

            bit_shift = temp_sensor_config_regread();

            returned_data = (uint8_t *)malloc(cmd.recv_count);
            cmd.send_data = (uint8_t *)malloc(cmd.send_count);
    //      printf("Data to be written in config reg is %x\n", data);
            if(em == 1)
        {
            mask_byte_MSB = (bit_shift & 0xFF00) >> 8;
            printf("MSB is %x\n", mask_byte_MSB);
            mask_byte_LSB = (bit_shift & 0x00FF) | (0x10);
            printf("LSB is %x\n", mask_byte_LSB);
        }
            else if (em == 0)
        {
            mask_byte_MSB = (bit_shift & 0xFF00) >> 8;
            printf("MSB is %x\n", mask_byte_MSB);
            mask_byte_LSB = (bit_shift & 0x00FF);
	    mask_byte_LSB &= ~(0x10);
            printf("LSB is %x\n", mask_byte_LSB);
        }

            uint8_t temp_array[3] = {0x01, mask_byte_MSB, mask_byte_LSB};
            memcpy(cmd.send_data, temp_array, 3);
            returned_data = (uint8_t *)i2c_rw(cmd);
            //bit_shift = ((returned_data[0] << 8) | returned_data[1]);
            return;
}

