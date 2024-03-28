#include "motor.h"

static int major = 0;
char kbuf[128] = {0};
static struct class *motor_class;
unsigned int *rcc_4, *rcc_2, *gpiof_moder, *gpiof_afrl;
unsigned int *tim16_psc, *tim16_arr, *tim16_ccr1, *tim16_ccmr1;
unsigned int *tim16_ccer, *tim16_cr1, *tim16_bdtr;


void motor_start(void)
{
    // 1.motor的初始化
    *rcc_4 |= (0x1 << 5); // GPIOF
    
    // 设置PF6为复用功能
    *gpiof_moder &= ~(0x3 << 12);
    *gpiof_moder |= (0x2 << 12);

    // 设置PF6为TIM16_CH1复用功能
    *gpiof_afrl &= ~(0xF << 24);
    *gpiof_afrl |= (0x1 << 24);

    // 使能TIM16时钟
    *rcc_2 |= (0x1 << 3);

    // 设置TIM16的预分频值
    *tim16_psc = 209 - 1;

    // 设置PWM方波的最终的频率
    *tim16_arr = 1000;

    // 设置PWM的占空比
    *tim16_ccr1 = 700;

    // 设置PWM模式
    *tim16_ccmr1 &= ~((0x1 << 16) | (0x7 << 4));
    *tim16_ccmr1 |= (0x6 << 4);

    // 设置TIM16_CH1通道使能TIMx_CCR1预加载使能寄存器
    *tim16_ccmr1 |= (0x1 << 3);

    // 设置TIM16_CH1的OC1通道配置为输出
    *tim16_ccmr1 &= ~(0x3 << 0);

    // 设置TIM16_CH1通道输出PWM方波的极性
    *tim16_ccer &= ~(0x1 << 1);

    // 设置TIM16_CH1通道的输出使能位，通过GPIO引脚输出PWM方波
    *tim16_ccer |= (0x1 << 0);

    // 设置TIM16_CH1通道的TIMx_ARR预装载寄存器的缓冲区的使能
    *tim16_cr1 |= (0x1 << 7);

    // 配置TIM16_CH1通道为主输出使能
    *tim16_bdtr |= (0x1 << 15);

    // 使能TIM16计数器
    *tim16_cr1 |= (0x1 << 0);
}

void motor_stop(void)
{
    // 关闭TIM16计数器
    *tim16_cr1 &= ~(0x1 << 0);

    // 关闭TIM16_CH1通道为主输出
    *tim16_bdtr &= ~(0x1 << 15);

    // 关闭TIM16_CH1通道的TIMx_ARR预装载寄存器的缓冲区
    *tim16_cr1 &= ~(0x1 << 7);

    // 关闭TIM16_CH1通道的输出
    *tim16_ccer &= ~(0x1 << 0);

    // 关闭TIM16_CH1通道输出PWM方波的极性
    *tim16_ccer |= (0x1 << 1);

    // 关闭TIM16_CH1的OC1通道配置为输出
    *tim16_ccmr1 |= (0x3 << 0);

    // 关闭TIM16_CH1通道使能TIMx_CCR1预加载使能寄存器
    *tim16_ccmr1 &= ~(0x1 << 3);

    // 关闭PWM模式
    *tim16_ccmr1 |= ((0x1 << 16) | (0x7 << 4));

    // 关闭PWM的占空比
    *tim16_ccr1 = 0;

    // 关闭PWM方波的最终的频率
    *tim16_arr = 0;

    // 关闭TIM16的预分频值
    *tim16_psc = 0;

    // 关闭TIM16时钟
    *rcc_2 &= ~(0x1 << 3);

    // 关闭PF6为TIM16_CH1复用功能
    *gpiof_afrl |= (0xF << 24);

    // 关闭PF6为复用功能
    *gpiof_moder &= ~(0x2 << 12);
    *gpiof_moder |= (0x3 << 12);

    // 关闭GPIOF
    *rcc_4 &= ~(0x1 << 5);

}


