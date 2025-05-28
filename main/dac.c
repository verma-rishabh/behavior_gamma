#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "dac.h"   
#include "init.h" 
//// Not used 
uint16_t voltage_to_dac_value(float voltage, float vref) {
    voltage = voltage < 0 ? 0 : (voltage > vref ? vref : voltage);
    // const uint32_t max_dac = 0xFFFF;  // 65535 for 16-bit
    float scaled = (voltage / 5.0) * 65535;
    uint16_t raw = (uint16_t)roundf(scaled);
    printf("voltage: %f, raw: %x\n", voltage, raw);
    return raw;
    // return (uint16_t)((voltage / vref) * ((1U << DAC_RESOLUTION) - 1) + 0.5f);
}

void set_dac_channel(spi_device_handle_t spi, uint8_t channel, uint16_t dac_value) {

    esp_err_t ret;
    dac_value = dac_value > ((1 << DAC_RESOLUTION) - 1) ? ((1 << DAC_RESOLUTION) - 1) : dac_value;
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,  // Set the Tx flag to use internal tx_data
        .length = 32,                  // Transaction length in bits
    };

    t.tx_data[0] = CMD_WRITE_UPDATE;  // Command: Write and update
    t.tx_data[1] = (channel << 4) | ((dac_value >> 12) & 0x0F); // Upper nibble is channel, lower nibble is DAC value
    t.tx_data[2] = (dac_value >> 4) & 0xFF; // DAC value (middle byte)
    t.tx_data[3] = (dac_value << 4) & 0xF0; // DAC value (lower nibble)

    ret = (int16_t)spi_device_transmit(spi, &t);
    ERROR_CHECK(ret);
    if (xSemaphoreTake(uart_mutex, portMAX_DELAY) == pdTRUE) {
        putchar(0x33);                                                                  //Start of message                            
        putchar(0x00);                                                                  //Pin type
        putchar(0x01);                                                                  //Mode
        putchar(channel);                                                            //Pin number                                          
        putchar(ret >> 8);                                                          //ret high byte                                 
        putchar(ret & 0x0F);                                                        //ret low byte                                          
        putchar(0x0A);                                                                  //End of message                                    
        xSemaphoreGive(uart_mutex);
    }

}

void gpio_analog_write_task(void *param) {
    uint16_t *gpio_params = (uint16_t *)param;
    uint8_t channel = gpio_params[0];
    uint16_t voltage = gpio_params[1];
    channel = channel - 1; // Convert to zero-based index
    set_dac_channel(spi, channel, voltage);

    // Delete the task after execution
    vTaskDelete(NULL);
}


// void set_voltage( uint8_t channel, float voltage) {
//     uint16_t dac_value = voltage_to_dac_value(voltage, VREF);

//     set_dac_channel(spi, channel, dac_value);
// }

