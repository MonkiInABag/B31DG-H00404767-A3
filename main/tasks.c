#include "tasks.h"

#include <stdio.h>
#include "driver/gpio.h"

#include "pins.h"
#include "workkernel.h"
#include "monitor.h"

// Task budgets in cycles
#define BUDGET_A   672000U
#define BUDGET_B   960000U
#define BUDGET_AGG 480000U
#define BUDGET_C  1680000U
#define BUDGET_D   960000U
#define BUDGET_S   600000U

// Task release indexes (IDX) start at 0 after SYNC alignment
static uint32_t idxA = 0;
static uint32_t idxB = 0;
static uint32_t idxAGG = 0;
static uint32_t idxC = 0;
static uint32_t idxD = 0;
static uint32_t idxS = 0;

// Latest tokens for AGG
static uint32_t tokenA = 0;
static uint32_t tokenB = 0;
static bool tokenA_valid = false;
static bool tokenB_valid = false;

// Initialize task state
void tasks_init(void)
{
    idxA = idxB = idxAGG = idxC = idxD = idxS = 0;
    tokenA = tokenB = 0;
    tokenA_valid = tokenB_valid = false;
}

// Helper functions to set ACK pins high/low
static inline void ack_high(gpio_num_t pin) { gpio_set_level(pin, 1); }
static inline void ack_low(gpio_num_t pin)  { gpio_set_level(pin, 0); }

// Task implementations

// 1. Task A and B read their respective edge counts
// 2. They then run the WorkKernel with their count and a unique seed
// 3. Task AGG reads the latest tokens from A and B
void task_A(void)
{
    ack_high(PIN_ACK_A);

    uint32_t countA = (uint32_t)pins_edgesA_delta();
    uint32_t seed   = (idxA << 16) ^ countA ^ 0xA1;
    uint32_t tokenA_local = WorkKernel(BUDGET_A, seed);
    // Update shared token for AGG with mutex protection
    xSemaphoreTake(token_mutex, portMAX_DELAY);
    tokenA          = tokenA_local;
    tokenA_valid    = true;
    xSemaphoreGive(token_mutex);

    ack_low(PIN_ACK_A);
    /*
    printf("A,%lu,%lu,%lu\n",
           (unsigned long)idxA,
           (unsigned long)countA,
           (unsigned long)tokenA);
    */
    idxA++;
}

void task_B(void)
{
    ack_high(PIN_ACK_B);

    uint32_t countB = (uint32_t)pins_edgesB_delta();
    uint32_t seed   = (idxB << 16) ^ countB ^ 0xB2;
    // Update shared token for AGG with mutex protection
    uint32_t tokenB_local = WorkKernel(BUDGET_B, seed);
    xSemaphoreTake(token_mutex, portMAX_DELAY);
    tokenB          = tokenB_local;
    tokenB_valid    = true;
    xSemaphoreGive(token_mutex);

    ack_low(PIN_ACK_B);
    /*
    printf("B,%lu,%lu,%lu\n",
           (unsigned long)idxB,
           (unsigned long)countB,
           (unsigned long)tokenB);
    */
    idxB++;
}

// 1. Task AGG reads latest tokens from A and B with mutex protection
// 2. If both tokens are valid, it computes agg = tokenA XOR tokenB, else uses a sentinel value
// 3. Task AGG then runs the WorkKernel with the computed agg value
void task_AGG(void)
{
    ack_high(PIN_ACK_AGG);
    
    uint32_t localA = 0;
    uint32_t localB = 0;
    bool validA = false;
    bool validB = false;
    // Read latest tokens from A and B with mutex protection
    xSemaphoreTake(token_mutex, portMAX_DELAY);
    localA = tokenA;
    localB = tokenB;
    validA = tokenA_valid;
    validB = tokenB_valid;
    xSemaphoreGive(token_mutex);
    uint32_t agg;
        if (validA && validB) {
        agg = tokenA ^ tokenB;
    } else {
        agg = 0xDEADBEEF; // Sentinel value if tokens are not valid
    }

    uint32_t seed   = (idxAGG << 16) ^ agg ^ 0xD4;
    uint32_t tokenAGG = WorkKernel(BUDGET_AGG, seed);

    ack_low(PIN_ACK_AGG);
    /*
    printf("AGG,%lu,%lu,%lu\n",
           (unsigned long)idxAGG,
           (unsigned long)agg,
           (unsigned long)tokenAGG);
    */
    idxAGG++;
}

void task_C(void)
{
    ack_high(PIN_ACK_C);

    uint32_t seed   = (idxC << 16) ^ 0xC3;
    uint32_t tokenC = WorkKernel(BUDGET_C, seed);

    ack_low(PIN_ACK_C);
    /*
    printf("C,%lu,%lu\n",
           (unsigned long)idxC,
           (unsigned long)tokenC);
    */
    idxC++;
}

void task_D(void)
{
    ack_high(PIN_ACK_D);

    uint32_t seed   = (idxD << 16) ^ 0xD5;
    uint32_t tokenD = WorkKernel(BUDGET_D, seed);

    ack_low(PIN_ACK_D);
    /*
    printf("D,%lu,%lu\n",
           (unsigned long)idxD,
           (unsigned long)tokenD);
    */
    idxD++;
}

void task_S(void)
{
    ack_high(PIN_ACK_S);

    uint32_t seed   = (idxS << 16) ^ 0x55;
    uint32_t tokenS = WorkKernel(BUDGET_S, seed);

    ack_low(PIN_ACK_S);
    /*
    printf("S,%lu,%lu\n",
           (unsigned long)idxS,
           (unsigned long)tokenS);
    */
    idxS++;
}