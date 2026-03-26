#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

// Pin definitions
#define PIN_SYNC     19
#define PIN_IN_A     18
#define PIN_IN_B     5
#define PIN_IN_S     17

#define PIN_ACK_A    27
#define PIN_ACK_B    26
#define PIN_ACK_AGG  25
#define PIN_ACK_C    33
#define PIN_ACK_D    32
#define PIN_ACK_S    14

#define PIN_ERROR_LED 13

// Function declarations
void pins_init(void);
bool pins_sync_seen(void);
int64_t pins_sync_T0_us(void);

int64_t pins_edgesA_delta(void);
int64_t pins_edgesB_delta(void);
bool pins_take_sporadic_pending(void);
bool pins_has_sporadic_pending(void);