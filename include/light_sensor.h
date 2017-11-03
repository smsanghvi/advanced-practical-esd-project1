#ifndef _LIGHT_SENSOR_H
#define _LIGHT_SENSOR_H

float light_sensor_read(void);
void light_sensor_control_regwrite(uint8_t);
uint8_t light_sensor_control_regread(void);
void light_sensor_integtime_regwrite(uint8_t);
uint8_t light_sensor_id_regread(void);
void light_sensor_integtime_regwrite(uint8_t);
#endif
