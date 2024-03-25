#include <linux/device.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define CNAME "ap3216"

struct i2c_client* gclient;
int major;
struct class* cls;
struct device* dev;
wait_queue_head_t wq_head;
unsigned int condition = 0;
struct work_struct work;

int i2c_write_reg(u8 reg, u8 data)
{
    int ret;
    u8 w_buf[] = { reg, data };
    // 1.封装消息结构体
    struct i2c_msg w_msg = {
        .addr = gclient->addr,
        .flags = 0,
        .len = 2,
        .buf = w_buf,
    };
    // 2.发送消息
    ret = i2c_transfer(gclient->adapter, &w_msg, 1);
    if (ret != 1) {
        printk("i2c_write_reg error\n");
        return -EAGAIN;
    }

    return 0;
}

int i2c_read_reg(u8 reg)
{
    int ret;
    u8 data;
    // 1.封装消息结构体
    struct i2c_msg r_msg[] = {
        [0] = {
            .addr = gclient->addr,
            .flags = 0,
            .len = 1,
            .buf = &reg,
        },
        [1] = {
            .addr = gclient->addr,
            .flags = 1,
            .len = 1,
            .buf = &data,
        },
    };
    // 2.发送消息
    ret = i2c_transfer(gclient->adapter, r_msg, 2);
    if (ret != 2) {
        printk("i2c_read_reg error\n");
        return -EAGAIN;
    }

    return data;
}

int ap3216_open(struct inode* inode, struct file* filp)
{
    return 0;
}

ssize_t ap3216_read(struct file* filp, char __user* ubuf,
    size_t size, loff_t* offset)
{
    int ret;
    u8 als_data_low, als_data_high;
    u16 als_data;
    

    // 3.读取环境光数据
    als_data_low = i2c_read_reg(0x0C);
    als_data_high = i2c_read_reg(0x0D);
    als_data = (als_data_high << 8) | als_data_low;

    // 4.将数据从内核空间拷贝到用户空间
    if (size > sizeof(als_data))
        size = sizeof(als_data);
    ret = copy_to_user(ubuf, &als_data, size);
    if(ret){
        printk("copy_to_user error\n");
        return -EAGAIN;
    }

    return size;
}

int ap3216_close(struct inode* inode, struct file* filp)
{
    return 0;
}

// 定义file_operations结构体
struct file_operations fops = {
    .open = ap3216_open,
    .read = ap3216_read,
    .release = ap3216_close,
};

// 通过i2c_driver结构体来注册i2c驱动
int ap3216_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
    // 保存i2c_client结构体指针
    gclient = client;
    printk("%s:%s:%d\n", __FILE__, __func__, __LINE__);
   
    // 1.ap3216初始化
    i2c_write_reg(0x00, 0x04); // 软件复位
    mdelay(150); // 延时150ms
    i2c_write_reg(0x00, 0x03); // 开启ALS,PS,IR

    // 设置ALS Gain为范围4
    // Range 4 (B5B4=’11’): 0 ~ 323 Lux. Resolution = 0.0049 lux/count
    i2c_write_reg(0x10, 0x11);

    // 2.注册字符设备驱动
    major = register_chrdev(0, CNAME, &fops);
    if (major < 0) {
        printk("register_chrdev error\n");
        return -EAGAIN;
    }
    
    // 3.创建类和设备节点
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

// 移除i2c驱动
int ap3216_remove(struct i2c_client* client)
{
    printk("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, CNAME);
    i2c_write_reg(0x00, 0x0); // power down
    return 0;
}

// 通过of_device_id结构体来匹配设备树中的compatible属性
struct of_device_id oftable[] = {
    {
        .compatible = "hi,ap3216",
    },
    {},
};

// 支持热插拔
MODULE_DEVICE_TABLE(of, oftable);

// 通过i2c_driver结构体来注册i2c驱动
struct i2c_driver ap3216 = {
    .probe = ap3216_probe,
    .remove = ap3216_remove,
    .driver = {
        .name = "ap3216",
        .of_match_table = oftable,
    },
};

module_i2c_driver(ap3216);
MODULE_LICENSE("GPL");