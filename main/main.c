/*
  Usando PWM, ADC e filas
  1- Task adc_task: le valores na porta 35 de 0 a 2.2V com precisao de 12 bits
  faz uma média de N_LEITURAS amostras e envia pra uma fila qh_pwmADC

  2- Task pwm_task: recebe os valores da task do ADC e muda o dutty cicle de acordo com eles
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "driver/adc.h"
#include "temp_funcs.h"
#include "freertos/queue.h"

#include "common.h"
#include "tcp_server.c"
#include "sin_pwm.h"
#include "task_funcs.h"

#define N_LEITURAS 1
#define ADC_GPIO   35
#define PWM_GPIO   18
#define N_HARM     2

//declarando filas
QueueHandle_t qh_timer;
QueueHandle_t qh_pwmADC;
QueueHandle_t qh_ISRADC;
QueueHandle_t qh_report;
//Declarando eventos
EventGroupHandle_t adc_event_group;
//coisas do wifi
float buff_vec0[SAMPLE_VEC_SIZE], buff_vec1[SAMPLE_VEC_SIZE]={0};

void pwm_task(void *pvParameters){
  //configureing the timer
  ledc_timer_config_t ledc_timer = {
      .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
      .freq_hz = 5000,                      // frequency of PWM signal
      .speed_mode = LEDC_HIGH_SPEED_MODE,           // timer mode
      .timer_num = LEDC_TIMER_0            // timer index
  };

  //estrutura de configuraçao de canal
  ledc_channel_config_t ledc_channel={
    .channel    = LEDC_CHANNEL_0,
    .duty       = 0,
    .gpio_num   = PWM_GPIO,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .hpoint     = 0,
    .timer_sel  = LEDC_TIMER_0
  };
  //configurando timer
  ledc_timer_config(&ledc_timer);
  //configurando canal
  ledc_channel_config(&ledc_channel);
  printf("O led deve estar aceso agora\n");
  ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
  //ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
  uint32_t brilho_led=0;
  //Loop principal
  while(1){
    //Recebendo valor do brilho vindo da fila
    xQueueReceive(qh_pwmADC, &brilho_led, portMAX_DELAY);
    brilho_led*=2;
    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, brilho_led);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
    //printf("Brilho em: %d\n", brilho_led);
  }
}

//troca de buffer o ADC
//O bit setado corresponde ao buffer EM USO
float *change_context(int *flag){
	if(*flag==1){
		*flag=0;
		xEventGroupClearBits(adc_event_group, (2));
		xEventGroupSetBits(adc_event_group, (1));
		return buff_vec0;
	}else
	{
		*flag=1;
		xEventGroupClearBits(adc_event_group, (1));
		xEventGroupSetBits(adc_event_group, (2));
		return buff_vec1;
	}

}

// Task do ADC (Bem sugestivo, nao acha?)
void adc_task(void *pvParameters){
  //Vetor das amostras do adc
  float *sample_vec=buff_vec0;
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_7,ADC_ATTEN_DB_11);
  int val = 0;
  double dummy_queue=0;
  BaseType_t q_info=0;
  int i_vec=0;//index de amostras do wifi
  int buff_flag=0;
//Loop principal
  while(1){
	xQueueReceive(qh_ISRADC, &dummy_queue, portMAX_DELAY);
    val=adc1_get_raw(ADC1_CHANNEL_7);
    //time_stats("Tempo pro ADC: ");
    //enviando leituras na fila
    q_info=xQueueSendToBack(qh_pwmADC, &val, 0); //tb pode colcoar portMAX_DELAY
    //colocando coisas no vetor de envio do wifi
    sample_vec[i_vec]=((float)val/(1<<12)-0.36)*620;
    i_vec++;
    if(i_vec>=SAMPLE_VEC_SIZE){
      i_vec=0;
      sample_vec=change_context(&buff_flag);
    }
    //check da fila
    //if(q_info!=pdPASS)
     // printf("Fila cheia\n");

    //vTaskDelay(pdMS_TO_TICKS(10));
    //printf("val: %d\n", val);


  }
}

//teste de interrupcao, nao usar junto com sin_pwm
void look_time_task(void *pvParameters){
	printf("Look time task running\n");
	double tempo=0;
	while(1){
		xQueueReceive(qh_timer, &tempo, portMAX_DELAY);
		printf("Interrupt tempo: %lf\n", tempo);
	}
}

void look_time_task2(void *pvParameters){
	printf("Look time task running\n");
	while(1){
		vTaskDelay(pdMS_TO_TICKS(1000));
		time_stats_sec("t2 Tempo: ");
	}
}

//Inicia o wifi
void wifi_init_here(){
//wifi
  ESP_ERROR_CHECK(nvs_flash_init());
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(example_connect());
}

void app_main(){

  wifi_init_here();
  printf("Vai começar a ousadia\n");
  //iniciando fila
  qh_pwmADC = xQueueCreate(10, sizeof(int));
  qh_timer = xQueueCreate(10, sizeof(double));
  qh_ISRADC = xQueueCreate(10, sizeof(double));
  qh_report = xQueueCreate(64, sizeof(sTask_report));
  //iniciando grupo de eventos
  adc_event_group = xEventGroupCreate();
  //iniciando timers
  init_timer_g1_t1(INTERRUPT_PERIOD);
  init_timer_g1_t0(ADC_PERIOD);
  init_timer_g0_t0(1000);
  //configurando pll
  float v_harm[N_HARM]={1.0, 3.0};
      //Inicializando todas as tasks do PLL
      for(int i=0; i<N_HARM; i++){
          sTask_args *pTargs=malloc(sizeof(sTask_args));
          pTargs->harm=v_harm[i];//passando harmonico a ser rastreado
          pTargs->ID=i; //passando ID da task
          xTaskCreate(&mr_pll, "mr_pll_task1", (1<<12), (void*)pTargs, 5, NULL);
      }

  //Criando tasks
  xTaskCreate(&pwm_task, "pwm_task", (1<<11), NULL, 5, NULL);
  xTaskCreate(&adc_task, "adc_task", (1<<11), NULL, 10, NULL);
  //xTaskCreate(&look_time_task, "time_task", (1<<11), NULL, 6, NULL);
  //xTaskCreate(&look_time_task2, "time_task2", (1<<11), NULL, 5, NULL);
  xTaskCreate(&sin_task, "sin_task", (1<<12), NULL, 6, NULL);
  xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
  //xTaskCreate(tcp_server_report_task, "tcp_server", 4096, NULL, 5, NULL);
  printf("Tasks rodando...\n");
}
