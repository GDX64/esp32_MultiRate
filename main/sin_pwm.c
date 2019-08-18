#include "sin_pwm.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include <math.h>
#include "esp_log.h"
#include "freertos/queue.h"
#include "temp_funcs.h"

//handle da fila inicializado na main
extern QueueHandle_t qh_timer;

void sin_task(void *pvParameters){
  //configureing the timer
  ledc_timer_config_t ledc_timer = {
      .duty_resolution = LEDC_TIMER_10_BIT, // resolution of PWM duty
      .freq_hz = PWM_FREQ,                      // frequency of PWM signal
      .speed_mode = LEDC_HIGH_SPEED_MODE,           // timer mode
      .timer_num = LEDC_TIMER_3            // timer index
  };

  //estrutura de configuraçao de canal
  ledc_channel_config_t ledc_channel={
    .channel    = LEDC_CHANNEL_1,
    .duty       = 0,
    .gpio_num   = SIN_PWM_GPIO,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .hpoint     = 0,
    .timer_sel  = LEDC_TIMER_3
  };
  //configurando timer
  ledc_timer_config(&ledc_timer);
  //configurando canal
  ledc_channel_config(&ledc_channel);
  printf("PWM configurado\n");
  int duty = 0;
  double tempo_q=0; //apenas para ser passado para a fila
  double tempo=0;
  //Loop eterno senoidal
  while(1){
	  ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, duty);
	  //time_stats_sec("t: ");
	  //recebendo mensagem do timer
	  xQueueReceive(qh_timer, &tempo_q, portMAX_DELAY);
	  //setando ADC
	  ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
	  tempo+=(double)INTERRUPT_PERIOD/1000000;
	  duty=(int)(((sin(SIN_FREQ*2*M_PI*tempo)+0.2*sin(SIN_FREQ*2*3*M_PI*tempo))/3+0.5)*(1<<10));
	  //printf("D: %d, t: %lf\n", duty, tempo);

  }
}
