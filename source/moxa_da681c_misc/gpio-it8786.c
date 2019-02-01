/*
 *  GPIO interface for Moxa Computer - IT8786 Super I/O chip
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License 2 as published
 *  by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/gpio.h>

#define GPIO_NAME		"moxa-it8786-gpio"
#define SIO_CHIP_ID		0x8786
#define CHIP_ID_HIGH_BYTE	0x20
#define CHIP_ID_LOW_BYTE	0x21

#define GPIO_BA_HIGH_BYTE	0x62
#define GPIO_BA_LOW_BYTE	0x63
#define GPIO_IOSIZE		8

#define GPIO_GROUP_1		0
#define GPIO_GROUP_2		1
#define GPIO_GROUP_3		2
#define GPIO_GROUP_4		3
#define GPIO_GROUP_5		4
#define GPIO_GROUP_6		5
#define GPIO_GROUP_7		6
#define GPIO_GROUP_8		7
#define GPIO_BIT_0		(1<<0)
#define GPIO_BIT_1		(1<<1)
#define GPIO_BIT_2		(1<<2)
#define GPIO_BIT_3		(1<<3)
#define GPIO_BIT_4		(1<<4)
#define GPIO_BIT_5		(1<<5)
#define GPIO_BIT_6		(1<<6)
#define GPIO_BIT_7		(1<<7)

/*
 * Customization for DA-681C
 */

/* Relay output pin */
static u8 relay_pin_def[] = {
	GPIO_GROUP_6, GPIO_BIT_5,
};

/* Programable LED */
static u8 pled_pin_def[] = {
	GPIO_GROUP_7, GPIO_BIT_0,
	GPIO_GROUP_7, GPIO_BIT_1,
	GPIO_GROUP_7, GPIO_BIT_2,
	GPIO_GROUP_7, GPIO_BIT_3,
	GPIO_GROUP_7, GPIO_BIT_4,
	GPIO_GROUP_7, GPIO_BIT_5,
	GPIO_GROUP_7, GPIO_BIT_6,
	GPIO_GROUP_7, GPIO_BIT_7,
};

/* DI pin */
static u8 di_pin_def[] = {
	GPIO_GROUP_8, GPIO_BIT_2,	/* di0 */
	GPIO_GROUP_8, GPIO_BIT_3,	/* di1 */
	GPIO_GROUP_8, GPIO_BIT_4,	/* di2 */
	GPIO_GROUP_8, GPIO_BIT_5,	/* di3 */
	GPIO_GROUP_3, GPIO_BIT_6,	/* do4 */
	GPIO_GROUP_3, GPIO_BIT_7,	/* do5 */
};

/* DO pin */
static u8 do_pin_def[] = {
	GPIO_GROUP_3, GPIO_BIT_3,	/* do0 */
	GPIO_GROUP_3, GPIO_BIT_4,	/* do1 */
};

static u8 ports[1] = { 0x2e };
static u8 port;
 
static DEFINE_SPINLOCK(sio_lock);
static u16 gpio_ba;

static u8 read_reg(u8 addr, u8 port)
{
	outb(addr, port);
	return inb(port + 1);
}

static void write_reg(u8 data, u8 addr, u8 port)
{
	outb(addr, port);
	outb(data, port + 1);
}

static int enter_conf_mode(u8 port)
{
	/*
	 * Try to reserve REG and REG + 1 for exclusive access.
	 */
	if (!request_muxed_region(port, 2, GPIO_NAME))
		return -EBUSY;

	outb(0x87, port);
	outb(0x01, port);
	outb(0x55, port);
	outb((port == 0x2e) ? 0x55 : 0xaa, port);

	return 0;
}

static void exit_conf_mode(u8 port)
{
	outb(0x2, port);
	outb(0x2, port + 1);
	release_region(port, 2);
}

static void enter_gpio_mode(u8 port)
{
	write_reg(0x7, 0x7, port);
}

void write_gpio(u8 *pindef, unsigned num, int val)
{
	u16 reg;
	u8 bit;
	u8 curr_vals;

	reg = gpio_ba + pindef[num*2];
	bit = pindef[num*2+1];

	spin_lock(&sio_lock);
	curr_vals = inb(reg);
	if (val)
		outb(curr_vals | bit, reg);
	else
		outb(curr_vals & ~bit, reg);
	spin_unlock(&sio_lock);
}

int read_gpio(u8 *pindef, unsigned num)
{
	u16 reg;
	u8 bit;

	reg = gpio_ba + pindef[num*2];
	bit = pindef[num*2+1];

	return !!(inb(reg) & bit);
}

int pled_set(unsigned num, int val)
{
	if (num >= (sizeof(pled_pin_def)/2)) {
		return -EINVAL;
	}

	write_gpio(pled_pin_def, num, val);

	return 0;
}

int pled_get(unsigned num, int *val)
{
	if (num >= (sizeof(pled_pin_def)/2)) {
		return -EINVAL;
	}

	*val = read_gpio(pled_pin_def, num);

	return 0;
}

int relay_set(unsigned num, int val)
{
	if (num >= (sizeof(relay_pin_def)/2)) {
		return -EINVAL;
	}

	write_gpio(relay_pin_def, num, val);

	return 0;
}

int relay_get(unsigned num, int *val)
{
	if (num >= (sizeof(relay_pin_def)/2)) {
		return -EINVAL;
	}

	*val = read_gpio(relay_pin_def, num);

	return 0;
}

int di_get(unsigned num, int *val)
{
	if (num >= (sizeof(di_pin_def)/2)) {
		return -EINVAL;
	}
	*val = read_gpio(di_pin_def, num);
	return 0;
}

int do_set(unsigned num, int val)
{
	if (num >= (sizeof(do_pin_def)/2)) {
		return -EINVAL;
	}
	write_gpio(do_pin_def, num, val);
	return 0;
}

int do_get(unsigned num, int *val)
{
	if (num >= (sizeof(do_pin_def)/2)) {
		return -EINVAL;
	}
	*val = read_gpio(do_pin_def, num);
	return 0;
}

int gpio_init(void)
{
	int err;
	int i, id;

	/* chip and port detection */
	for (i = 0; i < ARRAY_SIZE(ports); i++) {
		spin_lock(&sio_lock);
		err = enter_conf_mode(ports[i]);
		if (err) {
			spin_unlock(&sio_lock);
			return err;
		}

		id = (read_reg(CHIP_ID_HIGH_BYTE, ports[i]) << 8) +
			read_reg(CHIP_ID_LOW_BYTE, ports[i]);
		exit_conf_mode(ports[i]);
		spin_unlock(&sio_lock);

		if (id == SIO_CHIP_ID) {
			port = ports[i];
			break;
		}
	}

	if (!port)
		return -ENODEV;

	/* fetch GPIO base address */
	spin_lock(&sio_lock);
	err = enter_conf_mode(port);
	if (err) {
		spin_unlock(&sio_lock);
		return err;
	}

	enter_gpio_mode(port);
	gpio_ba = (read_reg(GPIO_BA_HIGH_BYTE, port) << 8) +
			read_reg(GPIO_BA_LOW_BYTE, port);
	exit_conf_mode(port);
	spin_unlock(&sio_lock);

	if (!request_region(gpio_ba, GPIO_IOSIZE, GPIO_NAME)) {
		printk("request_region address 0x%x failed!\n", gpio_ba);
		gpio_ba = 0;
		return -EBUSY;
	}

	return 0;
}

void gpio_exit(void)
{
	if (gpio_ba) {
		release_region(gpio_ba, GPIO_IOSIZE);
		gpio_ba = 0;
	}
}
