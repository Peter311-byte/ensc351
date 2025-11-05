#ifndef _SAMPLER_H_
#define _SAMPLER_H_



void sampler_init(void);
void sampler_cleanup(void);

void sampler_moveCurrentDataHistory(void);

double* sampler_getHistory(int* size);

double sampler_getAverageReading(void);

int sampler_getHistorySize(void);

int getTotalNumberofSamples(void);

int getTotalNumberofDips(void);
#endif