/******************************************************************************
 * @file     app_main.c
 * @brief    红外发射例程
 * @version  V1.0
 * @date     2020.6.12
 ******************************************************************************/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <app_init.h>
#include "dw_gpio.h"
#include "pin_name.h"
#include "pinmux.h"
#include "pins.h"
#include <pm.h>
#include <ARMCM0.h>

#include <devices/devicelist.h>
#include <devices/device.h>
#include <devices/driver.h>

// #include "drv_pwm.h"

#include "drv_gpio.h"//GPIO头文件
#include "drv_timer.h"//定时器头文件
#include "pwm.h"//pwm头文件,修改版

#include "ir.h"

#define THIS_PWM_CH     1           /*range 0~5 PWM_CH0~PWM_CH5*/
#define THIS_IR_PWM     PWM1
#define THIS_IR_GPIO    GPIO_P06



int app_main(void)
{
    
    board_yoc_init();
    disableSleepInPM(1);//关闭低功耗模式，禁止休眠

    IR_Ready();
    IR_Config_PWM(THIS_PWM_CH,38000,4,1);
    IR_Config_GPIO(THIS_IR_GPIO, THIS_IR_PWM);

    IR_Send(0x01,0x80);
    IR_Close();


    return 0;
}