#include <stdio.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <linux/i2c.h> 
#include <linux/i2c-dev.h> 
#include <sys/ioctl.h> 
#define PATH_LEN        (64)
int main(int argc, char const *argv[]) {
        int I2CBus = 2;
        int I2CAddress = 0x48;
        int fd, temperature;
        char namebuf[PATH_LEN];
        char databuffer[2];
        snprintf(namebuf, sizeof(namebuf), "/dev/i2c-%d", I2CBus);
        if ((fd = open(namebuf, O_RDWR)) < 0){
                perror("I2C - 2 does not exist\n");
                exit(1);
        }
        if(ioctl(fd, I2C_SLAVE, I2CAddress) < 0){
                perror("IOCTL failed\n");
                exit(1);
        }
        char buf[1] = {0x00};
        if(write(fd, buf, 1) != 1)
                perror("write error\n");
        int bytesRead = read(fd, databuffer, 2);
        if (bytesRead == -1)
        {
                perror("Error in read\n");

        }

        temperature = ((databuffer[0] << 8) | databuffer[1]);
        temperature >>= 4;

        printf("Data in registers : %x\n", temperature);
        printf("Temp (deg C) = %.4f\n", temperature * 0.0625);

        //printf("Data read back: 0x%x%x\n", databuffer[0], databuffer[1]);
        close(fd);
        return 0;
}
