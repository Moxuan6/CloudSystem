#include "si7006.h"
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
// &i2c1{
//     pinctrl-names   = "default","sleep";
//     pinctrl-0 = <&i2c1_pins_b>; //工作状态管脚复用
//     pinctrl-1 = <&i2c1_sleep_pins_b>;//休眠状态管脚复用
// 	clock-frequency = <400000>; //设置总线速率为400K
//     i2c-scl-rising-time-ns = <185>; //上升沿和下降沿的时间
//     i2c-scl-falling-time-ns = <20>;
//     status = "okay"; //控制器使能

//     si7006@40{
//         compatible = "hi,si7006";
//         reg = <0x40>;
//     };
// };

#define CNAME "si7006"

int major = 0;
struct class* cls;
struct device* dev;
struct i2c_client* gclient;

int i2c_read_tmp_hum(u8 reg)
{
    int ret;
    u8 r_buf[] = { reg };
    u16 data;
    // 1.封装消息
    struct i2c_msg r_msg[] = {
        [0] = {
            .addr = gclient->addr,
            .flags = 0,
            .len = 1,
            .buf = r_buf,
        },
        [1] = {
            .addr = gclient->addr,
            .flags = 1,
            .len = 2,
            .buf = (u8*)&data,
        },
    };
    // 2.发送消息
    ret = i2c_transfer(gclient->adapter, r_msg, 2);
    if (ret != 2) {
        printk("i2c_read_tmp_hum error\n");
        return -EAGAIN;
    }
    return (data >> 8 | data << 8);
}

int si7006_open(struct inode* inode, struct file* filp)
{
    return 0;
}

long si7006_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
    int data = 0, ret;
    switch (cmd) {
    case GET_TMP:
        data = i2c_read_tmp_hum(TMP_ADDR);
        if (data < 0) {
            printk("i2c_read_tmp_hum error\n");
            return -EAGAIN;
        }
        data = data & 0xffff;
        ret = copy_to_user((void*)arg, &data, sizeof(data));
        if (ret) {
            printk("copy_to_user error\n");
            return -EFAULT;
        }
        break;
    case GET_HUM:
        data = i2c_read_tmp_hum(HUM_ADDR);
        if (data < 0) {
            printk("i2c_read_tmp_hum error\n");
            return -EAGAIN;
        }
        data = data & 0xffff;
        ret = copy_to_user((void*)arg, &data, sizeof(data));
        if (ret) {
            printk("copy_to_user error\n");
            return -EFAULT;
        }
        break;
    default:
        break;
    }
    return 0;
}

int si7006_release(struct inode* inode, struct file* filp)
{
    return 0;
}

struct file_operations fops = {
    .open = si7006_open,
    .unlocked_ioctl = si7006_ioctl,
    .release = si7006_release,
};

int si7006_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
    gclient = client;
    printk("%s:%s:%d\n", __FILE__, __func__, __LINE__);

    // 1.注册字符设备驱动
    major = register_chrdev(0, CNAME, &fops);
    if (major < 0) {
        printk("register_chrdev error\n");
        return -EINVAL;
    }
    // 2.创建设备节点
    cls = class_create(THIS_MODULE, CNAME);
    if (IS_ERR(cls)) {
        printk("class_create error\n");
        return PTR_ERR(cls);
    }
    dev = device_create(cls, NULL, MKDEV(major, 0), NULL, CNAME);
    if (IS_ERR(dev)) {
        printk("device_create error\n");
        return PTR_ERR(dev);
    }
    return 0;
}
int si7006_remove(struct i2c_client* client)
{
    printk("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, CNAME);
    return 0;
}
struct of_device_id oftable[] = {
    { .compatible = "hi,si7006" },
    {},
};
struct i2c_driver si7006 = {
    .probe = si7006_probe,
    .remove = si7006_remove,
    .driver = {
        .name = "si7006",
        .of_match_table = oftable,
    },
};

module_i2c_driver(si7006);
MODULE_LICENSE("GPL");