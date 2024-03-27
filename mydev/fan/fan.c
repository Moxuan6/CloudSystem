#include "fan.h"

static int major = 0;
char kbuf[128] = {0};
static struct class *motor_class;
unsigned int *rcc_4, *rcc_2, *gpioe_moder, *gpioe_afrh;
unsigned int *tim1_psc, *tim1_arr, *tim1_ccr1, *tim1_ccmr1; 
unsigned int *tim1_ccer, *tim1_cr1, *tim1_bdtr;

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
    int duty_cycle;
    if (size > sizeof(kbuf))
        size = sizeof(kbuf);
    ret = copy_from_user(kbuf, ubuf, size);
    if (ret) {
        printk("copy_from_user error\n");
        return -EIO;
    }

    // 将用户输入的数据解析为一个整数
    ret = kstrtoint(kbuf, 10, &duty_cycle);
    if (ret) {
        printk("kstrtoint error\n");
        return ret;
    }

     // 根据输入的整数设置不同的占空比
    switch (duty_cycle) {
    case 0:
        *tim1_ccr1 = 0; // 0档位
        break;
    case 1:
        *tim1_ccr1 = 300; // 1档位
        break;
    case 2:
        *tim1_ccr1 = 500; // 2档位
        break;
    case 3:
        *tim1_ccr1 = 800; // 3档位
        break;
    default:
        printk("Invalid duty cycle\n");
        return -EINVAL;
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
        printk("ioremap rcc4 failed\n");
        return -ENOMEM;
    }
    rcc_2 = ioremap(RCC_MP_APB2ENSETR, 4);
    if(rcc_2 == NULL){
        printk("ioremap rcc2 failed\n");
        return -ENOMEM;
    }
    gpioe_moder = ioremap(GPIOE_MODER, 4);
    if(gpioe_moder == NULL){
        printk("ioremap gpioe_moder failed\n");
        return -ENOMEM;
    }
    gpioe_afrh = ioremap(GPIOE_AFRH, 4);
    if(gpioe_afrh == NULL){
        printk("ioremap gpioe_afrh failed\n");
        return -ENOMEM;
    }
    tim1_psc = ioremap(TIM1_PSC, 4);
    if(tim1_psc == NULL){
        printk("ioremap tim1_psc failed\n");
        return -ENOMEM;
    }
    tim1_arr = ioremap(TIM1_ARR, 4);
    if(tim1_arr == NULL){
        printk("ioremap tim1_arr failed\n");
        return -ENOMEM;
    }
    tim1_ccr1 = ioremap(TIM1_CCR1, 4);
    if(tim1_ccr1 == NULL){
        printk("ioremap tim1_ccr1 failed\n");
        return -ENOMEM;
    }
    tim1_ccmr1 = ioremap(TIM1_CCMR1, 4);
    if(tim1_ccmr1 == NULL){
        printk("ioremap tim1_ccmr1 failed\n");
        return -ENOMEM;
    }
    tim1_ccer = ioremap(TIM1_CCER, 4);
    if(tim1_ccer == NULL){
        printk("ioremap tim1_ccer failed\n");
        return -ENOMEM;
    }
    tim1_cr1 = ioremap(TIM1_CR1, 4);
    if(tim1_cr1 == NULL){
        printk("ioremap tim1_cr1 failed\n");
        return -ENOMEM;
    }
    tim1_bdtr = ioremap(TIM1_BDTR, 4);
    if(tim1_bdtr == NULL){
        printk("ioremap tim1_bdtr failed\n");
        return -ENOMEM;
    }

    // 2.motor的初始化
    *rcc_4 |= (0x1 << 4); // GPIOE
    
    // 设置PE9为复用功能
    *gpioe_moder &= ~(0x3 << 18);
    *gpioe_moder |= (0x2 << 18);

    // 设置PF9为TIM1_CH1复用功能
    *gpioe_afrh &= ~(0xF << 4);
    *gpioe_afrh |= (0x1 << 4);

    // 使能TIM16时钟
    *rcc_2 |= (0x1 << 0);

    // 设置TIM1的预分频值
    *tim1_psc = 209 - 1;

    // 设置PWM方波的最终的频率
    *tim1_arr = 1000;

    // 设置PWM的占空比
    *tim1_ccr1 = 0;

    // 设置PWM模式
    *tim1_ccmr1 &= ~((0x1 << 16) | (0x7 << 4));
    *tim1_ccmr1 |= (0x6 << 4);

    // 设置TIM1_CH1通道使能TIMx_CCR1预加载使能寄存器
    *tim1_ccmr1 |= (0x1 << 3);

    // 设置TIM1_CH1的OC1通道配置为输出
    *tim1_ccmr1 &= ~(0x3 << 0);

    // 设置TIM1_CH1通道输出PWM方波的极性
    *tim1_ccer &= ~(0x1 << 1);

    // 设置TIM1_CH1通道的输出使能位，通过GPIO引脚输出PWM方波
    *tim1_ccer |= (0x1 << 0);

    // 设置TIM1_CH1通道的TIMx_ARR预装载寄存器的缓冲区的使能
    *tim1_cr1 |= (0x1 << 7);

    // 设置定时器的计数方式，边沿对齐模式
    *tim1_cr1 &= ~(0x3 << 5);

    // 设置定时器计数的方向，采用递减计数/递增计数
    *tim1_cr1 &= ~(0x1 << 4);

    // 配置TIM1_CH1通道为主输出使能
    *tim1_bdtr |= (0x1 << 15);

    // 使能TIM1计数器
    *tim1_cr1 |= (0x1 << 0);

    return 0;
}

