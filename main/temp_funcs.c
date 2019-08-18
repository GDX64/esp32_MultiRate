#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
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

static intr_handle_t s_timer_handle_isr;
static intr_handle_t s_timer_handle_isr_ADC;
//fila da interrupçao
extern QueueHandle_t qh_timer;
extern QueueHandle_t qh_ISRADC;
//funcao apenas para printar o tempo
void time_stats(char *text){
  uint64_t tempo;
  timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &tempo);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
  printf("%s %lld\n", text, tempo);
}
//time stats mas em segundos
void time_stats_sec(char *text){
  static double tempo_anterior=0;
  double tempo;
  timer_get_counter_time_sec(TIMER_GROUP_0, TIMER_0, &tempo);
  printf("%s %lf\n", text, tempo-tempo_anterior);
  tempo_anterior=tempo;
}

//Interrupt
static void  IRAM_ATTR timer_isr(void* arg)
{
    TIMERG1.int_clr_timers.t1 = 1;
    TIMERG1.hw_timer[1].config.alarm_en = TIMER_ALARM_EN;

    //getting the time
    double tempo;
    timer_get_counter_time_sec(TIMER_GROUP_1, TIMER_1, &tempo);
    // your code, runs in the interrupt
    xQueueSendFromISR(qh_timer, &tempo, NULL);
}

void init_timer_g1_t1(int timer_period_us)
{
    timer_config_t config = {
            .alarm_en = TIMER_ALARM_EN,
            .counter_en = TIMER_PAUSE,
            .intr_type = TIMER_INTR_LEVEL,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = 1,
            .divider = 80   /* 1 us per tick */
    };

    timer_init(TIMER_GROUP_1, TIMER_1, &config);
    timer_set_counter_value(TIMER_GROUP_1, TIMER_1, 0);
    timer_set_alarm_value(TIMER_GROUP_1, TIMER_1, timer_period_us);
    timer_enable_intr(TIMER_GROUP_1, TIMER_1);
    timer_isr_register(TIMER_GROUP_1, TIMER_1, &timer_isr, NULL, 0, &s_timer_handle_isr);

    timer_start(TIMER_GROUP_1, TIMER_1);
}

//################## Timer 1

//interrupcao do ADC
static void  IRAM_ATTR timer_isr_ADC(void* arg)
{
    TIMERG1.int_clr_timers.t0 = 1;
    TIMERG1.hw_timer[0].config.alarm_en = TIMER_ALARM_EN;

    //getting the time
    double tempo;
    timer_get_counter_time_sec(TIMER_GROUP_1, TIMER_0, &tempo);
    // your code, runs in the interrupt
    xQueueSendFromISR(qh_ISRADC, &tempo, NULL);
}
//timer 0 grupo 1
void init_timer_g1_t0(int timer_period_us)
{
    timer_config_t config = {
            .alarm_en = TIMER_ALARM_EN,
            .counter_en = TIMER_PAUSE,
            .intr_type = TIMER_INTR_LEVEL,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = 1,
            .divider = 80   /* 1 us per tick */
    };

    timer_init(TIMER_GROUP_1, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_1, TIMER_0, 0);
    timer_set_alarm_value(TIMER_GROUP_1, TIMER_0, timer_period_us);
    timer_enable_intr(TIMER_GROUP_1, TIMER_0);
    timer_isr_register(TIMER_GROUP_1, TIMER_0, &timer_isr_ADC, NULL, 0, &s_timer_handle_isr_ADC);

    timer_start(TIMER_GROUP_1, TIMER_0);
}

//timer 0 grupo 0
void init_timer_g0_t0(int timer_period_us)
{
    timer_config_t config = {
            .alarm_en = TIMER_ALARM_DIS,
            .counter_en = TIMER_PAUSE,
            .intr_type = TIMER_INTR_LEVEL,
            .counter_dir = TIMER_COUNT_UP,
            .auto_reload = 1,
            .divider = 80   /* 1 us per tick */
    };

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    //timer_set_alarm_value(TIMER_GROUP_1, TIMER_0, timer_period_us);
    //timer_enable_intr(TIMER_GROUP_1, TIMER_0);
    //timer_isr_register(TIMER_GROUP_1, TIMER_0, &timer_isr_ADC, NULL, 0, &s_timer_handle_isr_ADC);

    timer_start(TIMER_GROUP_0, TIMER_0);
}
