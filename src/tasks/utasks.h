#ifndef TASKS_H_INCLUDED
#define TASKS_H_INCLUDED

#include "FreeRTOS.h"
#include "queue.h"
#include "rtos_func.h"

#include "mcuinit.h"
#include "usart.h"



void task1(void *args);
void task2(void *args);
void task3(void *args);

#endif /* TASKS_H_INCLUDED */
