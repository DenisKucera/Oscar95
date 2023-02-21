#pragma once

#include <cmath>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <atomic>
#include "defines.hpp"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "freertos/portmacro.h"
#include "driver/pcnt.h"
#include "uart.hpp"

Uart usb_uart {
        USB_UART,
        Uart::config_t {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 122,
            //.use_ref_tick = false
            .source_clk = UART_SCLK_APB,
        },
        Uart::pins_t {
            .pin_txd = USB_UART_TXD,
            .pin_rxd = USB_UART_RXD,
            .pin_rts = UART_PIN_NO_CHANGE,
            .pin_cts = UART_PIN_NO_CHANGE
        },
        Uart::buffers_t {
            .rx_buffer_size = USB_UART_BUF_SIZE,
            .tx_buffer_size = 0,
            .event_queue_size = 0
        }
    };

using namespace std;
////////////////////////ZASTAVENI, SPUSTENI MOTORU///////////////////////////
bool motor_pause_resume_delay(uint delay,bool state){
    int motor[4];
    for(int i=0; i<4; i++){
        motor[i]=motor_speed[i];
    }
    while(!state && delay==0){
        for(int i=0; i<4; i++){
            motor_speed[i]=0;
        }
        break;
    }
    while(!state && (delay>0)){
        for(int i=0; i<4; i++){
            motor_speed[i]=0;
        }
        vTaskDelay(delay / portTICK_PERIOD_MS);
        state = true;
    }
    while(state){
        for(int i=0; i<4; i++){
            motor_speed[i]=motor[i];
        }
        break;
    }
    return state;
}
//////////////////////////HOMING MOTORU/////////////////////
bool motor_homing(void){
    int DOJEZD[4] = {34,35,36,39};
    bool homing = false;
    int x=4;
    int count=0;
    while(!homing){

        for(int i = 0; i < 4; i++){
            if(gpio_get_level(static_cast<gpio_num_t>(DOJEZD[i]))){
                motor_speed[i] = 0;
                if(i!=x){
                    x=i;
                    count++;
                }
                else{
                    break;
                }
            }
        }
        if(count==4){
            homing = true;
            //break;
        }
    }
    return homing;
}
/////////////////////PARSOVANI HODNOT PRO MOTORY 0,1,2,3///////////////////////
int xyzw_parsing(int i, char array[]){
        int pos=0;
        int dir=1;
                if(array[i+1]=='-'){
                        dir=-1;
                    }
            
                for(int e=1; e<6; e++){
                    if(array[i+e]==' ' && (dir == -1)){
                        switch(e){
                            case 3:
                                pos=(array[i+(e-1)]-48);
                            break;
                            case 4:
                                pos=((array[i+(e-1)]-48)+((array[i+(e-2)]-48)*10));
                            break;
                            case 5:
                                pos=((array[i+(e-1)]-48)+((array[i+(e-2)]-48)*10)+((array[i+(e-3)]-48)*100));
                            break;
                            default:
                                //printf("chyba pri zadavani\n");
                            break;
                        }
                        pos = pos * dir;
                        dir = 0;
                    }
                    if(array[i+e]==' ' && (dir == 1)){
                        switch(e){
                            case 2:
                                pos=(array[i+(e-1)]-48);
                            break;
                            case 3:
                                pos=((array[i+(e-1)]-48)+((array[i+(e-2)]-48)*10));
                            break;
                            case 4:
                                pos=((array[i+(e-1)]-48)+((array[i+(e-2)]-48)*10)+((array[i+(e-3)]-48)*100));
                            break;
                            default:
                                //printf("chyba pri zadavani\n");
                            break;
                        }    
                        pos = pos * dir;
                        dir = 0;
                    }
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
       return pos;         
   }
////////////////////////////SPEED PARSING////////////////////////
int speed_parsing(int i, char array[]){
                        float speed_const=14.3216;
                        int speed = 0;
                        int return_speed;
                        for(int e=1; e<7; e++){
                        if(array[i+e]=='\n'){
                            switch(e){
                            case 2:
                                speed=(array[i+(e-1)]-48);
                            break;
                            case 3:
                                speed=((array[i+(e-1)]-48)+((array[i+(e-2)]-48)*10));
                            break;
                            case 4:
                                speed=((array[i+(e-1)]-48)+((array[i+(e-2)]-48)*10)+((array[i+(e-3)]-48)*100));
                            break;
                            case 5:
                                speed=((array[i+(e-1)]-48)+((array[i+(e-2)]-48)*10)+((array[i+(e-3)]-48)*100)+((array[i+(e-4)]-48)*1000));
                            break;
                            case 6:
                                speed=((array[i+(e-1)]-48)+((array[i+(e-2)]-48)*10)+((array[i+(e-3)]-48)*100)+((array[i+(e-4)]-48)*1000)+((array[i+(e-5)]-48)*10000));
                            break;          
                            default:
                                //printf("chyba pri zadavani\n");
                            break;      
                        }
                        return_speed = round(speed*speed_const);
                        //printf("spd %d\n",return_speed);
                    }  
                    vTaskDelay(portTICK_PERIOD_MS / 100);  
                }
    return return_speed;
}             
////////////////////////TASK G CODE PARSERU/////////////////////
void g_code_parser(void* param){
    bool init_gcode=false;
    bool speed_done=false;
    int direction[4];
    while(1){
        char g_code[usb_uart.available()];
        int max=usb_uart.available();
        if(usb_uart.available()){
            while(usb_uart.available()){
                char cr[2];

                int res = usb_uart.read();

                if(res > 9){
                    sprintf(cr, "%c", res);
                    strncat(g_code, cr, 1);
                }
            }
            if(g_code[0]=='%' && init_gcode==false){
                printf("init gcode\n");
                init_gcode=true;
            }
            else if(g_code[0]=='%' && init_gcode==true){
                printf("konec gcodu\n");
                init_gcode=false;
            }
            else if(init_gcode){
                for(int i=0; i<max; i++){
                    switch (g_code[i])
                    {
                    case 'G':{
                        printf("Nasel G\n");
                        int predcisli=-1;
                            for(int e=1; e<4; e++){
                                if((g_code[i+e]==' ') && (e==3)){
                                    predcisli=(g_code[i+(e-1)] - 48) + ((g_code[i+(e-2)]-48)*10);
                                break;
                            }
                            else if(g_code[i+e]==' ' && (e<3)){
                                printf("chyba pri zadavani\n");
                                predcisli=-1;
                                break;
                            }
                        }
                            switch(predcisli){
                                case -1:
                                    printf("rezim nevybran\n");
                                    break;
                                case 0:
                                    printf("00\n");
                                    break;
                                case 1:
                                    printf("01\n");
                                    break;
                                case 2:
                                    printf("02\n");
                                    break;   
                            }
                            break;
                        }  
                    case 'X':{
                        printf("nasel X\n");
                        int16_t posX=xyzw_parsing(i, g_code);
                        if(posX<0){
                            pcnt_set_event_value(PCNT_UNIT_1,PCNT_EVT_L_LIM,posX);
                            direction[1]=-1;
                        }
                        else if(posX>0){
                            pcnt_set_event_value(PCNT_UNIT_1,PCNT_EVT_H_LIM,posX);
                            direction[1]=1;
                        }
                        else{
                            direction[1]=0;
                        }
                        printf("posX: %d\n",posX);
                        break;}    
                    case 'Y':{
                        printf("nasel Y\n");
                        int16_t posY=xyzw_parsing(i, g_code);
                        if(posY<0){
                            pcnt_set_event_value(PCNT_UNIT_2,PCNT_EVT_L_LIM,posY);
                            direction[2]=-1;
                        }
                        else if(posY>0){
                            pcnt_set_event_value(PCNT_UNIT_2,PCNT_EVT_H_LIM,posY);
                            direction[2]=1;
                        }
                        else{
                            direction[2]=0;
                        }
                        printf("posY: %d\n",posY);
                        break;}
                    case 'Z':{
                        printf("nasel Z\n");
                        int16_t posZ=xyzw_parsing(i, g_code);
                        if(posZ<0){
                            pcnt_set_event_value(PCNT_UNIT_3,PCNT_EVT_L_LIM,posZ);
                            direction[3] = -1;
                        }
                        else if(posZ>0){
                            pcnt_set_event_value(PCNT_UNIT_3,PCNT_EVT_H_LIM,posZ);
                            direction[3] = 1;
                        }
                        else{
                            direction[3]=0;
                        }
                        printf("posZ: %d\n",posZ);
                        break;  }
                    case 'W':{
                        printf("nasel W\n");
                        int16_t posW=xyzw_parsing(i, g_code);
                        if(posW<0){
                            pcnt_set_event_value(PCNT_UNIT_0,PCNT_EVT_L_LIM,posW);
                            direction[0] = -1;
                        }
                        else if(posW>0){
                            pcnt_set_event_value(PCNT_UNIT_0,PCNT_EVT_H_LIM,posW);
                            direction[0] = 1;
                        }
                        else{
                            direction[0]=0;
                        }
                        printf("posW: %d\n",posW);
                        break; }          
                    case 'F':{
                        printf("Nasel F\n");
                        int speed=speed_parsing(i, g_code);
                        //printf("speed %d\n",speed);
                        for(int q=0; q<4; q++){
                            motor_speed[q]=(direction[q]*speed);
                            printf("set speed %d\n",motor_speed[q].load());
                            //printf("speed %d\n",speed);
                            speed_done=true;
                            vTaskDelay(portTICK_PERIOD_MS /50);
                        }
                        break;}  
                    case '%':{
                        printf("konec g-codu\n");
                        init_gcode=false;
                        break;}       
                }
            }
            printf("jsem tady\n");
            int temp[8];
            int count=0;
                for(int q=0; q<4; q++){
                    temp[q]=h_limits[q];
                    temp[q+1]=l_limits[q];
                }
                //bool motors_done[4]={0,0,0,0};
                while(speed_done){
                   for(int q=0; q<4; q++){
                        if(temp[q]<h_limits[q] || temp[q+1]<l_limits[q] || direction[q]==0){
                            /*if(!set[q]){
                                switch(q){
                                    case 0:{
                                        set[q]=true;
                                        motor_speed[q]=0;
                                        count++;   
                                    break;
                                    }
                                    case 1:{
                                        set[q]=true;
                                        motor_speed[q]=0;
                                        count++;   
                                    break;
                                    }
                                    case 2:{
                                        set[q]=true;
                                        motor_speed[q]=0;
                                        count++;   
                                    break;
                                    }
                                    case 3:{
                                        set[q]=true;
                                        motor_speed[q]=0;
                                        count++;   
                                    break;
                                    }
                                } 
                        }*/
                         vTaskDelay(portTICK_PERIOD_MS / 10);
                   } 
                   if(count==4){speed_done=false;}
                   vTaskDelay(portTICK_PERIOD_MS / 100);
                }
            }
            printf(g_code);
            printf("\n");

            /*printf("%d", int(strlen(msg)));
            printf("\n");*/
            
    }


     vTaskDelay(500/portTICK_PERIOD_MS);   
    }
}
   /*vector<int> reading_gcode_stream(void){
        std::ifstream gcode_file ("gcode.txt");
        //gcode_file.open("gcode.txt");
        //std::string stream;
        std::vector<int> stream{};
        if(gcode_file.is_open()){
            while(gcode_file){
                stream.push_back(gcode_file.get());
                //std::getline(file,stream);
                //read[i]=int(strtol(stream, NULL, 10));
            }
            //gcode_file.close("gcode.txt");
        }
        return stream;
   }*/
   /*bool init_done = false;
        bool read_done = false;

while(!init_done){
        char init_g_code[2]={0,0};
        if(usb_uart.available()){
        while(usb_uart.available()){
                char cr[2];
                int res = usb_uart.read();

                if(res > 31){
                    sprintf(cr, "%c",res);
                    strncat(init_g_code, cr, 1);
                }
            }
    
        switch(init_g_code[0]){
            case '%':
                init_done=true; 
                printf("init\n"); 
                break;
            default:
                printf("not good\n");
                printf(init_g_code);
                printf("\n");
                printf("%d", int(strlen(init_g_code)));
                printf("\n");
                break;
            }   
        }
while(init_done && !read_done){ 
    char g_code[64];
    printf("nacitam g-code\n");
    for(int i=0; i<64; i++){
        char g_code[i]={0};
    }
        if(usb_uart.available()){
        while(usb_uart.available()){
                char cr[2];
                int res = usb_uart.read();

                if(res > 31){
                    sprintf(cr, "%c", res);
                    strncat(g_code, cr, 1);
                }
            }
        int count=0;
        int motor_pos[4];
        read_done=true;  
        int temp[8];
    for(int i=0; i<4; i++){
        temp[i]=h_limits[i];
        temp[i+1]=l_limits[i];
    }  
    for(int i=0; i<64; i++){
        switch(g_code[i]){
            case 'G':{
                int predcisli;
                for(int e=1; i<4; i++){
                    if(g_code[i+e]==' ' && e==3){
                        predcisli=g_code[i+(e-1)]*10+g_code[i+(e-2)];
                        break;
                    }
                    else if(g_code[i+e]==' ' && (e<3)){
                        printf("chyba pri zadavani\n");
                        predcisli=-1;
                        break;
                    }
                }
                switch(predcisli){
                    case -1:
                        printf("rezim nevybrán\n");
                        break;
                    case 0:
                        //00
                        break;
                    case 1:
                        //01
                        break;
                    case 2:
                        //02
                        break;
                    default:
                        break;
                }
            break;
            }
            case 'X':{
                int pos1=xyzw_parsing(i, g_code);
                if(pos1<0){
                    pcnt_set_event_value(PCNT_UNIT_0,PCNT_EVT_L_LIM,pos1);
                }
                else if(pos1>0){
                    pcnt_set_event_value(PCNT_UNIT_2,PCNT_EVT_H_LIM,pos1);
                }
                else{
                    count++;
                }
                motor_pos[1]=pos1;
            break;
            }
            case 'Y':{
                int pos2=xyzw_parsing(i, g_code);
                if(pos2<0){
                    pcnt_set_event_value(PCNT_UNIT_2,PCNT_EVT_L_LIM,pos2);
                }
                else if(pos2>0){
                    pcnt_set_event_value(PCNT_UNIT_2,PCNT_EVT_H_LIM,pos2);
                }
                else{
                    count++;
                }
                motor_pos[2]=pos2;
            break;
            }
            case 'Z':{
                int pos3=xyzw_parsing(i, g_code);
                if(pos3<0){
                    pcnt_set_event_value(PCNT_UNIT_3,PCNT_EVT_L_LIM,pos3);
                }
                else if(pos3>0){
                    pcnt_set_event_value(PCNT_UNIT_3,PCNT_EVT_H_LIM,pos3);
                }
                else{
                    count++;
                }
                motor_pos[3]=pos3;
            break;
            }
            case 'W':{
                int pos0=xyzw_parsing(i, g_code);
                if(pos0<0){
                    pcnt_set_event_value(PCNT_UNIT_0,PCNT_EVT_L_LIM,pos0);
                }
                else if(pos0>0){
                    pcnt_set_event_value(PCNT_UNIT_0,PCNT_EVT_H_LIM,pos0);
                }
                else{
                    count++;
                }
                motor_pos[0]=pos0;
            break;    
            }
            case 'F':{
                float speed=143.216;
                for(int e=1; i<6; i++){
                    if(g_code[i+e]==' '){
                        switch(e){
                            case 2:
                                speed=speed*(g_code[i+(e-1)]);
                                break;
                            case 3:
                                speed=speed*(g_code[i+(e-1)]+g_code[i+(e-2)]*10);
                                break;
                            case 4:
                                speed=speed*(g_code[i+(e-1)]+g_code[i+(e-2)]*10+g_code[i+(e-3)]*100);
                                break;
                            case 5:
                                speed=speed*(g_code[i+(e-1)]+g_code[i+(e-2)]*10+g_code[i+(e-3)]*100+g_code[i+(e-4)]*1000);
                                break;  
                            default:
                                printf("chyba pri zadavani\n");
                            break;      
                        }
                    }    
                }
                for(int i=0; i<4; i++){
                    if(motor_pos[i]<0){
                        motor_speed[i]=round(speed)*(-1);
                    }
                    else if(motor_pos[i]>0){
                        motor_speed[i]=round(speed);
                    }
                    else{
                        motor_speed[i]=0;
                    }
                }
            break;
            }
            case '%':{
                init_done=false;
                printf("konec g_codu\n");
            }
            default:
                break;
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
        while(read_done){
            if(count!=4){
                for(int i=0; i<4; i++){
                    if(temp[i]<h_limits[i] || temp[i+1]<l_limits[i]){
                        count++;
                    }
                }
            }
            else if(count==4){
                    read_done=false;
                    printf("motory nastaveny\n");
                    break;
                }
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    }  
    printf("jsem dole\n");*/
   

