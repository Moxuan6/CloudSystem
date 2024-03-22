#include "led.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
// myled {
//     compatible = "hi,led";
//     core_leds{
//         led1 = <&gpioz 5 0>;
//         led2 = <&gpioz 6 0>;
//         led3 = <&gpioz 7 0>;
//     };
//     extend_leds{
//         led1 = <&gpioe 10 0>;
//         led2 = <&gpiof 10 0>;
//         led3 = <&gpioe 8 0>;
//     };
// };
#define CNAME "myled"
int major;
struct device_node *pnode, *cnode, *enode;
int cgpiono[3], egpiono[3];
const char* name[] = { "led1", "led2", "led3" };
struct class* cls;
struct device* dev;
int get_led_node(void)
{
    pnode = of_find_compatible_node(NULL, NULL, "hi,led");
    if (pnode == NULL) {
        printk("of_find_compatible_node error\n");
        return -ENODATA;
    }
    cnode = of_get_child_by_name(pnode, "core_leds");
    if (cnode == NULL) {
        printk("of_get_child_by_name cnode error\n");
        return -ENODATA;
    }
    enode = of_get_child_by_name(pnode, "extend_leds");
    if (enode == NULL) {
        printk("of_get_child_by_name enode error\n");
        return -ENODATA;
    }
    return 0;
}
int myleds_init_value(struct device_node* node, int* no)
{
    int i, ret;
    for (i = 0; i < ARRAY_SIZE(name); i++) {
        // 1.获取gpio号
        no[i] = of_get_named_gpio(node, name[i], 0);
        if (no[i] < 0) {
            printk("of_get_named_gpio error\n");
            ret = no[i];
            goto err1;
        }
        // 2.申请要使用的gpio
        ret = gpio_request(no[i], NULL);
        if (ret) {
            printk("gpio_request error\n");
            goto err1;
        }
        // 3.设置方向，设置值0
        ret = gpio_direction_output(no[i], 0);
        if (ret) {
            printk("gpio_direction_output error\n");
            goto err1;
        }
    }

    return 0; /******************************/

err1:
    for (--i; i >= 0; i--) {
        gpio_free(no[i]);
    }
    return ret;
}
void myleds_uninit_value(int* no)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(name); i++) {
        gpio_free(no[i]);
    }
}
int myleds_open(struct inode* inode, struct file* file)
{
    printk("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    return 0;
}
void led_set_value(int which, int status)
{
    switch (which) {
    case CLED1:
        status ? gpio_set_value(cgpiono[0], 1) : gpio_set_value(cgpiono[0], 0);
        break;
    case CLED2:
        status ? gpio_set_value(cgpiono[1], 1) : gpio_set_value(cgpiono[1], 0);
        break;
    case CLED3:
        status ? gpio_set_value(cgpiono[2], 1) : gpio_set_value(cgpiono[2], 0);
        break;
    case ELED4:
        status ? gpio_set_value(egpiono[0], 1) : gpio_set_value(egpiono[0], 0);
        break;
    case ELED5:
        status ? gpio_set_value(egpiono[1], 1) : gpio_set_value(egpiono[1], 0);
        break;
    case ELED6:
        status ? gpio_set_value(egpiono[2], 1) : gpio_set_value(egpiono[2], 0);
        break;
    }
}
long myleds_ioctl(struct file* file,
    unsigned int cmd, unsigned long arg)
{
    int which, ret;
    switch (cmd) {
    case LED_ON:
        ret = copy_from_user(&which, (void*)arg, _IOC_SIZE(cmd));
        if (ret) {
            printk("copy_from_user error\n");
            return -EIO;
        }
        led_set_value(which, ON);
        break;
    case LED_OFF:
        ret = copy_from_user(&which, (void*)arg, _IOC_SIZE(cmd));
        if (ret) {
            printk("copy_from_user error\n");
            return -EIO;
        }
        led_set_value(which, OFF);
        break;
    }
    return 0;
}
int myled_close(struct inode* inode, struct file* file)
{
    printk("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    return 0;
}
struct file_operations fops = {
    .open = myleds_open,
    .unlocked_ioctl = myleds_ioctl,
    .release = myled_close,
};
static int __init myleds_init(void)
{
    int ret;
    // 1.获取设备树节点
    if ((ret = get_led_node()))
        goto err1;
    // 2.对led灯做初始化工作
    if ((ret = myleds_init_value(cnode, cgpiono)))
        goto err1;
    if ((ret = myleds_init_value(enode, egpiono)))
        goto err2;
    // 3.注册字符设备驱动
    major = register_chrdev(0, CNAME, &fops);
    if (major < 0) {
        printk("register_chrdev error");
        ret = major;
        goto err3;
    }
    // 4.自动创建设备节点
    cls = class_create(THIS_MODULE, CNAME);
    if (IS_ERR(cls)) {
        printk("class_create error\n");
        ret = PTR_ERR(cls);
        goto err4;
    }
    dev = device_create(cls, NULL, MKDEV(major, 0), NULL, CNAME);
    if (IS_ERR(dev)) {
        printk("device_create error\n");
        ret = PTR_ERR(dev);
        goto err5;
    }
    return 0;
err5:
    class_destroy(cls);
err4:
    unregister_chrdev(major, CNAME);
err3:
    myleds_uninit_value(egpiono);
err2:
    myleds_uninit_value(cgpiono);
err1:
    return ret;
}
static void __exit myleds_exit(void)
{
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, CNAME);
    myleds_uninit_value(egpiono);
    myleds_uninit_value(cgpiono);
}
module_init(myleds_init);
module_exit(myleds_exit);
MODULE_LICENSE("GPL");