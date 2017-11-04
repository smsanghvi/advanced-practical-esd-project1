#ifndef _TEMP_SENSOR_H
#define _TEMP_SENSOR_H

#include <stdbool.h>

float temp_sensor_read(void);
void temp_sensor_config_regwrite(uint16_t);
uint16_t temp_sensor_config_regread(void);
void temp_sensor_sd(uint8_t);
void temp_sensor_config_conversion_rate(uint8_t);

#endif
