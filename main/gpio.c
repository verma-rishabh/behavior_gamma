#include <stdio.h>  
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "init.h"
// #include "pin_config.h"







// Task to control GPIO pins
void gpio_digital_write_task(void *param) {
    int *gpio_params = (int *)param;
    int pin_number = gpio_params[0]>8 ? 8 : gpio_params[0]; // Convert to 1-8 range
    int status = gpio_params[1]>0 ? 1 : 0; // Convert to 0 or 1

    int8_t mapped_pin = DIGITAL_OUTPUT[pin_number-1]; // Convert to GPIO number
    int16_t ret = (int16_t)gpio_set_level(mapped_pin, status);
    if (xSemaphoreTake(uart_mutex, portMAX_DELAY) == pdTRUE) {
        putchar(0x33);                                                                  //Start of message                            
        putchar(0x00);                                                                  //Pin type
        putchar(0x01);                                                                  //Mode
        putchar(pin_number);                                                            //Pin number                                          
        putchar(ret >> 8);                                                          //Voltage high byte                                 
        putchar(ret & 0x0F);                                                        //Voltage low byte                                          
        putchar(0x0A);                                                                  //End of message                                    
        xSemaphoreGive(uart_mutex);
    }
    // Delete the task after execution
    vTaskDelete(NULL);
}

// Task to read GPIO pin state
void gpio_digital_read_task(void *param) {
    int8_t pin_number = *(int8_t *)param;
    int8_t mapped_pin = DIGITAL_INPUT[pin_number-1]; // Convert to GPIO number
    uint16_t state = (uint16_t)gpio_get_level(mapped_pin);
    if (xSemaphoreTake(uart_mutex, portMAX_DELAY) == pdTRUE) {
        putchar(0x33);                                                                  //Start of message                            
        putchar(0x00);                                                                  //Pin type
        putchar(0x00);                                                                  //Mode
        putchar(pin_number);                                                            //Pin number                                          
        putchar(state >> 8);                                                          //Voltage high byte                                 
        putchar(state & 0x0F);                                                        //Voltage low byte                                          
        putchar(0x0A);                                                                  //End of message                                    
        xSemaphoreGive(uart_mutex);
    }

    vTaskDelete(NULL);
}

// Task to read analog input
void gpio_analog_read_task(void *param) {
    static int adc_raw=0;
    static int voltage=0;
    int8_t pin_number = *(int8_t *)param;
    if (pin_number == 1){
        ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &adc_raw));
        ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw, &voltage));
    }
    if (pin_number == 2){
        ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_2, &adc_raw));
        ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan1_handle, adc_raw, &voltage));
       
    }
    if (pin_number == 3){
        ERROR_CHECK(adc_oneshot_read(adc2_handle, ADC_CHANNEL_2, &adc_raw));
        ERROR_CHECK(adc_cali_raw_to_voltage(adc2_cali_chan0_handle, adc_raw, &voltage));
        
    }
    if (pin_number == 4){
        ERROR_CHECK(adc_oneshot_read(adc2_handle, ADC_CHANNEL_3, &adc_raw));
        ERROR_CHECK(adc_cali_raw_to_voltage(adc2_cali_chan1_handle, adc_raw, &voltage));
        
    }
    voltage = (int)((2.8 / 1.8) * voltage);
    if (xSemaphoreTake(uart_mutex, portMAX_DELAY) == pdTRUE) {
        putchar(0x33);                                                                  //Start of message                            
        putchar(0x01);                                                                  //Pin type
        putchar(0x00);                                                                  //Mode
        putchar(pin_number);                                                            //Pin number                                          
        putchar(voltage >> 8);                                                          //Voltage high byte                                 
        putchar(voltage & 0x0F);                                                        //Voltage low byte                                          
        putchar(0x0A);                                                                  //End of message                                    
        xSemaphoreGive(uart_mutex);
    }

   
    vTaskDelete(NULL);
}