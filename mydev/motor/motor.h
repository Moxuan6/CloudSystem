#ifndef __MOTOR_H__
#define __MOTOR_H__

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define RCC_MP_AHB4ENSETR   0x50000A28
#define GPIOF_MODER         0x50007000
#define GPIOF_AFRL          0x50007020
#define RCC_MP_APB2ENSETR   0x50000A08
#define TIM16_PSC           0x44007028
#define TIM16_ARR           0x4400702C
#define TIM16_CCR1          0x44007034
#define TIM16_CCMR1         0x44007018
#define TIM16_CCER          0x44007020
#define TIM16_CR1           0x44007000
#define TIM16_BDTR          0x44007044

#define CNAME "motor"

#endif // __MOTOR_H__