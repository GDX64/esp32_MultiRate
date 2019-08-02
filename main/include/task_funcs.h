//estrutura a ser passada dentro das tasks
typedef struct{
  int ID;
  float harm;

} sTask_args;

void mr_pll(void *);
void init_timer(int timer_period_us);
