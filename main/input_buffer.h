#ifndef INPUT_BUFFER_H
#define INPUT_BUFFER_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdbool.h>

#define NUM_INPUTS 4 // Change as needed

typedef struct {
    int value;
    bool consumed;
} input_entry_t;

extern input_entry_t input_buffer[NUM_INPUTS];
extern SemaphoreHandle_t input_buffer_mutex;

void input_buffer_init(void);
int read_input(int pin); // Dummy, to be replaced with actual input logic
void input_buffer_update_task(void *param);

#endif 