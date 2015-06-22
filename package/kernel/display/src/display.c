#include <linux/module.h>
#include <linux/version.h>
#include <linux/kmod.h>

#include <linux/kobject.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/sysfs.h>

#include <linux/reboot.h>
#include <linux/netdevice.h>

#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/time.h>


#define DRV_NAME	"display"
#define DRV_DESC	"7 segment display driver"
#define DRV_VERSION	"0.0.1"

#undef DISP_DEBUG
//#define DISP_DEBUG

#ifdef DISP_DEBUG
#define DISP_DBG(fmt, args...) printk(KERN_DEBUG "%s: " fmt, DRV_NAME, ##args )
#else
#define DISP_DBG(fmt, args...) do {} while (0)
#endif

#define DISP_ERR(fmt, args...) printk(KERN_ERR "%s: " fmt, DRV_NAME, ##args )


#define DELAY 50

struct clock_timer {
	struct timer_list timer;
	struct timeval now;
	struct tm tm_val;
	int phase;
	int activated;
	unsigned int data;
};


struct display_platform_data {
	struct gpio_desc *mosi;
	struct gpio_desc *cs;
	struct gpio_desc *sck;

	struct clock_timer clock;


};


const static unsigned char digits_font_table[] = { 0xfc, 0x60, 0xda, 0xf2, 0x66, 0xb6, 0xbe, 0xe0, 0xfe, 0xf6};

static struct display_platform_data *display_data;
static struct kobject *display_kobj;



static void display_init_gpio(void)
{
	gpiod_direction_input(display_data->mosi);
	gpiod_direction_output(display_data->sck, 0);
	gpiod_direction_output(display_data->cs, 1);
}

static void display_send_data(unsigned int data)
{
	int i, j;
	unsigned char digit[4];

	for(j = 0; j < 4; j++) {
		digit[j] = data & 0xFF;
		data >>= 8;
	}

	gpiod_set_value(display_data->cs, 0);
	ndelay(DELAY);

	for (j = 3; j >= 0; j--) {

		gpiod_direction_output(display_data->mosi, digit[j] & 0x01);
		digit[j] >>= 1;
		for (i = 0; i < 8; i++) {
			ndelay(DELAY);
			gpiod_set_value(display_data->sck, 1);
			ndelay(DELAY);
			gpiod_set_value(display_data->sck, 0);
			if (i != 7) {
				gpiod_direction_output(display_data->mosi, digit[j] & 0x01);
				digit[j] >>= 1;
			}
		}
		ndelay(DELAY);
	}

	ndelay(DELAY);
	gpiod_direction_input(display_data->mosi);
	gpiod_set_value(display_data->cs, 1);
}

static void display_reset(unsigned int value)
{
	if(value)
		display_send_data(0xFFFFFFFF);
	else
		display_send_data(0);
}

static unsigned int str_float_to_code(char *value)
{
	int has_sign;
	int has_point;
	int digit;
	unsigned int data;

	data = 0;
	digit = 0;
	has_sign = 0;
	has_point = 0;


	while(*value != 0 && digit != 4) {
		switch(*value) {
		case '-':
			if(!has_sign && !digit) {
				has_sign++;
				digit++;
				data |= 0x02 << (4 - digit) * 8;
			} else {
				return -EINVAL;
			}
			break;

		case '.':
			if(!has_point) {
				has_point++;
				if(!digit || (has_sign == digit)) {
					digit++;
					data |= (digits_font_table[0] | 0x01) << (4 - digit) * 8;
				} else {
					data |= 0x01 << (4 - digit) * 8;
				}
			} else {
				return -EINVAL;
			}
			break;
		case 10:
			break;

		default:
			if(*value >= '0' && *value <= '9') {
				digit++;
				data |= (digits_font_table[*value - 0x30]) << (4 - digit) * 8;
			} else {
				return -EINVAL;
			}


		}
		value++;
	}
	data &= ~(0x00000001);
	data >>= 8 * (4 - digit);
	return data;
}
static unsigned int integer_to_code(int value)
{
	unsigned int data;
	unsigned char digit;
	int i;
	int has_digit;

	has_digit = 0;
	data = 0;

	if (value < 0) {
		value = 0 - value;

		for (i = 2; i >= 0; i--) {
			digit = value / 100;
			value *= 10;
			value %= 1000;
			if(digit) {
				if(!has_digit)
					data |= 0x02 << (i+1) * 8;
				has_digit = 1;
			} else {
				if(!has_digit && i)
					continue;
			}
			data |= digits_font_table[digit] << i*8;
		}
	} else {
		for (i = 3; i >= 0; i--) {
			digit = value / 1000;
			value *= 10;
			value %= 10000;
			if(digit){
				has_digit = 1;
			} else {
				if(!has_digit && i)
					continue;
			}
			data |= digits_font_table[digit] << i*8;
		}
	}

	return data;
}

