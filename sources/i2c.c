#include <stdio.h> 
#include <stdint.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <linux/i2c.h> 
#include <linux/i2c-dev.h> 
#include <sys/ioctl.h> 
#include "i2c.h"
#define PATH_LEN        (64)

uint8_t* i2c_rw(i2c_cmd cmd)
{       
        uint8_t *received_data = NULL;
        uint8_t *buf = NULL;
        uint8_t namebuf[PATH_LEN];
        uint64_t fd;
        
        snprintf(namebuf, sizeof(namebuf), "/dev/i2c-%d", cmd.i2c_bus);
        
        if ((fd = open(namebuf, O_RDWR)) < 0)
        {
                perror("I2C - 2 does not exist\n");
                exit(1);
        }

        if(ioctl(fd, I2C_SLAVE, cmd.i2c_addr) < 0)
        {
                perror("IOCTL failed\n");
                exit(1);
        }

        buf = (uint8_t *)malloc(cmd.send_count);
        received_data = (uint8_t *)malloc(cmd.recv_count);

        if(write(fd, cmd.send_data , cmd.send_count) != cmd.send_count)
        {
                perror("Write error\n");
                exit(1);
        }

        if((read(fd, received_data, cmd.recv_count)) < 0)
        {
                perror("Error in read\n");
                exit(1);
        }
        close(fd);
        return received_data;
}