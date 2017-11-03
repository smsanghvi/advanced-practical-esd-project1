#include "i2c.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <math.h>
#include "light_sensor.h"

//Read light values. Convert to Lux values
float light_sensor_read(void)
{
        i2c_cmd cmd;
        uint16_t bit_shift;
    uint16_t ch0, ch1;
        float tmp, lux;
        uint8_t *returned_data = NULL;
        cmd.i2c_addr = 0x39;
        cmd.i2c_bus = 2;
        cmd.send_count = 2;
        cmd.recv_count = 4;
        //cmd.op = READ;

        returned_data = (uint8_t *)malloc(cmd.recv_count);
        cmd.send_data = (uint8_t *)malloc(cmd.send_count);

        //Power ON the light sensor first
        cmd.send_data[0] = 0x80;
    cmd.send_data[1] = 0x03;
        returned_data = (uint8_t *)i2c_rw(cmd);

    //Setting the integration time to 13.7ms (reduces ADC count value to min (=5047)
    cmd.send_data[0] = 0x81;
    cmd.send_data[1] = 0x00;
    returned_data = (uint8_t*)i2c_rw(cmd);

//  usleep(402000);

        //Reading from CH0 register
        cmd.send_data[0] = 0x8C;
    cmd.send_data[1] = 0x8D;
    returned_data = (uint8_t*)i2c_rw(cmd);
    ch0 = returned_data[1] << 8 | returned_data[0];
//  printf("Value of ch0 is %x\n", ch0);

    //Reading from CH1 register
        cmd.send_data[0] = 0x8E;
        cmd.send_data[1] = 0x8F;
        returned_data = (uint8_t*)i2c_rw(cmd);
        ch1 = returned_data[1] << 8 | returned_data[0];
        printf("Value of ch0 is %x\n", ch0);
    printf("Value of ch1 is %x\n", ch1);

    /*Calculating actual Lux values */

    //Preventing divide by zero case
    if(ch0 == 0)
        return 0.0;

    if((ch1/ch0) <= 0.50)
    {
        lux = (0.0304 * ch0) - (0.062 * ch0 * pow(ch1/ch0, 1.4));
        printf("Lux1 value is %f\n", lux);
    }
    else if((ch1/ch0) > 0.50 && (ch1/ch0) <= 0.61)
    {
        lux = (0.0224 * ch0) - (0.031 * ch1);
            printf("Lux2 value is %f\n", lux);
    }

    else if((ch1/ch0) > 0.61 && (ch1/ch0) <= 0.80)
    {
        lux = (0.0128 * ch0) - (0.0153 * ch1);
        printf("Lux2 value is %f\n", lux);
    }
    else if((ch1/ch0) > 0.80 && (ch1/ch0) <= 1.30)
    {
        lux = (0.00146 * ch0) - (0.00112 * ch1);
            printf("Lux3 value is %f\n", lux);
    }
    else if((ch1/ch0) > 1.30)
    {
        lux = 0;
            printf("Lux4 value is %f\n", lux);
    }

    return lux;
}

//Power ON or Power OFF the light sensor
void light_sensor_control_regwrite(uint8_t power)
{
    i2c_cmd cmd;
        float tmp, lux;
        uint8_t *returned_data = NULL;
        cmd.i2c_addr = 0x39;
        cmd.i2c_bus = 2;
        cmd.send_count = 2;
        cmd.recv_count = 1;

        returned_data = (uint8_t *)malloc(cmd.recv_count);
        cmd.send_data = (uint8_t *)malloc(cmd.send_count);
    if(power == 1)
    {
            //Power ON the light sensor
            cmd.send_data[0] = 0x80;
            cmd.send_data[1] = 0x03;
    }
    else if(power == 0)
    {
        //Power OFF the light sensor
            cmd.send_data[0] = 0x80;
            cmd.send_data[1] = 0x00;
    };

        returned_data = (uint8_t *)i2c_rw(cmd);
}


//Read control reg
uint8_t light_sensor_control_regread(void)
{
        i2c_cmd cmd;
        uint16_t bit_shift;
        uint8_t *returned_data = NULL;
        cmd.i2c_addr = 0x39;
        cmd.i2c_bus = 2;
        cmd.send_count = 1;
        cmd.recv_count = 1;

        returned_data = (uint8_t *)malloc(cmd.recv_count);
        cmd.send_data = (uint8_t *)malloc(cmd.send_count);

    cmd.send_data[0] = 0x80;
        returned_data = (uint8_t *)i2c_rw(cmd);
    return returned_data[0];
}

//Set integration time value
void light_sensor_integtime_regwrite(uint8_t value)
{
    i2c_cmd cmd;
        uint16_t bit_shift;
        uint8_t *returned_data = NULL;
        cmd.i2c_addr = 0x39;
        cmd.i2c_bus = 2;
        cmd.send_count = 1;
        cmd.recv_count = 1;

        returned_data = (uint8_t *)malloc(cmd.recv_count);
        cmd.send_data = (uint8_t *)malloc(cmd.send_count);

        cmd.send_data[0] = 0x81;        //Read the timing reg first
        returned_data = (uint8_t *)i2c_rw(cmd);
    free(cmd.send_data);            //Free it to re-assign it later

    if(value > 0x11)
        return;     //No values exist after 0x11
    else
    {
        cmd.send_count = 2;
        cmd.send_data = (uint8_t *)malloc(cmd.send_count);
        cmd.send_data[0] = 0x81;
        cmd.send_data[1] = returned_data[0] | value; //integration value sent, not affecting existing reg bit fields
        returned_data = (uint8_t *)i2c_rw(cmd);
    }

}

//Read ID reg
uint8_t light_sensor_id_regread(void)
{
    i2c_cmd cmd;
        uint8_t *returned_data = NULL;
        cmd.i2c_addr = 0x39;
        cmd.i2c_bus = 2;
        cmd.send_count = 1;
        cmd.recv_count = 1;

        returned_data = (uint8_t *)malloc(cmd.recv_count);
        cmd.send_data = (uint8_t *)malloc(cmd.send_count);

        cmd.send_data[0] = 0x8A;
        returned_data = (uint8_t *)i2c_rw(cmd);
    return returned_data[0];
}

//Enable or disable interrupts
void light_sensor_intrcontrol_regwrite(uint8_t value)
{
        i2c_cmd cmd;
        uint16_t bit_shift;
        uint8_t *returned_data = NULL;
        cmd.i2c_addr = 0x39;
        cmd.i2c_bus = 2;
        cmd.send_count = 1;
        cmd.recv_count = 1;

        returned_data = (uint8_t *)malloc(cmd.recv_count);
        cmd.send_data = (uint8_t *)malloc(cmd.send_count);

        cmd.send_data[0] = 0x86;                //Read the interrupt control reg first
        returned_data = (uint8_t *)i2c_rw(cmd);
        free(cmd.send_data);                    //Free it to re-assign it later

        if(value > 0x01)
            return;         //No values exist after 0x01
        else
        {
            cmd.send_count = 2;
            cmd.send_data = (uint8_t *)malloc(cmd.send_count);
            cmd.send_data[0] = 0x86;
            cmd.send_data[1] = returned_data[0] | value; //interrupt control  value sent, not affecting other bit fields
            returned_data = (uint8_t *)i2c_rw(cmd);
        }


}