static ssize_t display_print_float(char *value)
{
	unsigned int ret;

	ret = str_float_to_code(value);
	if(ret != -EINVAL) {
		display_send_data(ret);
		return 0;
	} else {
		return -EINVAL;
	}

}

static void display_print_integer(int value)
{
	display_send_data(integer_to_code(value));
}

static void display_clock(unsigned long data)
{

	mod_timer(&display_data->clock.timer, jiffies + msecs_to_jiffies(500));
	if(!display_data->clock.phase){

		/* Get current time */
		do_gettimeofday(&display_data->clock.now);
		time_to_tm(display_data->clock.now.tv_sec, 0, &display_data->clock.tm_val);

		display_data->clock.data = 0x00010000 | integer_to_code(display_data->clock.tm_val.tm_hour * 100 +  display_data->clock.tm_val.tm_min);
		display_data->clock.phase++;
	} else {
		display_data->clock.data &= ~(0x00010000);
		display_data->clock.phase = 0;
	}
	display_send_data(display_data->clock.data);
}

#ifdef CONFIG_OF
static struct display_platform_data *
display_parse_dt(struct device *dev)
{
	struct device_node *node;
	struct display_platform_data *pdata;
	int mosi, cs, sck;

	node = dev->of_node;
	if(!node) {
		dev_err(dev, "device lacks DT data\n");
		return ERR_PTR(-ENODEV);
	}

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if(!pdata) {
		dev_err(dev, "could not allocate memory for platform data\n");
		return ERR_PTR(-ENOMEM);
	}

	mosi = of_get_named_gpio(node, "display,mosi", 0);
	if(!gpio_is_valid(mosi)) {
		dev_err(dev, "invalid gpio for mosi %d\n", mosi);
		return ERR_PTR(-EINVAL);
	}
	pdata->mosi = gpio_to_desc(mosi);

	cs = of_get_named_gpio(node, "display,cs", 0);
	if(!gpio_is_valid(cs)) {
		dev_err(dev, "invalid gpio for cs %d\n", cs);
		return ERR_PTR(-EINVAL);
	}
	pdata->cs = gpio_to_desc(cs);

	sck = of_get_named_gpio(node, "display,sck", 0);
	if(!gpio_is_valid(sck)) {
		dev_err(dev, "invalid gpio for sck %d\n", sck);
		return ERR_PTR(-EINVAL);
	}
	pdata->sck = gpio_to_desc(sck);

	return pdata;

}

#else
static inline struct display_platform_data *
display_parse_dt(struct device *dev) {
	dev_err(dev, "no platform data defined\n");
	return ERR_PTR(-EINVAL);
}
#endif

#ifdef CONFIG_OF
static struct of_device_id display_of_match[] = {
	{	.compatible = "display",},
	{},
};
MODULE_DEVICE_TABLE(of, display_of_match);
#endif





static ssize_t reset_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int value;
	sscanf(buf, "%du", &value);
	display_reset(value);
	return count;
}

static ssize_t print_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	long value;
	int err;
	char *temp;

	temp = (char *)buf;

	while(*temp != 0) {
		if(*temp == '.') {
			/* Assume float */
			err = display_print_float((char *)buf);
			if(err)
				return -EINVAL;
			return count;
		}
		temp++;
	}

	err = kstrtol(buf, 0, &value);
	if(err)
		return -EINVAL;

	if(value > 9999 || value < -999)
		return -EINVAL;

	display_print_integer(value);
	return count;
}

static ssize_t print_raw_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if(err)
		return -EINVAL;

	display_send_data(value);
	return count;
}

static ssize_t clock_enabled_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	int err;

	err = kstrtoul(buf, 0, &value);
	if(err || (value != 0 && value != 1))
		return -EINVAL;

	/* Enable timer... */
	if(value) {
		if(!display_data->clock.activated) {
			display_data->clock.activated = 1;
			display_data->clock.phase = 0;
			setup_timer(&display_data->clock.timer, display_clock, 0);
			mod_timer(&display_data->clock.timer, jiffies + msecs_to_jiffies(500));
		}
	} else {
		if(display_data->clock.activated) {
			display_data->clock.activated = 0;
			del_timer(&display_data->clock.timer);
			display_reset(0);
		}
	}
	return count;
}

