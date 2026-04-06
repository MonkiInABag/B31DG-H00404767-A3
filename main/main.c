#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "rom/ets_sys.h"

#include "driver/gpio.h"

#include "pins.h"
#include "workkernel.h"
#include "tasks.h"
#include "monitor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Task execution state
static int64_t T0_us = 0;

// Forward declarations for task runners
uint32_t s_id = 0;
uint32_t a_id = 0;
uint32_t b_id = 0;
uint32_t agg_id = 0;
uint32_t c_id = 0;
uint32_t d_id = 0;

SemaphoreHandle_t semaA;
SemaphoreHandle_t semaB;
SemaphoreHandle_t semaS;
SemaphoreHandle_t token_mutex;

// Task runner functions that wrap task execution with monitor calls
static void Run_TaskA(void)
{
    beginTaskA(a_id++);
    task_A();
    endTaskA();
    xSemaphoreGive(semaA);
}

static void Run_TaskB(void)
{
    beginTaskB(b_id++);
    task_B();
    endTaskB();
    xSemaphoreGive(semaB);
}

static void Run_TaskAGG(void)
{
    beginTaskAGG(agg_id++);
    task_AGG();
    endTaskAGG();
}

static void Run_TaskC(void)
{
    uint32_t release_id = (uint32_t)((esp_timer_get_time() - T0_us) / 50000ULL);
    beginTaskC(release_id);
    task_C();
    endTaskC();
}

static void Run_TaskD(void)
{
    uint32_t release_id = (uint32_t)((esp_timer_get_time() - T0_us) / 50000ULL);
    beginTaskD(release_id);
    task_D();
    endTaskD();
}

static void Run_TaskS(void)
{
        beginTaskS(s_id++);
        task_S();
        endTaskS();
}

// FreeRTOS task functions that call the corresponding task runners and handle timing
static void TaskA_FRTOS(void *pvParameters)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    while(1)
    {
        Run_TaskA();
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(10));
    }
}

static void TaskB_FRTOS(void *pvParameters)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    while(1)
    {
        Run_TaskB();
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(20));
    }
}

static void TaskAGG_FRTOS(void *pvParameters)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    while(1)
    {
        xSemaphoreTake(semaA, portMAX_DELAY);
        xSemaphoreTake(semaB, portMAX_DELAY);
        Run_TaskAGG();
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(20));
    }
}

static void TaskC_FRTOS(void *pvParameters)
{
        TickType_t lastWakeTime = xTaskGetTickCount();
        while(1)
        {
            if (inModeButton() == 1)
            {
                Run_TaskC();
            }
            vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(50));
        }
}

static void TaskD_FRTOS(void *pvParameters)
{
        TickType_t lastWakeTime = xTaskGetTickCount();
        while(1)
        {
            if (inModeButton() == 1)
            {
                Run_TaskD();
            }
            vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(50));
           
        }
}

static void TaskS_FRTOS(void *pvParameters)
{
        while(1)
        {
            xSemaphoreTake(semaS, portMAX_DELAY);
            Run_TaskS();
        }
}

static void TaskMonitor(void *pvParameters)
{
    while (1)
    {
        monitorPollReports();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
   
}


void app_main(void)
{
    // Initialization
    pins_init();
    tasks_init();
    monitorInit();

    // Configure periodic and final reports
    monitorSetPeriodicReportEverySeconds(10);
    monitorSetFinalReportAfterSeconds(60);

    // Disable task watchdog
    esp_task_wdt_delete(NULL);

    // Create semaphores for task synchronization and shared token access
    semaA = xSemaphoreCreateBinary();
    semaB = xSemaphoreCreateBinary();
    semaS = xSemaphoreCreateCounting(10, 0);
    token_mutex = xSemaphoreCreateMutex();

    // Wait for SYNC signal to align the start of the schedule
    while (!pins_sync_seen()) {
        // Wait for sync
    }

    // Record T0 at SYNC and start the schedule
    T0_us = pins_sync_T0_us();
    synch();

    // Create tasks pinned to core 0 with appropriate priorities
    // parameters are: task function, name, stack size, parameters, priority, task handle, core ID
    // Higher priority number means higher priority
    xTaskCreatePinnedToCore(
        TaskA_FRTOS,
        "TaskA",
        2048,
        NULL,
        5, 
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        TaskB_FRTOS,
        "TaskB",
        2048,
        NULL,
        4, 
        NULL,
        0
    );
    xTaskCreatePinnedToCore(
        TaskAGG_FRTOS,
        "TaskAGG",
        2048,
        NULL,
        4, 
        NULL,
        0
    );
    xTaskCreatePinnedToCore(
        TaskC_FRTOS,
        "TaskC",
        2048,
        NULL,
        3, 
        NULL,
        0
    );
    xTaskCreatePinnedToCore(
        TaskD_FRTOS,
        "TaskD",
        2048,
        NULL,
        3, 
        NULL,
        0
    );
    xTaskCreatePinnedToCore(
        TaskS_FRTOS,
        "TaskS",
        2048,
        NULL,
        2, 
        NULL,
        0
    );
    xTaskCreatePinnedToCore(
        TaskMonitor,
        "Monitor",
        2048,
        NULL,
        1, 
        NULL,
        0
    );
}