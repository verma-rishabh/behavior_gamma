#ifndef GPIO_H
#define GPIO_H


void gpio_digital_write_task(void *param);
void gpio_digital_read_task(void *param);
void gpio_analog_read_task(void *param);

#endif // GPIO_H