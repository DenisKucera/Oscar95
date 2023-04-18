#define GRIDUI_LAYOUT_DEFINITION
#include "layout.hpp"     //  pro grafické rozhraní
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "defines.hpp"
#include "utils.hpp"
#include "init.hpp"
#include "uart.hpp"
#include "driver.hpp"
#include "gridui.h"         //  pro grafické rozhraní          
#include "rbprotocol.h"     //  pro grafické rozhraní
#include "rbwebserver.h"    //  pro grafické rozhraní
#include "rbwifi.h"         //  pro grafické rozhraní
#include "driver/ledc.h"
#include "soc/gpio_sig_map.h"
#include "soc/pcnt_reg.h"
#include "soc/pcnt_struct.h"
#include "driver/periph_ctrl.h"
#include "soc/rtc_wdt.h"
#include "parser.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <atomic>

using namespace rb;
using namespace std;

static void initGridUi(){
    using namespace gridui;
    // Initialize WiFi
    WiFi::startAp(WIFI,"");     //esp vytvoří wifi sít  //OSCAR
    // WiFi::connect("Jmeno wifi", "Heslo");    //připojení do místní sítě
    
    // Initialize RBProtocol
    auto *protocol = new Protocol("Student", "Oscar95", "Compiled at " __DATE__ " " __TIME__, [](const std::string& cmd, rbjson::Object* pkt) {
        UI.handleRbPacket(cmd, pkt);
    });
    protocol->start();
    // Start serving the web page
    rb_web_start(80);
    // Initialize the UI builder
    UI.begin(protocol);
    // Build the UI widgets. Positions/props are set in the layout, so most of the time,
    // you should only set the event handlers here.
    auto builder = gridui::Layout.begin();
    
    builder.RucniRizeni.onPress([](Button &){
        rucni_rizeni=true;
     });
    builder.KlesteMinus.onPress([](Button &){
        if(rucni_rizeni){motor_speed[0]=speed;}
    });
    builder.KlesteMinus.onRelease([](Button &){
        if(rucni_rizeni){motor_speed[0]=0;}
    });
    builder.KlestePlus.onPress([](Button &){
        if(rucni_rizeni){motor_speed[0]=-speed;}
    });
    builder.KlestePlus.onRelease([](Button &){
        if(rucni_rizeni){motor_speed[0]=0;}
    });
    builder.PodstavaMinus.onPress([](Button &){
        if(rucni_rizeni){motor_speed[2]=-speed;}
    });
    builder.PodstavaMinus.onRelease([](Button &){
         if(rucni_rizeni){motor_speed[2]=0;}
    });
    builder.PodstavaPlus.onPress([](Button &){
        if(rucni_rizeni){motor_speed[2]=speed;}
    });
    builder.PodstavaPlus.onRelease([](Button &){
        if(rucni_rizeni){motor_speed[2]=0;}
    });
    builder.LoketMinus.onPress([](Button &){
        if(rucni_rizeni){motor_speed[1]=-speed;}
    });
    builder.LoketMinus.onRelease([](Button &){
         if(rucni_rizeni){motor_speed[1]=0;}
    });
    builder.LoketPlus.onPress([](Button &){
        if(rucni_rizeni){motor_speed[1]=speed;}
    });
    builder.LoketPlus.onRelease([](Button &){
        if(rucni_rizeni){motor_speed[1]=0;}
    });
    builder.RamenoMinus.onPress([](Button &){

        if(rucni_rizeni){motor_speed[3]=-speed;}
    });
    builder.RamenoMinus.onRelease([](Button &){
         if(rucni_rizeni){motor_speed[3]=0;}
    });
    builder.RamenoPlus.onPress([](Button &){
        if(rucni_rizeni){motor_speed[3]=speed;}
    });
    builder.RamenoPlus.onRelease([](Button &){
        if(rucni_rizeni){motor_speed[3]=0;}
    });
    builder.Slider1.onChanged([](Slider &s) {
        speed = int(MOTOR_SPEED_COEFICIENT * (s.value()/100));
        printf("motor_speed: %f -> %d\n", s.value(), speed);   
    });
    builder.Reset.onPress([](Button &){
        esp_restart();
    });
    /*builder.MotorSpeed3.onChanged([](Slider &s) {
        motor_speed[3] = int(MOTOR_SPEED_COEFICIENT * s.value());
        printf("motor_speed3: %f -> %d\n", s.value(), motor_speed[3].load());   
    }); */

    // Commit the layout. Beyond this point, calling any builder methods on the UI is invalid.
    builder.commit();
}

   static void initDriver(Driver& driver, const int iRun, const int iHold) {
    driver.init();
    vTaskDelay(100 / portTICK_PERIOD_MS);   
    uint32_t data =0;

    int result = driver.get_PWMCONF(data);
    if (result != 0)
        printf("PWMCONF driveru %d : ERROR  %d\n", driver.address(), result);
    else
        printf("PWMCONF driveru %d =  %08X\n", driver.address(), data);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    result = driver.get_DRV_STATUS(data);
    if (result != 0)
        printf("DRV_STATUS driveru %d : ERROR  %d\n", driver.address(), result);
    else
        printf("DRV_STATUS driveru %d =  %08X\n", driver.address(), data);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    result = driver.read_gconf(data);
    if (result != 0)
        printf("GCONF %d : ERROR  %d\n", driver.address(), result);
    else
        printf("GCONF %d =  %08X\n", driver.address(), data);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    result = driver.get_CHOPCONF(data);
    if (result != 0)
        printf("CHOPCONF %d : ERROR  %d\n", driver.address(), result);
    else
        printf("CHOPCONF %d =  %08X\n", driver.address(), data);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    result = driver.get_GSTAT(data);
    if (result != 0)
        printf("GSTAT %d : ERROR  %d\n", driver.address(), result);
    else
        printf("GSTAT %d =  %08X\n", driver.address(), data);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    driver.set_speed(0);                      // otáčení motoru se nastavuje zápisem rychlosti do driveru přes Uart
    driver.set_IHOLD_IRUN (16, 32);             // proud IHOLD (při stání) =8/32, IRUN (při běhu)= 8/32 (8/32 je minimum, 16/32 je maximum pro dluhodobější provoz) 
    if(!gpio_get_level(SILOVKA)){
        driver.enable();
        //driver_stdby = 1;
        printf("\n");
        printf("Zapnuti driveru!\n");
        printf("\n");
    }
    else{
        driver.disable();
        printf("\n");
        printf("Napajeni nepripojeno!\n");
        printf("\n");
    }                
    vTaskDelay(300 / portTICK_PERIOD_MS);     //doba stání pro nastavení automatiky driveru
    driver.set_IHOLD_IRUN (iRun, iHold);             //proud IHOLD =0, IRUN = 8/32 (při stání je motor volně otočný)
   }

   bool garbo(){
    if (LED_ON_OFF == 1) {
        return !power_on_off;
    } else {
        return power_on_off;
    }
   }

