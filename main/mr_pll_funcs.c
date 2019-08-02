#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include "mr_pll_funcs.h"


//Função do PLL, recebe uma estrutura do tipo sinWave, a amostra mais atual do sinal analisado e
//e o período de amostragem
void ePLL(sinWave *sWave, float u, float ts){
  float w_anterior=sWave->wa;
  sWave->e = u-sWave->A*sin(sWave->fi);

  sWave->A += (sWave->mi[0])*ts*(sWave->e)*sin(sWave->fi);

  sWave->wa += sWave->mi[1]*ts*sWave->e*cos(sWave->fi);

  sWave->fi += ts*w_anterior+sWave->mi[2]*ts*(sWave->e)*cos(sWave->fi);

  //calculando frequencia real
  sWave->w = (sWave->wa - sWave->wa_0)*sWave->sinal+sWave->w0;
}

//encontra a frequencia aparente para iniciar o PLL
//fs : frequencia de amostragem depois do downsampling
//f_init: frequencia para transformar
float freq_finder(const float fs, const float f_init, int *flag){
  float f_aparente=f_init;
  *flag=1;

  if(f_aparente>(fs/2)){
    f_aparente=fmod(f_init/fs, 1);
    if(f_aparente<=0){
      f_aparente=f_aparente*fs;
    }
    else{
      f_aparente=(1-f_aparente)*fs;
      *flag=-1;
    }
  }

  return f_aparente;
}

//funcao para utilizar um filtro de ordem 2
float iir_filter(filter_struct *filtro, float amostra){
  float saida=0;
  //deslocado buffer_x
  filtro->buff_x[2]=filtro->buff_x[1];
  filtro->buff_x[1]=filtro->buff_x[0];
  filtro->buff_x[0]=amostra;

  //calculando saida
  for(int i=0; i<2; i++){
    saida += filtro->buff_x[i]*filtro->num[i];
    saida -= filtro->buff_y[i]*filtro->den[i+1];
  }
  saida+=filtro->buff_x[2]*filtro->num[2];
  //deslocando buffer_y
  filtro->buff_y[1]=filtro->buff_y[0];
  filtro->buff_y[0]=saida;

  return saida;
}

//Funcao utilizada para inicializar o filtro
void filter_init(filter_struct *filtro, float alpha, float freq_norm){
  float beta=cos(freq_norm);

  filtro->num[0]=(1.0-alpha)/2.0;
  filtro->num[1]=0;
  filtro->num[2]=-filtro->num[0];

  filtro->den[0]=1;
  filtro->den[1]=-beta*(1.0+alpha);
  filtro->den[2]=alpha;
}
//Atualiza coeficiente do filtro
void filter_update(filter_struct *filtro, float alpha, float freq_norm){
  float beta=cos(freq_norm);
  filtro->den[1]=-beta*(1.0+alpha);
}

float vec_mean(float vec[], int size){
  float mean=0;
  for(int i=0; i<size; i++)
    mean+=vec[i];

  return mean/size;
}
