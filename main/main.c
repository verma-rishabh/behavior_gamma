#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "pin_config.h"
// #include "//ESP_Log.h"
#include "driver/spi_master.h"
#include "math.h"
#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024
// DAC constants
#define VREF            5       // Reference voltage
#define CMD_WRITE_UPDATE 0x03      // Command: Write and update
#define CMD_WRITE        0x00      // Command: Write
#define CMD_UPDATE       0x01      // Command: Update
#define DAC_RESOLUTION  16         // 12-bit DAC
#define BUF_SIZE 1024
spi_device_handle_t spi;
esp_err_t ret;
// Task to control GPIO pin
void init_task(void) {
    gpio_set_direction(DigitalOutput1, GPIO_MODE_OUTPUT);
    gpio_set_level(DigitalOutput1, 0);
    gpio_set_direction(DigitalOutput2, GPIO_MODE_OUTPUT);
    gpio_set_level(DigitalOutput2, 0);
    gpio_set_direction(DigitalOutput3, GPIO_MODE_OUTPUT);
    gpio_set_level(DigitalOutput3, 0);
    gpio_set_direction(DigitalOutput4, GPIO_MODE_OUTPUT);
    gpio_set_level(DigitalOutput4, 0);  
    gpio_set_direction(DigitalOutput5, GPIO_MODE_OUTPUT);
    gpio_set_level(DigitalOutput5, 0);
    gpio_set_direction(DigitalOutput6, GPIO_MODE_OUTPUT);
    gpio_set_level(DigitalOutput6, 0);
    gpio_set_direction(DigitalOutput7, GPIO_MODE_OUTPUT);
    gpio_set_level(DigitalOutput7, 0);
    gpio_set_direction(DigitalInput1, GPIO_MODE_INPUT);
    gpio_set_direction(DigitalInput2, GPIO_MODE_INPUT);
    gpio_set_direction(DigitalInput3, GPIO_MODE_INPUT);
    gpio_set_direction(DigitalInput4, GPIO_MODE_INPUT);
    //init SPI
    
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = SPI_DIN,
        .sclk_io_num = SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };

    // Configure SPI device
    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 1,                 // SPI mode 0
        .duty_cycle_pos = 128,
        .cs_ena_pretrans = 1,
        .cs_ena_posttrans = 1,
        .clock_speed_hz = SPI_MASTER_FREQ_8M, // Clock speed (1 MHz)
        .spics_io_num = SPI_SYNC,
        .queue_size = 1,
    };

    // Initialize SPI bus
    ret = spi_bus_initialize(SPI2_HOST, &buscfg,0);
    ESP_ERROR_CHECK(ret);

    // Add device to bus
    // static spi_device_handle_t spi;
    ret = spi_bus_add_device(SPI2_HOST  , &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

}

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
    
    uint8_t tx_data[4] = {0}; // 32-bit transaction (4 bytes)

    // Validate inputs
    // channel = channel & 0x03;  // Only 0-3 valid
    dac_value = dac_value > ((1 << DAC_RESOLUTION) - 1) ? ((1 << DAC_RESOLUTION) - 1) : dac_value;

    /* Data format for 32-bit shift register (left aligned):
     * [31:28] - Unused (0-padded)
     * [23:20] - Unused (0-padded)
     * [19:4]  - DAC value (D11-D4)
     * [3:0]   - Channel
     */
    // uint32_t command_frame = (
    //     (0x00 << 24) |        // R/W: Write (0110)
    //     (0x03 << 20) |        // Command: Write+Update (0010)
    //     (channel << 16) |     // Channel selection
    //     (dac_value << 4)           // Data left-aligned in 19-4 bits
    // );

    // uint8_t tx_buffer[4] = {
    //     (command_frame >> 24) & 0xFF,
    //     (command_frame >> 16) & 0xFF,
    //     (command_frame >> 8) & 0xFF,
    //     command_frame & 0xFF
    // };

    // Construct 32-bit frame in LSB first format
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,  // Set the Tx flag to use internal tx_data
        .length = 32,                  // Transaction length in bits
    };

    t.tx_data[0] = CMD_WRITE_UPDATE;  // Command: Write and update
    t.tx_data[1] = (channel << 4) | ((dac_value >> 12) & 0x0F); // Upper nibble is channel, lower nibble is DAC value
    t.tx_data[2] = (dac_value >> 4) & 0xFF; // DAC value (middle byte)
    t.tx_data[3] = (dac_value << 4) & 0xF0; // DAC value (lower nibble)

    // t.tx_data[0] = 0b00000011;  // Command: Write and update
    // t.tx_data[1] = 0b11110100; // DAC value and channel
    // t.tx_data[2] = 0b11001100; // DAC value (lower byte)
    // t.tx_data[3] = 0b11000000; // DAC value (upper byte)
    // t.length = 32;        // 4 bytes * 8 bits = 32 bits
    // t.tx_buffer = tx_data;

    printf("tx%02x%02x%02x%02x\n", t.tx_data[0], t.tx_data[1], t.tx_data[2], t.tx_data[3]);
    // Perform SPI transaction
    ret = spi_device_transmit(spi, &t);
    if (ret != ESP_OK) {
        //ESP_LOGE("spi", "SPI transaction failed: %s", esp_err_to_name(ret));
    }
    // vTaskDelay(100 / portTICK_PERIOD_MS);

    // t.tx_data[0] = CMD_UPDATE;  // Command: Write and update
    // t.tx_data[1] = (channel << 4) | ((dac_value >> 12) & 0x0F); // Upper nibble is channel, lower nibble is DAC value
    // t.tx_data[2] = (dac_value >> 4) & 0xFF; // DAC value (middle byte)
    // t.tx_data[3] = (dac_value << 4) & 0xF0; // DAC value (lower nibble)
    // ret = spi_device_transmit(spi, &t);
    // if (ret != ESP_OK) {
    //     //ESP_LOGE("spi", "SPI transaction failed: %s", esp_err_to_name(ret));
    // }
    // vTaskDelay(100 / portTICK_PERIOD_MS);

}

