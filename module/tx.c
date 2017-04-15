/*
 *  RC Transmitter as Joystick
 *  Copyright (C) 2010  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
 *  Copyright (C) 2011  Tomas 'ZeXx86' Jedrzejek (zexx86@zexos.org)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define DEVICE_NAME 	"tx"   /* Dev name as it appears in /proc/devices   */
#define DEVICE_MAJOR	222

MODULE_AUTHOR ("Tomas 'ZeXx86' Jedrzejek <zexx86@zexos.org>");
MODULE_DESCRIPTION ("RC transmitter with PPM modulation as joystick");
MODULE_LICENSE ("GPL");

struct tx_dev_t {
	int chan[12];		/* 12 possible channels of TX */

	int opened;     	/* Is device open ? */
	int major;
	
	struct class *fc;
	
	struct cdev txdev;
		
	struct input_dev *input_dev;
};

static struct tx_dev_t tx_dev;

static int device_open (struct inode *inode, struct file *file)
{
	struct tx_dev_t *tx = &tx_dev;
	
	if (!tx)
		return 0;

        if (tx->opened)
                return -EBUSY;
	
	tx->opened ++;

        try_module_get (THIS_MODULE);

        return 0;
}

static int device_release (struct inode *inode, struct file *file)
{
	struct tx_dev_t *tx = &tx_dev;
	
	if (!tx)
		return 0;

        tx->opened --;          /* We're now ready for our next caller */

        /* 
         * Decrement the usage count, or else once you opened the file, you'll
         * never get rid of the module. 
         */
        module_put (THIS_MODULE);

        return 0;
}

/*  
 * Called when a process writes to dev file: echo "hi" > /dev/hello 
 */
static ssize_t device_write (struct file *filp, const char *buff, size_t len, loff_t * off)
{
	if (len != (sizeof (int) * 12)) {
		printk (KERN_ALERT "Incompatibile data\n");
		return -EINVAL;
	}

	struct tx_dev_t *tx = &tx_dev;
	
	if (!tx)
		return 0;
	
	/* copy userspace data to kernelspace */
	int r = copy_from_user (&tx->chan, buff, len);
	
	/* report new values to joystick device */
	input_report_abs (tx->input_dev, ABS_X, tx->chan[0]);
	input_report_abs (tx->input_dev, ABS_Y, tx->chan[1]);
	input_report_abs (tx->input_dev, ABS_Z, tx->chan[2]);
	input_report_abs (tx->input_dev, ABS_THROTTLE, tx->chan[3]);
	input_report_abs (tx->input_dev, ABS_RUDDER, tx->chan[4]);
	input_report_abs (tx->input_dev, ABS_MISC, tx->chan[5]);
	
        return len;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

static int tx_open (struct input_dev *dev)
{
	struct tx_dev_t *tx = input_get_drvdata (dev);

	return 0;
}

static void tx_close (struct input_dev *dev)
{
	struct tx_dev_t *tx = input_get_drvdata (dev);
}

static int tx_connect (struct tx_dev_t *tx)
{
	int err = -ENODEV;

	tx->input_dev = input_allocate_device ();
	
	if (!tx->input_dev)
		return err;

	input_set_drvdata (tx->input_dev, tx);

	tx->input_dev->name = "PPM Transmitter";

	/* TODO what id vendor/product/version ? */
	tx->input_dev->id.vendor = 0x0001;
	tx->input_dev->id.product = 0x0001;
	tx->input_dev->id.version = 0x0100;
	tx->input_dev->open = tx_open;
	tx->input_dev->close = tx_close;

	tx->input_dev->evbit[0] = BIT (EV_ABS) | BIT_MASK (EV_KEY);
	tx->input_dev->keybit[BIT_WORD (BTN_GEAR_DOWN)] = BIT_MASK (BTN_GEAR_DOWN);

	input_set_abs_params (tx->input_dev, ABS_X, -512, 512, 0, 0);
	input_set_abs_params (tx->input_dev, ABS_Y, -512, 512, 0, 0);
	input_set_abs_params (tx->input_dev, ABS_Z, -512, 512, 0, 0);
	input_set_abs_params (tx->input_dev, ABS_THROTTLE, -512, 512, 0, 0);
	input_set_abs_params (tx->input_dev, ABS_RUDDER, -512, 512, 0, 0);
	input_set_abs_params (tx->input_dev, ABS_MISC, -512, 512, 0, 0);

	err = input_register_device (tx->input_dev);

	if (err)
		goto error;

/*	tx->major = register_chrdev (DEVICE_MAJOR, DEVICE_NAME, &fops);

        if (tx->major < 0) {
		printk (KERN_ALERT "txppm -> Registering char device failed with %d\n", tx->major);
		return err;
        }
	
	tx->major = DEVICE_MAJOR;
	printk (KERN_INFO "txppm -> use: 'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, tx->major);*/

	tx->major = DEVICE_MAJOR;

	int result;
	dev_t dev = MKDEV (tx->major, 0);

	/* Figure out our device number. */
	result = register_chrdev_region (dev, 1, "tx");
	
	if (result < 0) {
		printk (KERN_WARNING "tx: unable to get major %d\n", tx->major);
		
		dev = MKDEV (0, 0);
		result = alloc_chrdev_region (&dev, 0, 1, "tx");
                tx->major = MAJOR (dev);
		printk (KERN_NOTICE "tx: obtained new major %d\n", tx->major);
	}

	/* Now set up cdev. */
	int devno = MKDEV (tx->major, 0);

	cdev_init (&tx->txdev, &fops);
	
	tx->txdev.owner = THIS_MODULE;
	tx->txdev.ops = &fops;
	
	/* Fail gracefully if need be */
	if (cdev_add (&tx->txdev, devno, 1)) {
		printk (KERN_NOTICE "Error %d adding tx%d", err, devno);
		err = -1;
		goto error;
	} else
		printk (KERN_INFO "tx: device /dev/%s created (major: %d minor: 0)\n", DEVICE_NAME, tx->major);

	tx->fc = class_create (THIS_MODULE, "txppm");

	device_create (tx->fc, NULL, devno, "%s","tx");

	return 0;
error:
	input_free_device (tx->input_dev);

	return err;
}

static void tx_disconnect (struct tx_dev_t *tx)
{
	input_unregister_device (tx->input_dev);
	
	//unregister_chrdev (tx->major, DEVICE_NAME);
	cdev_del (&tx->txdev);
	
	device_destroy (tx->fc, MKDEV (tx->major, 0));
	class_destroy (tx->fc);

	unregister_chrdev_region (MKDEV (tx->major, 0), 1);
}

static int __init tx_init (void)
{
	return tx_connect (&tx_dev);
}

static void __exit tx_exit (void)
{
	tx_disconnect (&tx_dev);
}

module_init (tx_init);
module_exit (tx_exit);