static ssize_t clock_enabled_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", display_data->clock.activated);
}

static struct kobj_attribute reset_attribute =
		__ATTR(reset, 0644, NULL, reset_store);
static struct kobj_attribute print_attribute =
		__ATTR(print, 0644, NULL, print_store);
static struct kobj_attribute print_raw_attribute =
		__ATTR(print_raw, 0644, NULL, print_raw_store);
static struct kobj_attribute clock_enabled_attribute =
		__ATTR(clock_enabled, 0644, clock_enabled_show, clock_enabled_store);

static struct  attribute *attrs[] = {
		&reset_attribute.attr,
		&print_attribute.attr,
		&print_raw_attribute.attr,
		&clock_enabled_attribute.attr,
		NULL,
};


static struct attribute_group attr_group = {
		.attrs = attrs,
};


static int display_reboot_notifier(struct notifier_block *nb,  unsigned long code, void *unused)
{
	/* Send "rebo" on the display */
	display_send_data(0x0a9e3e3a);
	return NOTIFY_DONE;
}

/* Display netdev status:
 * NETDEV_UP				1
 * NETDEV_DOWN				2
 * NETDEV_REBOOT				3
 * NETDEV_CHANGE			4
 * NETDEV_REGISTER 			5
 * NETDEV_UNREGISTER		6
 * NETDEV_CHANGEMTU			7
 * NETDEV_CHANGEADDR		8
 * NETDEV_GOING_DOWN			9
 * NETDEV_CHANGENAME		10
 * NETDEV_FEAT_CHANGE		11
 * NETDEV_BONDING_FAILOVER 	12
 * NETDEV_PRE_UP			13
 * NETDEV_PRE_TYPE_CHANGE	14
 * NETDEV_POST_TYPE_CHANGE	15
 * NETDEV_POST_INIT			16
 * NETDEV_UNREGISTER_FINAL 	17
 * NETDEV_RELEASE			18
 * NETDEV_NOTIFY_PEERS		19
 * NETDEV_JOIN				20
 * NETDEV_CHANGEUPPER		21
 * NETDEV_RESEND_IGMP		22
 * NETDEV_PRECHANGEMTU		23
 * NETDEV_CHANGEINFODATA		24
 */
//static int display_netdev_notifier(struct notifier_block *nb, unsigned long code, void *unused)
//{
//	unsigned int raw_data;
//
//	raw_data = 0x9E020000;
//	raw_data |= digits_font_table[(code / 10 )] << 8;
//	raw_data |= digits_font_table[(code % 10)];
//	display_send_data(raw_data);
//	return NOTIFY_DONE;
//}

static struct notifier_block display_reboot_nb = {
		.notifier_call = display_reboot_notifier,
};
//static struct notifier_block display_netdev_nb = {
//		.notifier_call = display_netdev_notifier,
//};


static int display_probe(struct platform_device *pdev) {

	int ret;

	display_data = dev_get_platdata(&pdev->dev);
	if (!display_data) {
		display_data = display_parse_dt(&pdev->dev);
		if (IS_ERR(display_data)) {
			dev_err(&pdev->dev, "no platform data defined\n");
			return PTR_ERR(display_data);
		}
	}

	display_init_gpio();
	display_reset(0);


	/**
	 * Initialize sysfs
	 */
	display_kobj = kobject_create_and_add("display", kernel_kobj);
	if(!display_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(display_kobj, &attr_group);
	if(ret)
		kobject_put(display_kobj);

	return ret;
}

static int display_remove(struct platform_device *pdev)
{
	if(display_data->clock.activated)
		del_timer(&display_data->clock.timer);
	display_reset(0);
	platform_set_drvdata(pdev, NULL);
	kobject_put(display_kobj);
	return 0;
}
static struct platform_driver display_driver = {
		.probe = display_probe,
		.remove = display_remove,
		.driver = {
				.name = "display",
				.owner = THIS_MODULE,
				.of_match_table = of_match_ptr(display_of_match),
		},
};

static int __init display_init(void)
{
	int ret;
	ret = platform_driver_register(&display_driver);

	/* Register reboot notifier */
	if(!ret) {
		register_reboot_notifier(&display_reboot_nb);
	}
	return ret;
}


static void __exit display_exit(void)
{
	unregister_reboot_notifier(&display_reboot_nb);
	platform_driver_unregister(&display_driver);
}

module_init(display_init);
module_exit(display_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Stefan Mavrodiev <support@olimex.com>");
MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