void set_voltage( uint8_t channel, float voltage) {
    uint16_t dac_value = voltage_to_dac_value(voltage, VREF);

    set_dac_channel(spi, channel, dac_value);
}
void spi_write_data(void *param)     // Function to write data at given address
{
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for 1 second
    uint8_t channel = 0; // Channel number
    float voltage = 2.5; // Voltage value
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,  // Set the Tx flag to use internal tx_data
        .length = 32,                  // Transaction length in bits
    };
// software reset
    t.tx_data[0] = 0b00000111;  // Command: Write and update
    t.tx_data[1] = 0b00000000; // DAC value and channel
    t.tx_data[2] = 0b00000000; // DAC value (lower byte)
    t.tx_data[3] = 0b00000000; // DAC value (upper byte)
    ret = spi_device_transmit(spi, &t);
    if (ret != ESP_OK) {
        //ESP_LOGE("spi", "SPI transaction failed: %s", esp_err_to_name(ret));
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

// LDAC register
    t.tx_data[0] = 0b00000110;  // Command: Write and update
    t.tx_data[1] = 0b00000000; // DAC value and channel
    t.tx_data[2] = 0b00000000; // DAC value (lower byte)
    t.tx_data[3] = 0b00001111; // DAC value (upper byte)
    ret = spi_device_transmit(spi, &t);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        //ESP_LOGE("spi", "SPI transaction failed: %s", esp_err_to_name(ret));
    }

    // while (1) {
    //     for (float t = 0; t < 1.0 / frequency; t += step) {
    //         float voltage_ch0 = amplitude * sinf(2 * M_PI * frequency * t) + offset;
    //         float voltage_ch1 = amplitude * sinf(2 * M_PI * frequency * t + phase_diff) + offset;

    //         set_voltage(0, voltage_ch0); // Set voltage on channel 0
    //         set_voltage(1, voltage_ch1); // Set voltage on channel 1

    //         //ESP_LOGI("SPI_WRITE_DATA", "Time: %.2fs, Ch0 Voltage: %.2fV, Ch1 Voltage: %.2fV", t, voltage_ch0, voltage_ch1);
    //         vTaskDelay((int)(step * 1000) / portTICK_PERIOD_MS); // Delay for the time step
    //     }
    // }


    while (1) {
        for (channel = 0; channel < 4; channel++) { // Cycle through all channels 0-3
            for (voltage = 2.5; voltage <= 2.5; voltage += 0.5) { // Increment voltage from 0 to 5V
                set_voltage(channel, voltage); // Set voltage on the specified channel
                //ESP_LOGI("SPI_WRITE_DATA", "Set voltage: %.2fV on channel %d", voltage, channel);
                vTaskDelay(2000 / portTICK_PERIOD_MS); // Wait for 500 ms
            }
        }
    }

    vTaskDelete(NULL); // Delete the task after execution
}




// Task to control GPIO pins
void gpio_control_task(void *param) {
    int *gpio_params = (int *)param;
    int pin_number = gpio_params[0];
    int status = gpio_params[1];

    gpio_set_level(pin_number, status);

    // Delete the task after execution
    vTaskDelete(NULL);
}

