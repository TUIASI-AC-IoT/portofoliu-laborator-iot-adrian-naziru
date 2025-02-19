#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#define GPIO_OUTPUT_IO 4
#define GPIO_INPUT_IO 2 
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT_IO)  | (0ULL<<GPIO_INPUT_IO)
#define MIN_UNIT_OF_TIME (10 * portTICK_PERIOD_MS) //MS
#define ON 1
#define OFF 0

void (*custom_func)(void *);

static void gpio_task_example(void* arg)
{
   static uint32_t u32butonPressed;
  // printf("cnt: %d\n", u32butonPressed++);
   // 10 ms debounce
}


void app_main() {
//zero-initialize the config structure.
gpio_config_t io_conf = {};
//disable interrupt
io_conf.intr_type = GPIO_INTR_DISABLE;
//set as output mode
io_conf.mode = GPIO_MODE_OUTPUT;
//bit mask of the pins that you want to set
io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
//disable pull-down mode
io_conf.pull_down_en = 0;
//disable pull-up mode
io_conf.pull_up_en = 0;
//configure GPIO with the given settings
gpio_config(&io_conf);
int cnt = 0;

//start gpio task
xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);


while(1) {
printf("cnt: %d\n", cnt++);

//Ton1=1 sec → To f f 1=0,5 sec → Ton2=0,25 sec → To f f 2=0,75 sec
/* ON FOR 1s */
vTaskDelay(1000 / portTICK_PERIOD_MS);
gpio_set_level(GPIO_OUTPUT_IO, ON );
/* OFF FOR 0,5s */
vTaskDelay(500 / portTICK_PERIOD_MS);
gpio_set_level(GPIO_OUTPUT_IO, OFF );

/* ON FOR 1s */
vTaskDelay(250 / portTICK_PERIOD_MS);
gpio_set_level(GPIO_OUTPUT_IO, ON );
/* OFF FOR 0,5s */
vTaskDelay(750 / portTICK_PERIOD_MS);
gpio_set_level(GPIO_OUTPUT_IO, OFF );
}

}