extern "C" void app_main(void){ 

    check_reset();
    iopins_init();

    gpio_install_isr_service(0);

    gpio_config_t switch_conf = {
        .pin_bit_mask = ((1ULL<<ON_OFF_SWITCH) | (1ULL<<SILOVKA)),   //inicializuje switch 
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&switch_conf);

    gpio_config_t buzzer_conf = {
        .pin_bit_mask = ((1ULL<<BUZZER) | (1ULL<<LED)),   //inicializuje pin buzzer 
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&buzzer_conf);

    switch_evt_queue = xQueueCreate(1, sizeof(int));
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    xTaskCreate(switch_control_task, "switch_Control_Task", 2048, NULL,tskIDLE_PRIORITY, NULL);

    gpio_isr_handler_add(ON_OFF_SWITCH,switch_intr_isr,(void*) ON_OFF_SWITCH);

    /*driver_stdby = !gpio_get_level(SILOVKA);
    if(driver_stdby==0){
        beep(2, 500, 1800);
    }*/

    gpio_set_level(LED,LED_ON_OFF); //1 pro Oscar#2
    power_on_off=gpio_get_level(LED);
    bool once=true;
    while(garbo()){ //bez (!)
        while(once){
        printf("Oscar95 OFF state\n");
        printf("press button to start!\n");
        gpio_set_level(VCC_IO, 0);
        once=false;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Oskar95 start \n\tbuild %s %s\n", __DATE__, __TIME__);
    gpio_set_level(VCC_IO, 1);
    // zapnuti siloveho napajeni do driveru*/

    gpio_config_t opto_pin_conf = {
        .pin_bit_mask = GPIO_BIT_MASK_KONCOVE_DOJEZDY,   //inicializuje piny 35,34,36,39 (optozávory) + silovka
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&opto_pin_conf);

    gpio_config_t diag_pin_conf = {};
        diag_pin_conf.pin_bit_mask = GPIO_BIT_MASK_DIAG_PINS;  //inicializuje piny 19,21,22,23 (stallguard)
        diag_pin_conf.mode = GPIO_MODE_INPUT_OUTPUT;
        diag_pin_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        diag_pin_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        diag_pin_conf.intr_type = GPIO_INTR_POSEDGE;
    gpio_config(&diag_pin_conf);

    if(!gpio_get_level(SILOVKA)){
        diag_pin_conf.mode = GPIO_MODE_INPUT;  
        diag_pin_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; 
        gpio_config(&diag_pin_conf);
        gpio_set_level(VCC_IO, 0);              // reset driveru
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(VCC_IO, 1); 
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    nvs_init();                             //inicializace pro zápis do flash paměti
    initGridUi();

    Uart drivers_uart {
        DRIVERS_UART,
        Uart::config_t {
            .baud_rate = 750000,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 122,
            .use_ref_tick = false
        },
        Uart::pins_t {
            .pin_txd = DRIVERS_UART_TXD,
            .pin_rxd = DRIVERS_UART_RXD,
            .pin_rts = UART_PIN_NO_CHANGE,
            .pin_cts = UART_PIN_NO_CHANGE
        },
        Uart::buffers_t {
            .rx_buffer_size = DRIVERS_UART_BUF_SIZE,
            .tx_buffer_size = 0,
            .event_queue_size = 0
        }
    };

    pcnt_init(PCNT_UNIT_0,PCNT_INPUT_0);
    pcnt_init(PCNT_UNIT_1,PCNT_INPUT_1);
    pcnt_init(PCNT_UNIT_2,PCNT_INPUT_2);
    pcnt_init(PCNT_UNIT_3,PCNT_INPUT_3);

    printf("\n");
    Driver driver0 { drivers_uart, DRIVER_0_ADDRES, ENN_PIN0, PCNT_UNIT_0};
    initDriver(driver0, 31, 0);
    printf("\n");
    Driver driver1 { drivers_uart, DRIVER_1_ADDRES, ENN_PIN1, PCNT_UNIT_1};
    initDriver(driver1, 31, 0); 
    printf("\n");
    Driver driver2 { drivers_uart, DRIVER_2_ADDRES, ENN_PIN2, PCNT_UNIT_2};
    initDriver(driver2, 31, 0);
    printf("\n");
    Driver driver3 { drivers_uart, DRIVER_3_ADDRES, ENN_PIN3, PCNT_UNIT_3};
    initDriver(driver3, 31, 0);
    printf("\n");

    //TaskHandle_t silovka_init;
    //xTaskCreatePinnedToCore(silovka,"silovka check",4096,NULL,tskIDLE_PRIORITY,&silovka_init,1);

    xTaskCreate(gpio_control_task, "Intr_Control_Task", 2048, NULL, tskIDLE_PRIORITY, NULL);

    gpio_isr_handler_add(DIAG_PIN0, gpio_intr_isr, (void*) DIAG_PIN0);
    gpio_isr_handler_add(DIAG_PIN1, gpio_intr_isr, (void*) DIAG_PIN1);
    gpio_isr_handler_add(DIAG_PIN2, gpio_intr_isr, (void*) DIAG_PIN2);
    gpio_isr_handler_add(DIAG_PIN3, gpio_intr_isr, (void*) DIAG_PIN3);

    TaskHandle_t pulse_count;
    xTaskCreatePinnedToCore(pulse,"pulse counter",4096,NULL,tskIDLE_PRIORITY,&pulse_count,1);

    xTaskCreate(g_code_parser,"parsing task",4096, NULL, tskIDLE_PRIORITY,NULL);
    /*pcnt_set_event_value(PCNT_UNIT_0,PCNT_EVT_H_LIM,10);
    pcnt_set_event_value(PCNT_UNIT_1,PCNT_EVT_H_LIM,10);
    pcnt_set_event_value(PCNT_UNIT_2,PCNT_EVT_H_LIM,10);
    pcnt_set_event_value(PCNT_UNIT_3,PCNT_EVT_H_LIM,10);

    pcnt_set_event_value(PCNT_UNIT_0,PCNT_EVT_L_LIM,-10);
    pcnt_set_event_value(PCNT_UNIT_1,PCNT_EVT_L_LIM,-10);
    pcnt_set_event_value(PCNT_UNIT_2,PCNT_EVT_L_LIM,-10);
    pcnt_set_event_value(PCNT_UNIT_3,PCNT_EVT_L_LIM,-10);*/

    /*pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_1);
    pcnt_counter_clear(PCNT_UNIT_2);
    pcnt_counter_clear(PCNT_UNIT_3);*/

    /*uint32_t SG0=0;
    uint32_t SG1=0;
    uint32_t SG2=0;
    uint32_t SG3=0;*/
    /*for(int i=0; i<4;i++){
        motor_speed[i]=-17902;
    }*/

    driver0.set_SGTHRS(40);
    driver1.set_SGTHRS(10);
    driver2.set_SGTHRS(3);
    driver3.set_SGTHRS(15);

    driver0.set_TCOOLTHRS(0xFFFFF);
    driver1.set_TCOOLTHRS(0xFFFFF);
    driver2.set_TCOOLTHRS(0xFFFFF);
    driver3.set_TCOOLTHRS(0xFFFFF);
    
    //driver2.set_speed(motor_speed2);

    /*while(gpio_get_level(SILOVKA)){

       if(h_limits[2]>x){
        driver2.set_speed(-motor_speed2);
        x=h_limits[2];
       }
       else if(l_limits[2]>y){
        driver2.set_speed(motor_speed2);
        y=l_limits[2];
       }
        if(h_limits[2]==1){
            //driver2.set_speed(speed);
            pcnt_set_event_value(PCNT_UNIT_2,PCNT_EVT_H_LIM,100);
        }
        if(h_limits[2]==2){
           pcnt_set_event_value(PCNT_UNIT_2,PCNT_EVT_H_LIM,100); 
        }
        if(h_limits[2]==3){
           pcnt_set_event_value(PCNT_UNIT_2,PCNT_EVT_H_LIM,100); 
        }
        if(h_limits[2]==4){
           pcnt_set_event_value(PCNT_UNIT_2,PCNT_EVT_H_LIM,100); 
        }
        if(h_limits[2]==5
        ){
            driver2.set_speed(0);
        }
        
       printf("%d\n",h_limits[2]);
        
        driver0.get_SG(SG0);
        driver1.get_SG(SG1);
        driver2.get_SG(SG2);
        driver3.get_SG(SG3);
        printf("SG0: %d  SG1: %d  SG2: %d  SG3: %d\n",SG0,SG1,SG2,SG3);
        
        printf("konec dojezd2: %d\n",gpio_get_level(KONCOVY_DOJEZD_2));

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }*/
    //gpio_set_level(ON_OFF_SWITCH,1);

    
    //printf("%d\n", gpio_get_level(SILOVKA));
    //motor_homing();

    printf("ready\n");
    while(!gpio_get_level(SILOVKA)){
        
        driver0.set_speed(MOTOR0/*motor_speed[0]*/); //0
        driver1.set_speed(MOTOR1/*motor_speed[1]*/); //1
        driver2.set_speed(MOTOR2/*motor_speed[2]*/); //2
        driver3.set_speed(MOTOR3/*motor_speed[3]*/); //3
        
        vTaskDelay(10/portTICK_PERIOD_MS);
    } 
    /*while(1){
        printf("opto0: %d\n",gpio_get_level(static_cast<gpio_num_t>(35)));
        printf("opto1: %d\n",gpio_get_level(static_cast<gpio_num_t>(34)));
        printf("opto2: %d\n",gpio_get_level(static_cast<gpio_num_t>(39)));
        printf("opto3: %d\n",gpio_get_level(static_cast<gpio_num_t>(36)));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    int *val;
    val = spiffs();

    */
}
    