int motor_open(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t motor_read(struct file* file,
    char __user* ubuf, size_t size, loff_t* offs)
{
    int ret;
    if (size > sizeof(kbuf))
        size = sizeof(kbuf);
    ret = copy_to_user(ubuf, kbuf, size);
    if (ret) {
        printk("copy_to_user error\n");
        return -EIO;
    }
    
    return size;
}

ssize_t motor_write(struct file* file,
    const char __user* ubuf, size_t size, loff_t* offs)
{
    int ret;
    if (size > sizeof(kbuf))
        size = sizeof(kbuf);
    ret = copy_from_user(kbuf, ubuf, size);
    if (ret) {
        printk("copy_from_user error\n");
        return -EIO;
    }

    if (kbuf[0] == '1') {
        motor_start();  // 启动电机
    } else if (kbuf[0] == '0') {
        motor_stop();   // 关闭电机
    }

    return size;
}

int motor_close(struct inode *inode, struct file *filp)
{
    return 0;
}

const struct file_operations motor_fops = {
    .owner = THIS_MODULE,
    .open = motor_open,
    .read = motor_read,
    .write = motor_write,
    .release = motor_close,
};

static int __init motor_init(void)
{
    // 1.注册字符设备驱动
    major = register_chrdev(0, CNAME, &motor_fops);
    if (major < 0) {
        printk("register_chrdev error\n");
        return -EIO;
    }

    // 2.创建类
    motor_class = class_create(THIS_MODULE, CNAME);
    if (IS_ERR(motor_class)) {
        printk("class_create error\n");
        unregister_chrdev(major, CNAME);
        return PTR_ERR(motor_class);
    }

    // 3.创建设备
    device_create(motor_class, NULL, MKDEV(major, 0), NULL, CNAME);

        // 1.地址映射
    rcc_4 = ioremap(RCC_MP_AHB4ENSETR, 4);
    if(rcc_4 == NULL){
        printk("ioremap rcc_4 failed\n");
        return -ENOMEM;
    }
    rcc_2 = ioremap(RCC_MP_APB2ENSETR, 4);
    if(rcc_2 == NULL){
        printk("ioremap rcc_2 failed\n");
        return -ENOMEM;
    }
    gpiof_moder = ioremap(GPIOF_MODER, 4);
    if(gpiof_moder == NULL){
        printk("ioremap gpiof_moder failed\n");
        return -ENOMEM;
    }
    gpiof_afrl = ioremap(GPIOF_AFRL, 4);
    if(gpiof_afrl == NULL){
        printk("ioremap gpiof_afrl failed\n");
        return -ENOMEM;
    }
    tim16_psc = ioremap(TIM16_PSC, 4);
    if(tim16_psc == NULL){
        printk("ioremap tim16_psc failed\n");
        return -ENOMEM;
    }
    tim16_arr = ioremap(TIM16_ARR, 4);
    if(tim16_arr == NULL){
        printk("ioremap tim16_arr failed\n");
        return -ENOMEM;
    }
    tim16_ccr1 = ioremap(TIM16_CCR1, 4);
    if(tim16_ccr1 == NULL){
        printk("ioremap tim16_ccr1 failed\n");
        return -ENOMEM;
    }
    tim16_ccmr1 = ioremap(TIM16_CCMR1, 4);
    if(tim16_ccmr1 == NULL){
        printk("ioremap tim16_ccmr1 failed\n");
        return -ENOMEM;
    }
    tim16_ccer = ioremap(TIM16_CCER, 4);
    if(tim16_ccer == NULL){
        printk("ioremap tim16_ccer failed\n");
        return -ENOMEM;
    }
    tim16_cr1 = ioremap(TIM16_CR1, 4);
    if(tim16_cr1 == NULL){
        printk("ioremap tim16_cr1 failed\n");
        return -ENOMEM;
    }
    tim16_bdtr = ioremap(TIM16_BDTR, 4);
    if(tim16_bdtr == NULL){
        printk("ioremap tim16_bdtr failed\n");
        return -ENOMEM;
    }

    return 0;
}

static void __exit motor_exit(void)
{
    // 关闭电机
    motor_stop();

    // 释放地址映射
    iounmap(rcc_4);
    iounmap(rcc_2);
    iounmap(gpiof_moder);
    iounmap(gpiof_afrl);
    iounmap(tim16_psc);
    iounmap(tim16_arr);
    iounmap(tim16_ccr1);
    iounmap(tim16_ccmr1);
    iounmap(tim16_ccer);
    iounmap(tim16_cr1);
    iounmap(tim16_bdtr);

    // 1.销毁设备
    device_destroy(motor_class, MKDEV(major, 0));
    // 2.销毁类
    class_destroy(motor_class);
    // 3.注销字符设备驱动
    unregister_chrdev(major, CNAME);


}

module_init(motor_init);
module_exit(motor_exit);
MODULE_LICENSE("GPL");

