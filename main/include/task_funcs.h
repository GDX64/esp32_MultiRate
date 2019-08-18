#ifndef TASK_FUNCS_H
#define TASK_FUNCS_H
//estrutura a ser passada dentro das tasks
typedef struct{
  int ID;
  float harm;

} sTask_args;

typedef struct{
	int ID;
	float frequency;
	float amplitude;
} sTask_report;

void mr_pll(void *);
void init_timer(int timer_period_us);

#endif
