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
#include "driver/usb_serial_jtag.h"


// Task to establish UART connection
void uart_task(void *param) {
    //using uart without mutex since its only task reading from uart buffer
    while (1) {
        uint8_t data_[1] = {0} ;
        int len = usb_serial_jtag_read_bytes(data_, 1, 5 / portTICK_PERIOD_MS);
        uint8_t id_ = data_[0];
        if (id_== 0x33){
            len = usb_serial_jtag_read_bytes(data_, 1, 5 / portTICK_PERIOD_MS);
            uint8_t pin_type = data_[0];
            len = usb_serial_jtag_read_bytes(data_, 1, 5 / portTICK_PERIOD_MS);
            uint8_t mode_ = data_[0];
            if (pin_type == 0x00){
                //digital pin
                if (mode_ == 0x00){
                    //read
                    int8_t pin = 0;
                    len = usb_serial_jtag_read_bytes(data_, 1, 5 / portTICK_PERIOD_MS);
                    pin = (int8_t)data_[0];
                    len = usb_serial_jtag_read_bytes(data_, 2, 5 / portTICK_PERIOD_MS);                                         // Remove last two bytes
                    ERROR_CHECK(xTaskCreate(gpio_digital_read_task, "gpio_digital_read_task", 2048, &pin, 5, NULL) != pdPASS); //PdPASS is 1
                }
                else if (mode_ == 0x01){
                    //write
                    int gpio_params[2];
                    len = usb_serial_jtag_read_bytes(data_, 1, 5 / portTICK_PERIOD_MS);
                    gpio_params[0] = (int)data_[0]; // GPIO pin number
                    len = usb_serial_jtag_read_bytes(data_, 1, 5 / portTICK_PERIOD_MS);
                    gpio_params[1] = (int)data_[0]; // upper byte
                    len = usb_serial_jtag_read_bytes(data_, 1, 5 / portTICK_PERIOD_MS);
                    gpio_params[1] = (gpio_params[1] << 8) | data_[0];  // lower byte
                    ERROR_CHECK(xTaskCreate(gpio_digital_write_task, "gpio_digital_write_task", 2048, gpio_params, 5, NULL) != pdPASS); //PdPASS is 1
                }
            }
            else if (pin_type == 0x01){
                //analog pin
                if (mode_ == 0x00){
                    //read
                    int8_t pin = 0;
                    len = usb_serial_jtag_read_bytes(data_, 1, 5 / portTICK_PERIOD_MS);
                    pin = (int8_t)data_[0];
                    len = usb_serial_jtag_read_bytes(data_, 2, 5 / portTICK_PERIOD_MS);                                         // Remove last two bytes
                    ERROR_CHECK(xTaskCreate(gpio_analog_read_task, "gpio_analog_read_task", 2048, &pin, 5, NULL) != pdPASS); //PdPASS is 1
                }
                else if (mode_ == 0x01){
                    //write
                    uint16_t gpio_params[2];
                    len = usb_serial_jtag_read_bytes(data_, 1, 5 / portTICK_PERIOD_MS);
                    gpio_params[0] = (int)data_[0]; // GPIO pin number
                    len = usb_serial_jtag_read_bytes(data_, 1, 5 / portTICK_PERIOD_MS);
                    gpio_params[1] = (int)data_[0]; // Upper byte
                    len = usb_serial_jtag_read_bytes(data_, 1, 5 / portTICK_PERIOD_MS);
                    gpio_params[1] = (gpio_params[1] << 8) | data_[0];  // Lower byte
                    ERROR_CHECK(xTaskCreate(gpio_analog_write_task, "gpio_analog_write_task", 2048, gpio_params, 5, NULL) != pdPASS); //PdPASS is 1
                }

            }
        
        }
        // vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
   
}

void app_main() {
    init_task();
   
    xTaskCreate(uart_task, "uart_task", 2048*10, NULL, 5, NULL);
}