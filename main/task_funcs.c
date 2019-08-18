#include "task_funcs.h"
#include "mr_pll_funcs.c"
#include "driver/gpio.h"
#include "temp_funcs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "common.h"

#define FATOR_NORM 1
#define VEC_SIZE 7680
#define PONTOS_POR_CILCO 128
#define MK 16
#define F0 60
#define ALPHA 0.97
#define N_FREQ_FILTER 24
#define N_REPORT_FILTERS 64
#define TRACK_HARM 1
#define BLINK_GPIO 2
#define DESVIO 0
#define BUFF_SIZE (SAMPLE_VEC_SIZE/4)//Tamanho do buffer em amostras

//Handle for the queue report
extern QueueHandle_t qh_report;

void mr_pll(void *pvParameter){
	sTask_args *pT_args = (sTask_args*)pvParameter;

    printf("Task%d --> Multi Rate PLL iniciando em 3s...\n",pT_args->ID);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 1);
    vTaskDelay(pdMS_TO_TICKS(3000));
    gpio_set_level(GPIO_NUM_2, 0);
    printf("mr_pll%d: Vai comecar a ousadia\n", pT_args->ID);
    //declaraçoes
    float ts=1.0/F0/PONTOS_POR_CILCO/FATOR_NORM, u_filt, u; //sampling period and filtered signal variable
    int mk=MK; //por mais estupido que isso pareça, tem sentido
    float fs_dowsampling=1.0/ts/mk; //frequencia vista pelo PLL depois de efetuado o downsampling
    float ts_dowsampling=ts*mk; //periodo de amostragem visto pelo downsampler
    float freq_vector[N_FREQ_FILTER]; //vetor para o filtro de frequencia
    float *sample_vec; //vetor com as amostras para teste.
    float report_amp_vector[N_REPORT_FILTERS]; //vetor para media de amplitude (para reportar)
    float report_freq_vector[N_REPORT_FILTERS]; //vetor para media de frequencia (para reportar)
    int filter_counter=0; //contador para o filtro de frequencia
    int report_filter_counter=0; //filtro para o reporte
    int buff_track=0; //para a mudança de buffer
    int n_loops=0; //contagem de loops
    sinWave s1={.A=1, .w0=(pT_args->harm*F0+DESVIO)*2*M_PI*TRACK_HARM, .fi=0, .e=0, .mi={300.0,500.0,6.0}};
    sTask_report t_report={.ID=pT_args->ID, .frequency=0, .amplitude=0}; //report struct

    //inicializa frequencia para o PLL
    s1.wa_0=freq_finder(fs_dowsampling*2*M_PI, s1.w0, &s1.sinal);
    //Estrutura que pode alterar o valor de downsampling de acordo com o risco de
    if((s1.wa_0<60) | (s1.wa_0>(fs_dowsampling*M_PI-60)))
    {
      mk+=4;
      fs_dowsampling=1.0/ts/mk; //frequencia vista pelo PLL depois de efetuado o downsampling
      ts_dowsampling=ts*mk; //periodo de amostragem visto pelo downsampling
      s1.wa_0=freq_finder(fs_dowsampling*2*M_PI, s1.w0, &s1.sinal);
    }
    s1.wa=s1.wa_0;

    //Inicializando filtro
    filter_struct filtro1={.num={1,1,1}, .den={1,1,1}, .buff_x={0}, .buff_y={0}}, filtro2;
    filter_init(&filtro1, ALPHA, s1.w0*ts); //tem que passar a frequencia normalizada como ultimo argumento
    filtro2=filtro1;

    //inicializando filtro de frequencia
    for(int i=0; i<N_FREQ_FILTER; i++)
      freq_vector[i]=s1.w0;

    //Loop principal das amostras
    while(1){
    	//retorna buffer disponível
    	sample_vec=cm_buffer_wait(&buff_track);
		for(int i=0; i<BUFF_SIZE; i++){
		  //Pega amostras ====SIMULANDO ADC========
		  u=sample_vec[i];

		  //filtragem
		  u_filt=iir_filter(&filtro1, u);//filtrando primeira vez
		  u_filt=iir_filter(&filtro2, u_filt);//filtrando outra vez

		  //downsampling
		  if(fmod(i,mk)==0){
			ePLL(&s1, u_filt, ts_dowsampling);
			//Printando resultados
			//printf("it %-3d: u=%5.2f  uf=%5.2f  A=%-5.2f  frequencia=%-5.2f  fi=%-5.2f, e=%-5.2f, t=%5.3fs\n", i, u, u_filt, s1.A, s1.w/2/M_PI, s1.fi, s1.e, i*ts);

			//atualizando frequencia
			if(filter_counter>=N_FREQ_FILTER){
			  filter_counter=0;
			}
			freq_vector[filter_counter]=s1.w;
			filter_counter++;//atualiza contador do filtro

			//filtros para exportar
			if(report_filter_counter>=N_REPORT_FILTERS){
			  report_filter_counter=0;
			}
			report_freq_vector[report_filter_counter]=s1.w;
			report_amp_vector[report_filter_counter]=s1.A;
			report_filter_counter++;//atualiza contador do filtro



			//atualizando filtros
			filter_update(&filtro1, ALPHA, vec_mean(freq_vector, N_FREQ_FILTER)*ts);
			filter_update(&filtro2, ALPHA, vec_mean(freq_vector, N_FREQ_FILTER)*ts);

		  }//fim do downsampler com pll
		}//fim do for das amostras
		//Resultados
		n_loops++;
		printf("ID%d -- A=%-5.2f  frequencia=%-5.2f  fi=%-5.2f, e=%-5.2f, n=%5d\n", pT_args->ID, s1.A, s1.w/2/M_PI, s1.fi, s1.e, BUFF_SIZE*n_loops);
		//Sending report to server
		t_report.frequency=vec_mean(report_freq_vector, N_REPORT_FILTERS)/2/M_PI;
		t_report.amplitude=vec_mean(report_amp_vector, N_REPORT_FILTERS);
		xQueueSendToBack(qh_report, &t_report, 0);
    }//fim do while(1)
}
