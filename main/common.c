#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "common.h"
//Funçao para encontrar buffer disponivel
extern float buff_vec0[], buff_vec1[];
extern EventGroupHandle_t adc_event_group;

float *cm_buffer_check(){
	EventBits_t ebits=xEventGroupWaitBits(adc_event_group,
	0x05,
	false,
	true,
	0);
	printf("ebits: %d\n", ebits);
	if(ebits==2){
		return buff_vec0;
	}
	else{
		return buff_vec1;
	}
	return buff_vec0;
}

//usado nas pll_tasks
float *cm_buffer_wait(int *buff_track){
	if(*buff_track==0){
		*buff_track=1;
		xEventGroupWaitBits(adc_event_group,
			0x01,
			false,
			true,
			portMAX_DELAY);
		return buff_vec1;
	}
	else{
		*buff_track=0;
		xEventGroupWaitBits(adc_event_group,
					0x02,
					false,
					true,
					portMAX_DELAY);
		return buff_vec0;
	}
}



