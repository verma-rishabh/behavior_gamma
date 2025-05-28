#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
// #include "pin_config.h"
// #include "ESP_Log.h" // Commented out as the file is not found
#include "esp_log.h" // Use the standard ESP-IDF logging library instead
#include "math.h"
#include "init.h"
#include "dac.h"    
#include "gpio.h"



// Task to establish UART connection
void uart_task(void *param) {
    //using uart without mutex since its only task reading from uart buffer
    while (1) {
        uint8_t data_ =0 ;
        data_ = getchar_unlocked();
        if (data_== 0x33){
            uint8_t pin_type = getchar_unlocked();
            uint8_t mode_ = getchar_unlocked();
            if (pin_type == 0x00){
                //digital pin
                if (mode_ == 0x00){
                    //read
                    int8_t pin = (int8_t)getchar_unlocked();
                    ERROR_CHECK(xTaskCreate(gpio_digital_read_task, "gpio_digital_read_task", 2048, &pin, 5, NULL) != pdPASS); //PdPASS is 1
                }
                else if (mode_ == 0x01){
                    //write
                    int gpio_params[2];
                    gpio_params[0] = (int)getchar_unlocked(); // GPIO pin number
                    gpio_params[1] = (int)getchar_unlocked(); // upper byte
                    gpio_params[1] = (gpio_params[1] << 8) | getchar_unlocked();  // lower byte
                    ERROR_CHECK(xTaskCreate(gpio_digital_write_task, "gpio_digital_write_task", 2048, gpio_params, 5, NULL) != pdPASS); //PdPASS is 1
                }
            }
            else if (pin_type == 0x01){
                //analog pin
                if (mode_ == 0x00){
                    //read
                    int8_t pin = (int8_t)getchar_unlocked();
                    ERROR_CHECK(xTaskCreate(gpio_analog_read_task, "gpio_analog_read_task", 2048, &pin, 5, NULL) != pdPASS); //PdPASS is 1
                }
                else if (mode_ == 0x01){
                    //write
                    uint16_t gpio_params[2];
                    gpio_params[0] = (int)getchar_unlocked(); // GPIO pin number
                    gpio_params[1] = (int)getchar_unlocked(); // Upper byte
                    gpio_params[1] = (gpio_params[1] << 8) | getchar_unlocked();  // Lower byte
                    ERROR_CHECK(xTaskCreate(gpio_analog_write_task, "gpio_digital_write_task", 2048, gpio_params, 5, NULL) != pdPASS); //PdPASS is 1
                }

            }
        
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
   
}

void app_main() {
    init_task();
   
    xTaskCreate(uart_task, "uart_task", 2048*10, NULL, 5, NULL);
}