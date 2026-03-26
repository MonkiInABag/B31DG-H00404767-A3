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


// Task scheduling parameters
#define FRAME_US 10000
#define HYPERFRAME_FRAMES 10
#define TASKS_GAURD_TIME_US 3200

// Task execution state
static int64_t T0_us = 0;
static int64_t nextFrame_us = 0;
static int current_frame = 0;

// Forward declarations for task runners
uint32_t s_id = 0;
uint32_t a_id = 0;
uint32_t b_id = 0;
uint32_t agg_id = 0;
uint32_t c_id = 0;
uint32_t d_id = 0;

// Task runner functions that wrap task execution with monitor calls
static void Run_TaskA(void)
{
    beginTaskA(a_id++);
    task_A();
    endTaskA();
}

static void Run_TaskB(void)
{
    beginTaskB(b_id++);
    task_B();
    endTaskB();
}

static void Run_TaskAGG(void)
{
    beginTaskAGG(agg_id++);
    task_AGG();
    endTaskAGG();
}

static void Run_TaskC(void)
{
    beginTaskC(c_id++);
    task_C();
    endTaskC();
}

static void Run_TaskD(void)
{
    beginTaskD(d_id++);
    task_D();
    endTaskD();
}

static void Run_TaskS(void)
{
    if (pins_take_sporadic_pending()) {
        beginTaskS(s_id++);
        task_S();
        endTaskS();
    }
}

void app_main(void)
{
    // Initialization
    pins_init();
    tasks_init();
    monitorInit();

    // Configure periodic and final reports
    monitorSetPeriodicReportEverySeconds(0);
    monitorSetFinalReportAfterSeconds(60);

    // Disable task watchdog
    esp_task_wdt_delete(NULL);

    // Wait for SYNC signal to align the start of the schedule
    while (!pins_sync_seen()) {
        // Wait for sync
    }

    // Record T0 at SYNC and start the schedule
    T0_us = pins_sync_T0_us();
    synch();

    nextFrame_us = T0_us;
    current_frame = 0;

    while (1)
    {
        // Wait until the exact start of this frame
        while (esp_timer_get_time() < nextFrame_us)
        {
            monitorPollReports();

            //checks if any dealienes are missed
            if (!monitorAllDeadlinesMet())
            {
                gpio_set_level(PIN_ERROR_LED, 1);
            }

            ets_delay_us(50);
        }

        // Update the current frame based on the current time to handle any drift
        int64_t now_us = esp_timer_get_time();
        // if behind schedule, catch up to expected frame based on current time
        while (now_us >= nextFrame_us + FRAME_US)
        {
            nextFrame_us += FRAME_US;
            current_frame = (current_frame + 1) % HYPERFRAME_FRAMES;
            now_us = esp_timer_get_time();
        }

        // Calculate the end time of this frame for later slack calculations
        int64_t frame_end_us = nextFrame_us + FRAME_US;

        // Run the fixed periodic tasks for this frame
        switch (current_frame) {
            case 0:
                Run_TaskA();
                Run_TaskB();
                Run_TaskAGG();
                break;

            case 1:
                Run_TaskA();
                Run_TaskC();
                break;

            case 2:
                Run_TaskA();
                Run_TaskB();
                Run_TaskAGG();
                break;

            case 3:
                Run_TaskA();
                Run_TaskD();
                break;

            case 4:
                Run_TaskA();
                Run_TaskB();
                Run_TaskAGG();
                break;

            case 5:
                Run_TaskA();
                Run_TaskC();
                break;

            case 6:
                Run_TaskA();
                Run_TaskB();
                Run_TaskAGG();
                break;

            case 7:
                Run_TaskA();
                Run_TaskD();
                break;

            case 8:
                Run_TaskA();
                Run_TaskB();
                Run_TaskAGG();
                break;

            case 9:
                Run_TaskA();
                break;

            default:
                break;
        }

        // Try to run one sporadic job only if there is enough slack left
        int64_t post_tasks_us = esp_timer_get_time();
        int64_t time_left_us = frame_end_us - post_tasks_us;

        // Only allow S in frames that actually have space
        if (time_left_us >= TASKS_GAURD_TIME_US)
        {
            Run_TaskS();
        }

        // Wait out the rest of the frame so the 10 ms frame is enforced
        while (esp_timer_get_time() < frame_end_us)
        {
            monitorPollReports();

            if (!monitorAllDeadlinesMet())
            {
                gpio_set_level(PIN_ERROR_LED, 1);
            }

            ets_delay_us(50);
        }

        // Advance to the next exact frame boundary
        nextFrame_us += FRAME_US;
        current_frame = (current_frame + 1) % HYPERFRAME_FRAMES;
    }
}