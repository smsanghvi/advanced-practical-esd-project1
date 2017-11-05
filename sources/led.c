#include <stdio.h>
#include "led.h"

char* PATH = "/sys/class/leds/beaglebone:green:usr1/trigger";
char* LEDPATH = "/sys/class/leds/beaglebone:green:usr1/brightness";

void remove_trigger(void) {
        FILE* fp = NULL;
        if((fp = fopen(PATH, "r+")))
        {
                fwrite("none", 1, 4, fp);
                fclose(fp);
        }
        else
                printf("Error\n");
}


void LEDOn(void)
{
        FILE* LED = NULL;
        remove_trigger();
        if((LED = fopen(LEDPATH, "r+")))
        {
                fwrite("1", 1, 1, LED);
                fclose(LED);
        }
        else
                printf("LEDOn error\n");
}

void LEDOff(void)
{
        FILE* LED = NULL;
        remove_trigger();
        if((LED = fopen(LEDPATH, "r+")))
        {
                fwrite("0", 1, 1, LED);
                fclose(LED);
        }
        else
                printf("LEDOff error\n");
}

