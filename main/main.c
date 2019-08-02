/* PLL_MULTITAXA

*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <stddef.h>
#include "esp_intr_alloc.h"
#include "esp_attr.h"
#include "driver/timer.h"
#include <math.h>
#include "temp_funcs.h"
#include "task_funcs.h"


//task simulando o ADC
void adc_task(void *pvParameter){
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
  float sinal=0;
  while(1){

      sinal=0;
      vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(13));
  }
}

#define N_HARM 4

void app_main()
{
    init_timer(100);
    printf("o tick tá valendo %d\n", pdMS_TO_TICKS(13));
    //vetor de frequencias
    float v_harm[N_HARM]={4.0, 3.0, 1.0, 9.0};
    //Inicializando todas as tasks do PLL
    for(int i=0; i<N_HARM; i++){
        sTask_args *pTargs=malloc(sizeof(sTask_args));
        pTargs->harm=v_harm[i];//passando harmonico a ser rastreado
        pTargs->ID=i; //passando ID da task
        xTaskCreate(&mr_pll, "mr_pll_task1", (1<<12), (void*)pTargs, 5, NULL);
    }
    //ADC
    //xTaskCreate(&adc_task, "adc_task", (1<<10), NULL, 6, NULL);
    //vTaskStartScheduler();
}
