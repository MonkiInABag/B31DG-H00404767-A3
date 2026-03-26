#include "pins.h"
#include "monitor.h"

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_timer.h"
#include "esp_attr.h"

// Forward declaration for ISR handler
static void IRAM_ATTR in_s_isr(void *arg);

// Sync states
static volatile bool sync_latched = false;
static int64_t sync_t0 = 0;
static volatile int last_sync_level = 0;

// Pulse counter handles
static pcnt_unit_handle_t pcnt_unit_A = NULL;
static pcnt_unit_handle_t pcnt_unit_B = NULL;

// Sporadic event state
static volatile uint32_t sporadic_pending_count = 0;
static volatile int last_sporadic_us = 0;

// Edge counting state
static int last_countA = 0;
static int last_countB = 0;

// pin setup
void pins_init(void)
{
    // Configure GPIO pins for sync, inputs, acks, and error LED
    gpio_set_direction(PIN_SYNC, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_SYNC, GPIO_PULLDOWN_ONLY);

    gpio_set_direction(PIN_IN_A, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_IN_A, GPIO_PULLDOWN_ONLY);

    gpio_set_direction(PIN_IN_B, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_IN_B, GPIO_PULLDOWN_ONLY);
    
    gpio_set_direction(PIN_IN_S, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_IN_S, GPIO_PULLDOWN_ONLY);

    gpio_set_direction(PIN_ACK_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_ACK_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_ACK_AGG, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_ACK_C, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_ACK_D, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_ACK_S, GPIO_MODE_OUTPUT);

    gpio_set_direction(PIN_ERROR_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_ERROR_LED, 0);

    gpio_set_level(PIN_ACK_A, 0);
    gpio_set_level(PIN_ACK_B, 0);
    gpio_set_level(PIN_ACK_AGG, 0);
    gpio_set_level(PIN_ACK_C, 0);
    gpio_set_level(PIN_ACK_D, 0);
    gpio_set_level(PIN_ACK_S, 0);

    // For Sporadic interrupt
    gpio_set_intr_type(PIN_IN_S, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_IN_S, in_s_isr, NULL);

    // PCNT setup for A
    pcnt_unit_config_t pcnt_config_A = {
        .low_limit = -32768,
        .high_limit = 32767,
    };
    pcnt_new_unit(&pcnt_config_A, &pcnt_unit_A);

    pcnt_chan_config_t pcnt_chan_config_a =
    {
        .edge_gpio_num = PIN_IN_A,
        .level_gpio_num = -1,
    };

    pcnt_channel_handle_t pcnt_chan_a = NULL;
    pcnt_new_channel(pcnt_unit_A, &pcnt_chan_config_a, &pcnt_chan_a);

    pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD);
    pcnt_unit_enable(pcnt_unit_A);
    pcnt_unit_clear_count(pcnt_unit_A);
    pcnt_unit_start(pcnt_unit_A);

    // PCNT setup for B
    pcnt_unit_config_t pcnt_config_B = {
        .low_limit = -32768,
        .high_limit = 32767,
    };
    pcnt_new_unit(&pcnt_config_B, &pcnt_unit_B);

    pcnt_chan_config_t pcnt_chan_config_b =
    {
        .edge_gpio_num = PIN_IN_B,
        .level_gpio_num = -1,
    };

    pcnt_channel_handle_t pcnt_chan_b = NULL;
    pcnt_new_channel(pcnt_unit_B, &pcnt_chan_config_b, &pcnt_chan_b);

    pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD);
    pcnt_unit_enable(pcnt_unit_B);
    pcnt_unit_clear_count(pcnt_unit_B);
    pcnt_unit_start(pcnt_unit_B);
}  
// ISR for the sporadic input pin IN_S
static void IRAM_ATTR in_s_isr(void *arg)
{
    int64_t t = esp_timer_get_time();

    // simple debounce / min-interarrival guard (30ms)
    if ((t - last_sporadic_us) >= 30000) {
        sporadic_pending_count++;
        last_sporadic_us = t;
        notifySRelease();
    }
}
// checks if sync has been seen
bool pins_take_sporadic_pending(void)
{
    if (sporadic_pending_count > 0) {
        sporadic_pending_count--;
        return true;
    }
    return false;
}
// checks if there is a pending sporadic event
bool pins_has_sporadic_pending(void)
{
    return (sporadic_pending_count > 0);
}
// Checks if sync has been seen and latched
bool pins_sync_seen(){
    if(sync_latched)
    {
        return true;
    }
    int32_t level = gpio_get_level(PIN_SYNC);
    if(last_sync_level == 0 && level == 1){
        sync_latched = true;
        sync_t0 = esp_timer_get_time();
    }
    last_sync_level = level;
    return sync_latched;
}
// Returns the timestamp of the SYNC signal in microseconds
int64_t pins_sync_T0_us(){
    return sync_t0;
}
// Returns the delta of edges seen on pin A since the last call
int64_t pins_edgesA_delta(){
    int countA = 0;
    pcnt_unit_get_count(pcnt_unit_A,&countA);
    int delta_A = countA - last_countA;
    last_countA = countA;
    return delta_A;
}
// Returns the delta of edges seen on pin B since the last call
int64_t pins_edgesB_delta(){
    int countB = 0;
    pcnt_unit_get_count(pcnt_unit_B,&countB);
    int delta_B = countB - last_countB;
    last_countB = countB;
    return delta_B;
}



