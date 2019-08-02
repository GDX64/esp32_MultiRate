#include "task_funcs.h"
#include "mr_pll_funcs.c"
#include "driver/gpio.h"
#include "temp_funcs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define VEC_SIZE 7680
#define PONTOS_POR_CILCO 128
#define MK 16
#define F0 60
#define ALPHA 0.97
#define N_FREQ_FILTER 24
#define TRACK_HARM 1
#define BLINK_GPIO 2
#define DESVIO 0

void mr_pll(void *pvParameter){
  sTask_args *pT_args = (sTask_args*)pvParameter;

  while(1){
    printf("Task%d --> Multi Rate PLL iniciando em 3s...\n",pT_args->ID);
    uint64_t tempo=0; //usado para medir velocidade do codigo;
    time_stats("tempo: ", tempo);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 0);
    vTaskDelay(pdMS_TO_TICKS(3000));
    gpio_set_level(GPIO_NUM_2, 1);
    //declaraçoes
    float ts=1.0/F0/PONTOS_POR_CILCO, u_filt, u; //sampling period and filtered signal variable
    int mk=MK; //por mais estupido que isso pareça, tem sentido
    float fs_dowsampling=1.0/ts/mk; //frequencia vista pelo PLL depois de efetuado o downsampling
    float ts_dowsampling=ts*mk; //periodo de amostragem visto pelo downsampler
    float freq_vector[N_FREQ_FILTER]; //vetor para o filtro de frequencia
    float sample_vec[PONTOS_POR_CILCO]; //vetor com as amostras para teste.
    int filter_counter=0; //contador para o filtro de frequencia
    sinWave s1={.A=1, .w0=(pT_args->harm*F0+DESVIO)*2*M_PI*TRACK_HARM, .fi=0, .e=0, .mi={300.0,500.0,6.0}};

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

    //Inicializa vetor de amostras =====Simulando ADC=======
    for(int i=0; i<PONTOS_POR_CILCO; i++)
      sample_vec[i]=180*sin(pT_args->harm*F0*i*ts*2*M_PI)/pT_args->harm;

    //Loop principal das amostras
    time_stats("iniciando loop principal de amostras tempo: ", tempo);
    for(int i=0; i<VEC_SIZE; i++){
      //Pega amostras ====SIMULANDO ADC========
      u=sample_vec[(int)fmod(i,PONTOS_POR_CILCO)];

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

        //atualizando filtros
        filter_update(&filtro1, ALPHA, vec_mean(freq_vector, N_FREQ_FILTER)*ts);
        filter_update(&filtro2, ALPHA, vec_mean(freq_vector, N_FREQ_FILTER)*ts);

      }//fim do downsampler com pll
    }//fim do for das amostras
    //Resultados
    time_stats("tempo passado desde o loop: ", tempo);
    printf("A=%-5.2f  frequencia=%-5.2f  fi=%-5.2f, e=%-5.2f, t=%5.3fs\n\n\n", s1.A, s1.w/2/M_PI, s1.fi, s1.e, VEC_SIZE*ts);


  }//fim do while(1)
}
