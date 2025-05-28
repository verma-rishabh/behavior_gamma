#ifndef INIT_H
#define INIT_H

// Include necessary headers
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
// #include "pin_config.h"

#define SPI_CLK 48
#define SPI_DIN 38
#define SPI_SYNC 47

#define ERROR_LED 46
extern int DIGITAL_OUTPUT[];
extern int DIGITAL_INPUT[];


#define ERROR_CHECK(x) do {                                         \
    esp_err_t err_rc_ = (x);                                        \
    if (unlikely(err_rc_ != ESP_OK)) {                              \
        gpio_set_level(ERROR_LED, 0);            \
    }                                                               \
} while(0)

extern spi_device_handle_t spi;
extern esp_err_t ret;
extern adc_cali_handle_t adc1_cali_chan0_handle;
extern adc_cali_handle_t adc1_cali_chan1_handle;
extern adc_cali_handle_t adc2_cali_chan0_handle;
extern adc_cali_handle_t adc2_cali_chan1_handle;
extern adc_oneshot_unit_handle_t adc1_handle;
extern adc_oneshot_unit_handle_t adc2_handle;
extern SemaphoreHandle_t uart_mutex;
// Function declarations
void init_task(void);



#endif // INIT_H