//sinWave structure
typedef struct {
  float A; //amplitude
  float w; //frequencia
  float w0; //frequencia inicial
  float wa; //frequencia aparente
  float wa_0; //frequencia aparente inicial, nunca muda
  float fi; //fase
  float y; //valor atual
  float e; //erro atual
  int sinal; //sinal dentro do PLL
  float mi[3]; //constantes PLL

} sinWave;

//estrutura do filtro
typedef struct{
  float num[3];
  float den[3];
  float buff_x[3];
  float buff_y[3];
} filter_struct;

float freq_finder(const float, const float, int *);
void ePLL(sinWave *, float , float);
float iir_filter(filter_struct *filtro, float amostra);
void filter_init(filter_struct *filtro, float alpha, float freq_norm);
float * readVector(int , char []);
void filter_update(filter_struct *, float, float);
float vec_mean(float *, int);
