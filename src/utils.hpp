#pragma once
#include "defines.hpp"
#include <stdio.h>
#include "esp_spiffs.h"
#include "esp_log.h"
#include <iostream>
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
//#include "protocol_examples_common.h"
#include "esp_err.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "sdkconfig.h"
#include <atomic>

using namespace std;

void step_pulse_init(const uint32_t freq_hz, const ledc_timer_t timer_num, const gpio_num_t ledc_output, const ledc_channel_t channel)
{
    // Prepare and then apply the LEDC PWM timer configuration
    periph_module_enable(PERIPH_LEDC_MODULE);
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .timer_num        = timer_num,
        .freq_hz          = freq_hz,  // set output frequency
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&ledc_timer);
    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .gpio_num   = ledc_output,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel    = channel,
        .intr_type  = LEDC_INTR_FADE_END,
        .timer_sel  = timer_num,
        .duty       = 128, // set duty at about 50%
        .hpoint     = 0,
    };    
    ledc_channel_config(&ledc_channel);
}

////////////////////////BUZZER INIT//////////////////////////
   void beep(uint beeps,uint delay, uint freq){
        step_pulse_init(freq,LEDC_TIMER_0,BUZZER,LEDC_CHANNEL_0);
        for(int i=0; i<beeps; i++){
            ledc_timer_pause(LEDC_HIGH_SPEED_MODE,LEDC_TIMER_0);
            vTaskDelay(delay / portTICK_PERIOD_MS);
            ledc_timer_resume(LEDC_HIGH_SPEED_MODE,LEDC_TIMER_0);
            vTaskDelay(delay / portTICK_PERIOD_MS);
        } 
        ledc_timer_pause(LEDC_HIGH_SPEED_MODE,LEDC_TIMER_0);
   }

/*void silovka(void*pvParameters){
    while(1){
        if(!gpio_get_level(SILOVKA) != driver_stdby){
            if((!driver_stdby) == 1){ 
                printf("Pripojeno 12V:\n");
                beep(1, 1000, 1800);
                esp_restart();
            }
            else{
                printf("Stand by:\n");
                esp_restart();
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }*/

static QueueHandle_t switch_evt_queue;

static void IRAM_ATTR switch_intr_isr(void *arg)
{
    int pinNumber = (int) arg;
    xQueueSendFromISR(switch_evt_queue, &pinNumber, NULL);
}

void switch_control_task(void *param)
{
    int pinNumber, count = 0, curr_count=0;
    while (xQueueReceive(switch_evt_queue, &pinNumber, portMAX_DELAY))
    {
        count++;
        if(count==curr_count+2){
            count=curr_count;
            gpio_set_level(LED, LED_ON_OFF); //Oscar#2 1
            beep(2, 300, 1800);
            esp_restart();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        else{
            printf("GPIO %d was pressed %d times. The state is %d\n", pinNumber, count, gpio_get_level(ON_OFF_SWITCH));
            gpio_set_level(LED, PWR_ON_OFF); //Oscar#2 0
            beep(1, 1000, 1800);
            curr_count=count;
            count++;
            power_on_off=PWR_ON_OFF; //Oscar#2 false
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}    

static QueueHandle_t gpio_evt_queue = NULL;   // A queue to handle pulse counter events

static void IRAM_ATTR gpio_intr_isr(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue,&gpio_num,NULL);
}

void gpio_control_task(void* arg){
    uint32_t io_num;
    while(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)){
        switch(io_num){
            case 19:
            motor_speed[3] = 0;
            printf("motor3 stalled!!!\n");
            stall[3]=true;
            vTaskDelay(portTICK_PERIOD_MS / 1000);
            break;
            case 21:
            motor_speed[2] = 0;    
            printf("motor2 stalled!!!\n");
            stall[2]=true;
            vTaskDelay(portTICK_PERIOD_MS / 1000);
            break;
            case 22:
            motor_speed[1] = 0;    
            printf("motor1 stalled!!!\n");
            vTaskDelay(portTICK_PERIOD_MS / 1000);
            stall[1]=true;
            break;
            case 23:
            motor_speed[0] = 0;    
            printf("motor0 stalled!!!\n");
            stall[0]=true;
            vTaskDelay(portTICK_PERIOD_MS / 1000);
            break;
        }
    }
}

esp_reset_reason_t check_reset() {
    esp_reset_reason_t resetReason = esp_reset_reason();
    switch (resetReason) {
    case ESP_RST_UNKNOWN:
        printf("\tUnknown reset - strange\n");
        break;
    case ESP_RST_POWERON:
        printf("\tPoweron reset\n");
        break;
    case ESP_RST_EXT:
        printf("\tExternal reset\n");
        break;
    case ESP_RST_SW:
        printf("\tSoftware reset\n");
        break;
    case ESP_RST_PANIC:
        printf("\tReset due to core panic - stop program\n");
        vTaskSuspend(nullptr);
        break;
    case ESP_RST_INT_WDT:
        printf("\tReset due to interrupt watchdog - stop program\n");
        vTaskSuspend(nullptr);
        break;
    case ESP_RST_TASK_WDT:
        printf("\tReset due to task watchdog - stop program\n");
        vTaskSuspend(nullptr);
        break;
    case ESP_RST_WDT:
        printf("\tReset due to some watchdog - stop program\n");
        vTaskSuspend(nullptr);
        break;
    case ESP_RST_DEEPSLEEP:
        printf("\tWaked from deep sleep\n");
        break;
    case ESP_RST_BROWNOUT:
        printf("\tBrownout reset - please check power\n");
        break;
    case ESP_RST_SDIO:
        printf("\tSDIO reset - strange\n");
        break;
    }
    return resetReason;
}

class MutexGuard {
    MutexGuard() = delete;
    MutexGuard(const MutexGuard&) = delete;
    void operator = (const MutexGuard&) = delete;
public:
    MutexGuard(SemaphoreHandle_t& mutex)
        : m_mutex {mutex}
    {}
    int acquire (TickType_t timeout = portMAX_DELAY){
        return xSemaphoreTake(m_mutex, timeout);
    }
    int release () {
        return xSemaphoreGive(m_mutex);
    }
    ~MutexGuard() {
        xSemaphoreGive(m_mutex);
    }
private:
    SemaphoreHandle_t& m_mutex;
};
