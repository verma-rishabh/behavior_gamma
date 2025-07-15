#include "driver/uart.h"
// #include "pin_config.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "math.h"
#include "init.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "driver/usb_serial_jtag.h"
#include "input_buffer.h"

#define BUF_SIZE (1024)


spi_device_handle_t spi;
esp_err_t ret;

SemaphoreHandle_t uart_mutex;

adc_cali_handle_t adc1_cali_chan0_handle = NULL;
adc_cali_handle_t adc1_cali_chan1_handle = NULL;
adc_cali_handle_t adc2_cali_chan0_handle = NULL;
adc_cali_handle_t adc2_cali_chan1_handle = NULL;

int DIGITAL_OUTPUT[] = {
    ERROR_LED,
    10,  // DigitalOutput1 
    17, // DigitalOutput2 
    8,  // DigitalOutput3 
    5,  // DigitalOutput4 
    6,  // DigitalOutput5 
    9,  // DigitalOutput6 
    7, // DigitalOutput7 
};
int DIGITAL_INPUT[] = {
    21, // DigitalInput1
    1,  // DigitalInput2
    18, // DigitalInput3
    2,  // DigitalInput4
};
/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;
    if (!calibrated) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
    *out_handle = handle;

    return calibrated;
}

adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_handle_t adc2_handle;
void init_adc(void){
    //-------------ADC1 Init---------------//

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12, // Set the attenuation to 12dB
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ERROR_CHECK(adc_oneshot_config_channel(adc1_handle,  ADC_CHANNEL_2, &config));
    ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));
    //-------------ADC1 Calibration Init---------------//

    bool do_calibration1_chan0 = adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_3, ADC_ATTEN_DB_12, &adc1_cali_chan0_handle);
    bool do_calibration1_chan1 = adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_2, ADC_ATTEN_DB_12, &adc1_cali_chan1_handle);

    //-------------ADC2 Init---------------//

    adc_oneshot_unit_init_cfg_t init_config2 = {
        .unit_id = ADC_UNIT_2,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ERROR_CHECK(adc_oneshot_new_unit(&init_config2, &adc2_handle));

   

    //-------------ADC2 Config---------------//
    ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_2, &config));
    ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_3, &config));

     //-------------ADC2 Calibration Init---------------//

     bool do_calibration2_chan0 = adc_calibration_init(ADC_UNIT_2, ADC_CHANNEL_2, ADC_ATTEN_DB_12, &adc2_cali_chan0_handle);
     bool do_calibration2_chan1 = adc_calibration_init(ADC_UNIT_2, ADC_CHANNEL_3, ADC_ATTEN_DB_12, &adc2_cali_chan1_handle);
}


void init_serial(){
    // Configure USB SERIAL JTAG
    usb_serial_jtag_driver_config_t usb_serial_jtag_config = {
        .rx_buffer_size = BUF_SIZE,
        .tx_buffer_size = BUF_SIZE,
    };

    // Disable buffering for stdout to ensure immediate output
    setvbuf(stdout, NULL, _IONBF, 0);

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_serial_jtag_config));


}


void init_task(void) {
    gpio_set_direction(DIGITAL_OUTPUT[0], GPIO_MODE_OUTPUT);
    gpio_set_level(DIGITAL_OUTPUT[0], 0);
    gpio_set_direction(DIGITAL_OUTPUT[1], GPIO_MODE_OUTPUT);
    gpio_set_level(DIGITAL_OUTPUT[1], 0);
    gpio_set_direction(DIGITAL_OUTPUT[2], GPIO_MODE_OUTPUT);
    gpio_set_level(DIGITAL_OUTPUT[2], 0);
    gpio_set_direction(DIGITAL_OUTPUT[3], GPIO_MODE_OUTPUT);
    gpio_set_level(DIGITAL_OUTPUT[3], 0);  
    gpio_set_direction(DIGITAL_OUTPUT[4], GPIO_MODE_OUTPUT);
    gpio_set_level(DIGITAL_OUTPUT[4], 0);
    gpio_set_direction(DIGITAL_OUTPUT[5], GPIO_MODE_OUTPUT);
    gpio_set_level(DIGITAL_OUTPUT[5], 0);
    gpio_set_direction(DIGITAL_OUTPUT[6], GPIO_MODE_OUTPUT);
    gpio_set_level(DIGITAL_OUTPUT[6], 0);
    gpio_set_direction(DIGITAL_INPUT[0], GPIO_MODE_INPUT);
    gpio_set_direction(DIGITAL_INPUT[1], GPIO_MODE_INPUT);
    gpio_set_direction(DIGITAL_INPUT[2], GPIO_MODE_INPUT);
    gpio_set_direction(DIGITAL_INPUT[3], GPIO_MODE_INPUT);
    gpio_set_direction(ERROR_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(ERROR_LED, 1);
    init_adc(); // Initialize ADC
    init_serial(); // Initialize USB Serial JTAG
    
    // UART MUTEX
    uart_mutex = xSemaphoreCreateMutex();
    if (uart_mutex == NULL) {
        ERROR_CHECK(ESP_FAIL);
        return;
    }
    //init read buffer
    input_buffer_init();
    
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
        .clock_speed_hz = SPI_MASTER_FREQ_8M, // Clock speed (8 MHz)
        .spics_io_num = SPI_SYNC,
        .queue_size = 1,
    };

    // Initialize SPI bus
    ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg,0));
    

    // Add device to bus
    // static spi_device_handle_t spi;
    ERROR_CHECK(spi_bus_add_device(SPI2_HOST  , &devcfg, &spi));
    

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
    ERROR_CHECK(spi_device_transmit(spi, &t));
   
    vTaskDelay(100 / portTICK_PERIOD_MS);

// LDAC register
    t.tx_data[0] = 0b00000110;  // Command: Write and update
    t.tx_data[1] = 0b00000000; // DAC value and channel
    t.tx_data[2] = 0b00000000; // DAC value (lower byte)
    t.tx_data[3] = 0b00001111; // DAC value (upper byte)
    ERROR_CHECK(spi_device_transmit(spi, &t));
    vTaskDelay(100 / portTICK_PERIOD_MS);
    








}