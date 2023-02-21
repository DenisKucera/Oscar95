#pragma once
#include <stdio.h>
#include "esp_spiffs.h"
#include "esp_log.h"
#include <iostream>
#include "defines.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"
#include "esp_attr.h"
#include "soc/gpio_sig_map.h"

#define TAG "spiffs"
using namespace std;

xQueueHandle pcnt_evt_queue;   // A queue to handle pulse counter events
pcnt_isr_handle_t user_isr_handle = NULL;
/* A sample structure to pass events from the PCNT
 * interrupt handler to the main program.
 */
typedef struct {
    int unit;  // the PCNT unit that originated an interrupt
    uint32_t status; // information on the event type that caused the interrupt
} pcnt_evt_t;

/* Decode what PCNT's unit originated an interrupt
 * and pass this information together with the event type
 * the main program using a queue.
 */
static void IRAM_ATTR pcnt_example_intr_handler(void *arg)
{
    uint32_t intr_status = PCNT.int_st.val;
    int i;
    pcnt_evt_t evt;
    portBASE_TYPE HPTaskAwoken = pdFALSE;

    for (i = 0; i < PCNT_UNIT_MAX; i++) {
        if (intr_status & (BIT(i))) {
            evt.unit = i;
            /* Save the PCNT event type that caused an interrupt
               to pass it to the main program */
            evt.status = PCNT.status_unit[i].val;
            PCNT.int_clr.val = BIT(i);
            xQueueSendFromISR(pcnt_evt_queue, &evt, &HPTaskAwoken);
            if (HPTaskAwoken == pdTRUE) {
                portYIELD_FROM_ISR();
            }
        }
    }
}

void iopins_init(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL<<VCC_IO));
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}
//reading spiffs
int * spiffs(void)
{
    esp_vfs_spiffs_conf_t config = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    esp_vfs_spiffs_register(&config);
    
    /* read data from instruction.txt file */
    ESP_LOGE(TAG, "Reading data from file: instruction.txt");
    FILE *file = fopen("/spiffs/instruction.txt", "r");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "File does not exist!");
    }
    else
    {
        int i=0;
        char line[256];
        static int x[256];
        while (fgets(line, sizeof(line), file) != NULL)
        {
            x[i] = int(strtol(line, NULL, 10));
            if(q<i){
                q=i;
            }
            i++;
        }
        x[q+1]=q+1;
        fclose(file);
        return x;
    }
    esp_vfs_spiffs_unregister(NULL);
    return 0;
}

void pcnt_init(const pcnt_unit_t unit, const gpio_num_t pin)
{
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = pin,
        .ctrl_gpio_num = -1,
        .lctrl_mode = PCNT_MODE_DISABLE,
        .hctrl_mode = PCNT_MODE_DISABLE,
        .pos_mode = PCNT_COUNT_DIS,
        .neg_mode = PCNT_COUNT_DIS,
        .counter_h_lim = 0,
        .counter_l_lim = 0,
        .unit = unit,
        .channel = PCNT_CHANNEL_0,
    };

    /* Initialize PCNT unit */
    
    pcnt_unit_config(&pcnt_config);

    /*pcnt_set_event_value(unit, PCNT_EVT_THRES_1, 100);
    pcnt_event_enable(unit, PCNT_EVT_THRES_1);*/

    pcnt_event_enable(unit, PCNT_EVT_H_LIM);
    pcnt_event_enable(unit, PCNT_EVT_L_LIM);

    /* Configure and enable the input filter */
    /*pcnt_set_filter_value(unit, 1000);
    pcnt_filter_enable(unit);*/

    /* Initialize PCNT's counter */
    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);

    pcnt_isr_register(pcnt_example_intr_handler,NULL,0,&user_isr_handle);//pcnt_get_event_value()
    pcnt_intr_enable(unit);

    /* Everything is set up, now go to counting */
    pcnt_counter_resume(unit);
}

void pulse(void*pvParameters){
     /* Initialize PCNT event queue and PCNT functions */
    pcnt_evt_queue = xQueueCreate(10, sizeof(pcnt_evt_t));

    int16_t count[4]={0,0,0,0};
    pcnt_evt_t evt;
    //portBASE_TYPE res;

    while (1) {
        /* Wait for the event information passed from PCNT's interrupt handler.
         * Once received, decode the event type and print it on the serial monitor.
         */ 
        if (xQueueReceive(pcnt_evt_queue, &evt, portMAX_DELAY)) {
            pcnt_get_counter_value(static_cast<pcnt_unit_t>(evt.unit), &count[evt.unit]);
            printf("Event PCNT unit[%d] count: %d\n", evt.unit,count[evt.unit]);
            if (evt.status & PCNT_STATUS_H_LIM_M) {
                h_limits[evt.unit]=h_limits[evt.unit]+1;
                printf("H_LIM\n");
            }
            if (evt.status & PCNT_STATUS_L_LIM_M) {
                l_limits[evt.unit]=l_limits[evt.unit]+1;
                printf("L_LIM\n");
            }
        } 
    }
       if(user_isr_handle) {
        //Free the ISR service handle.
        esp_intr_free(user_isr_handle);
        user_isr_handle = NULL;
    }
}

void nvs_init()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was trun_0cated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
}
