#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


// Function declarations for tasks
void tasks_init(void);
void task_A(void);
void task_B(void);
void task_AGG(void);
void task_C(void);
void task_D(void);
void task_S(void);
extern SemaphoreHandle_t token_mutex; // Mutex for protecting tokenA and tokenB access