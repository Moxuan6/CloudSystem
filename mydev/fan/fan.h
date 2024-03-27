#ifndef __MOTOR_H__
#define __MOTOR_H__

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define RCC_MP_AHB4ENSETR   0x50000A28
#define GPIOE_MODER         0x50006000
#define GPIOE_AFRH          0x50006024
#define RCC_MP_APB2ENSETR   0x50000A08
#define TIM1_PSC            0x44000028
#define TIM1_ARR            0x4400002C
#define TIM1_CCR1           0x44000034
#define TIM1_CCMR1          0x44000018
#define TIM1_CCER           0x44000020
#define TIM1_CR1            0x44000000
#define TIM1_BDTR           0x44000044

#define CNAME "fan"

#endif // __MOTOR_H__