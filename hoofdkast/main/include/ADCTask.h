#ifndef ADCTASK_H
#define ADCTASK_H   
#include <stdint.h>
extern float ADCValue[];
extern uint32_t ADCsamples;

enum { ADC_CURRENT, ADC_5V, ADC_BATT };  // ADC channel functions

void ADCTask(void *pvParameter);
#endif