static void __exit motor_exit(void)
{
    //还原寄存器的设置
    // 重置GPIOE
    *rcc_4 &= ~(0x1 << 4);

    // 重置PE9为输入模式
    *gpioe_moder &= ~(0x3 << 18);

    // 重置PF9为GPIO功能
    *gpioe_afrh &= ~(0xF << 4);

    // 禁用TIM16时钟
    *rcc_2 &= ~(0x1 << 0);

    // 重置TIM1的预分频值
    *tim1_psc = 0;

    // 重置PWM方波的频率
    *tim1_arr = 0;

    // 重置PWM的占空比
    *tim1_ccr1 = 0;

    // 重置PWM模式
    *tim1_ccmr1 &= ~((0x1 << 16) | (0x7 << 4));

    // 禁用TIM1_CH1通道的预加载使能寄存器
    *tim1_ccmr1 &= ~(0x1 << 3);

    // 重置TIM1_CH1的OC1通道配置为输入
    *tim1_ccmr1 |= (0x3 << 0);

    // 重置TIM1_CH1通道的极性
    *tim1_ccer |= (0x1 << 1);

    // 禁用TIM1_CH1通道的输出
    *tim1_ccer &= ~(0x1 << 0);

    // 禁用TIM1_CH1通道的预装载寄存器的缓冲区
    *tim1_cr1 &= ~(0x1 << 7);

    // 重置定时器的计数方式为向上计数模式
    *tim1_cr1 |= (0x3 << 5);

    // 重置定时器计数的方向为递增计数
    *tim1_cr1 |= (0x1 << 4);

    // 禁用TIM1_CH1通道的主输出
    *tim1_bdtr &= ~(0x1 << 15);

    // 禁用TIM1计数器
    *tim1_cr1 &= ~(0x1 << 0);

    // 取消地址映射
    iounmap(rcc_4);
    iounmap(rcc_2);
    iounmap(gpioe_moder);
    iounmap(gpioe_afrh);
    iounmap(tim1_psc);
    iounmap(tim1_arr);
    iounmap(tim1_ccr1);
    iounmap(tim1_ccmr1);
    iounmap(tim1_ccer);
    iounmap(tim1_cr1);
    iounmap(tim1_bdtr);

    // 销毁设备
    device_destroy(motor_class, MKDEV(major, 0));
    // 销毁类
    class_destroy(motor_class);
    // 注销字符设备驱动
    unregister_chrdev(major, CNAME);
}

module_init(motor_init);
module_exit(motor_exit);
MODULE_LICENSE("GPL");

