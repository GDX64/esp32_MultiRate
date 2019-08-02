#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <stddef.h>
#include "esp_intr_alloc.h"
#include "esp_attr.h"
#include "driver/timer.h"
#include <math.h>
#include "temp_funcs.h"

static intr_handle_t s_timer_handle;

static void timer_isr(void* arg)
{
    TIMERG1.int_clr_timers.t0 = 1;
    TIMERG1.hw_timer[0].config.alarm_en = 1;

    // your code, runs in the interrupt
}

void init_timer(int timer_period_us)
{
    timer_config_t config = {
            .alarm_en = false,
            .counter_en = false,
            .intr_type = 1,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = false,
            .divider = 80   /* 1 us per tick */
    };

    timer_init(TIMER_GROUP_1, TIMER_1, &config);
    timer_set_counter_value(TIMER_GROUP_1, TIMER_1, 0);
    timer_set_alarm_value(TIMER_GROUP_1, TIMER_1, timer_period_us);
    //timer_enable_intr(TIMER_GROUP_1, TIMER_1);
    timer_isr_register(TIMER_GROUP_1, TIMER_1, &timer_isr, NULL, 0, &s_timer_handle);

    timer_start(TIMER_GROUP_1, TIMER_1);
}

//funcao apenas para printar o tempo
void time_stats(char *text, uint64_t tempo){
  timer_get_counter_value(TIMER_GROUP_1, TIMER_1, &tempo);
  timer_set_counter_value(TIMER_GROUP_1, TIMER_1, 0);
  printf("%s %lld\n", text, tempo);
}
