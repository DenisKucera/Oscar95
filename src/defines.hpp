#pragma once
using namespace std;
#include <atomic>

#define DRIVER_0_ADDRES           0
#define DRIVER_1_ADDRES           1
#define DRIVER_2_ADDRES           2
#define DRIVER_3_ADDRES           3

#define VCC_IO                   GPIO_NUM_32  // L = transistor Q3 off -> motor power off, H = all drivers on

#define ENN_PIN0                   GPIO_NUM_33  
#define ENN_PIN1                   GPIO_NUM_25
#define ENN_PIN2                   GPIO_NUM_26
#define ENN_PIN3                   GPIO_NUM_27

#define SILOVKA                    GPIO_NUM_4
#define BUZZER                     GPIO_NUM_5
#define ON_OFF_SWITCH              GPIO_NUM_14
#define LED                        GPIO_NUM_2

#define KONCOVY_DOJEZD_0          GPIO_NUM_35
#define KONCOVY_DOJEZD_1          GPIO_NUM_34
#define KONCOVY_DOJEZD_2          GPIO_NUM_39
#define KONCOVY_DOJEZD_3          GPIO_NUM_36

#define DIAG_PIN0                 GPIO_NUM_19
#define DIAG_PIN1                 GPIO_NUM_21
#define DIAG_PIN2                 GPIO_NUM_22
#define DIAG_PIN3                 GPIO_NUM_23

#define PCNT_INPUT_0        GPIO_NUM_12 
#define PCNT_INPUT_1        GPIO_NUM_18
#define PCNT_INPUT_2        GPIO_NUM_15
#define PCNT_INPUT_3        GPIO_NUM_13

#define GPIO_BIT_MASK_KONCOVE_DOJEZDY ((1ULL<<KONCOVY_DOJEZD_0) | (1ULL<<KONCOVY_DOJEZD_1) | (1ULL<<KONCOVY_DOJEZD_2) | (1ULL<<KONCOVY_DOJEZD_3))
#define GPIO_BIT_MASK_DIAG_PINS ((1ULL<<DIAG_PIN0) | (1ULL<<DIAG_PIN1) | (1ULL<<DIAG_PIN2) | (1ULL<<DIAG_PIN3))

#define DRIVERS_UART              UART_NUM_1
#define DRIVERS_UART_TXD          GPIO_NUM_17 
#define DRIVERS_UART_RXD          GPIO_NUM_16 
#define DRIVERS_UART_BUF_SIZE     256
#define DRIVERS_RX_TIMEOUT        (20 / portTICK_RATE_MS)
#define DRIVERS_UART_START_BYTE   0x05

#define USB_UART                  UART_NUM_0
#define USB_UART_TXD          GPIO_NUM_1 
#define USB_UART_RXD          GPIO_NUM_3
#define USB_UART_BUF_SIZE     1024
#define USB_RX_TIMEOUT        (20 / portTICK_RATE_MS)
#define USB_UART_START_BYTE   0x05

#define MOTOR_SPEED_COEFICIENT    143216    // 71608 = 1RPS VACTUAL 0x22

// globální proměnné pro pokusy s grafickým rozhraním
volatile int q=0;
bool driver_stdby=0;
std::atomic<int> motor_speed[4];

volatile int speed;
/*std::atomic<int> motor_speed1;
std::atomic<int> motor_speed2;
std::atomic<int> motor_speed3;*/
bool power_on_off;
bool rucni_rizeni=false;

int h_limits[4]={0,0,0,0};
int l_limits[4]={0,0,0,0};

/*int16_t driver0_const=10; //139,149
int16_t driver1_const=250; //558,568
int16_t driver2_const=500; //1003,993
int16_t driver3_const=450; //955,962*/