// Task to read GPIO pin state
void gpio_read_task(void *param) {
    int pin_number = *(int *)param;
    int state = gpio_get_level(pin_number);
    //ESP_LOGI("GPIO_READ_TASK", "GPIO Pin %d state: %d", pin_number, state);
    vTaskDelete(NULL);
}

#define ECHO_TEST_TXD 43
#define ECHO_TEST_RXD 44
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
#define ECHO_UART_PORT_NUM      UART_NUM_0
#define ECHO_UART_BAUD_RATE     115200
#define ECHO_TASK_STACK_SIZE    2048
// Task to establish UART connection
void uart_task(void *param) {
    // uart_write_bytes(UART_NUM_0, (const char *)data, 5);
    while (1) {
    uint8_t data_ =0 ;
    data_ = getchar_unlocked();
    if (data_== 0x33){
        data_ = getchar_unlocked();
        if (data_==0x01){
            uint16_t voltage = 0;
            voltage = getchar_unlocked();
            voltage = (voltage << 8) | getchar_unlocked();
            // set_voltage(0, voltage);
            set_dac_channel(spi, 0, voltage);
            // printf("voltage: %x\n", voltage);
        }
        if(data_==0x04){
            //reward
            bool state_ = 0;
            state_ = (bool)getchar_unlocked();
            int gpio_params[2];
            gpio_params[0] = DigitalOutput1; // GPIO pin number
            gpio_params[1] = state_; // Set to HIGH
            xTaskCreate(gpio_control_task, "gpio_control_task", 2048, gpio_params, 5, NULL);

            // printf("voltage: %x\n", voltage);
        }
        if(data_==0x05){
            //sound
            bool state_ = 0;
            state_ = (bool)getchar_unlocked();
            int gpio_params[2];
            gpio_params[0] = DigitalOutput2; // GPIO pin number
            gpio_params[1] = state_; // Set to HIGH
            xTaskCreate(gpio_control_task, "gpio_control_task", 2048, gpio_params, 5, NULL);

            // printf("voltage: %x\n", voltage);
        }

    }
        // int len_in = strlen((char *)indata);
        
        // // Clear the input buffer to avoid leftover data
       
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        // if (len > 0) {
            // data[len] = '\0'; // Null-terminate the received data
            // int pin_number, state;
            // if (sscanf((char *)data, "%d %d", &pin_number, &state) == 2) {
            //     int gpio_params[2] = {pin_number, state};
            //     xTaskCreate(gpio_control_task, "gpio_control_task", 2048, gpio_params, 5, NULL);
            //     // //ESP_LOGI("UART_TASK", "Received: Pin %d, State %d", pin_number, state);
            // }
            // vTaskDelay(10 / portTICK_PERIOD_MS); 
        }
        
        
        vTaskDelete(NULL);
   
}

//


// Mod GPIO pin alternatively using gpio_control_task
void gpio_task(void *param) {
    int gpio_pins[] = {DigitalOutput1, DigitalOutput2, DigitalOutput3, DigitalOutput4, DigitalOutput5, DigitalOutput6, DigitalOutput7};
    int gpio_input_pins[] = {DigitalInput1, DigitalInput2, DigitalInput3, DigitalInput4};
    int counter = 0;
    int gpio_params[2];
    int state = true;
    while (1) {
        counter%=7; // Reset counter after 7
        gpio_params[0] = gpio_pins[counter]; // GPIO pin number
        gpio_params[1] = state; // Set to HIGH
        xTaskCreate(gpio_control_task, "gpio_control_task", 2048, gpio_params, 5, NULL);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for 1 second
        counter++;
        // state++;
        //ESP_LOGI("GPIO_TASK", "GPIO Pin %d set to %d", gpio_params[0], gpio_params[1]);
        // Read GPIO pin state
        for (int i = 0; i < 4; i++) {
            xTaskCreate(gpio_read_task, "gpio_read_task", 2048, &gpio_input_pins[i], 5, NULL);
            vTaskDelay(100 / portTICK_PERIOD_MS); // Wait for 100 ms
        }
    }
    vTaskDelete(NULL);
}


void app_main() {
    init_task();
    // xTaskCreate(gpio_task, "gpio_task", 2048*10, NULL, 5, NULL);
    xTaskCreate(uart_task, "uart_task", 2048*10, NULL, 5, NULL);
    // xTaskCreate(spi_write_data, "spi_write_task", 2048*10, NULL, 5, NULL);
}