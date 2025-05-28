#ifndef DAC_H
#define DAC_H


// Include necessary headers
#include "driver/spi_master.h"
#include "math.h"
//Variables


//Define constants
#define VREF            5       // Reference voltage
#define CMD_WRITE_UPDATE 0x03      // Command: Write and update
#define CMD_WRITE        0x00      // Command: Write
#define CMD_UPDATE       0x01      // Command: Update
#define DAC_RESOLUTION  16         // 12-bit DAC
#define BUF_SIZE 1024

//Function declarations
void gpio_analog_write_task(void *param); 











#endif