#include "input_buffer.h"
#include "driver/gpio.h"
#include "init.h"

input_entry_t input_buffer[NUM_INPUTS];
SemaphoreHandle_t input_buffer_mutex = NULL;

void input_buffer_init(void) {
    input_buffer_mutex = xSemaphoreCreateMutex();
    for (int i = 0; i < NUM_INPUTS; ++i) {
        input_buffer[i].value = 0;
        input_buffer[i].consumed = true;
    }
}

// Dummy input read function, replace with your actual input read logic
int read_input(int pin) {
    int8_t mapped_pin = DIGITAL_INPUT[pin]; // Convert to GPIO number
    uint16_t state = (uint16_t)gpio_get_level(mapped_pin);
    return state;
}

void input_buffer_update_task(void *param) {
    while (1) {
        for (int i = 0; i < NUM_INPUTS; ++i) {
            xSemaphoreTake(input_buffer_mutex, portMAX_DELAY);
            int new_value = read_input(i);
            if (input_buffer[i].consumed || new_value) {
                input_buffer[i].value = new_value;
                input_buffer[i].consumed = false;
            }
            xSemaphoreGive(input_buffer_mutex);
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
} 