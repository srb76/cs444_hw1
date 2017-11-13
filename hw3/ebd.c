/*
 A modified version of ebd.c that includes AES encryption.
 Obtained from: http://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/
Samuel Bonner and Jack Neff 
 *
 A sample, extra-simple block driver. Updated for kernel 2.6.31.
 *
 * (C) 2003 Eklektix, Inc.
 * (C) 2010 Pat Patterson <pat at superpat dot com>
 * Redistributable under the terms of the GNU GPL.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/crypto.h>

MODULE_LICENSE("Dual BSD/GPL");
static char *Version = "1.4";

static int major_num = 0;
module_param(major_num, int, 0);
static int logical_block_size = 512;
module_param(logical_block_size, int, 0);
static int nsectors = 1024; /* How big the drive is */
module_param(nsectors, int, 0);
// *Added key parameter. 32 (8 * 4) bit key
static char key[4];
module_param(key, char, 0);

//Add cipher struct and cipher key values
struct crypto_cipher *aes;
static int key_size = 32;
static bool key_set = false;

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE 512

/*
 * Our request queue.
 */
static struct request_queue *Queue;

/*
 * The internal representation of our device.
 */
static struct ebd_device {
	unsigned long size;
	spinlock_t lock;
	u8 *data;
	struct gendisk *gd;
} Device;

/*
 * Handle an I/O request.
 * Added setting of crypto key, and encrypting / decrypting.
 */
static void ebd_transfer(struct ebd_device *dev, sector_t sector,
		unsigned long nsect, char *buffer, int write) {
	unsigned long offset = sector * logical_block_size;
	unsigned long nbytes = nsect * logical_block_size;
	
	int i = 0;

	if ((offset + nbytes) > dev->size) {
		printk (KERN_NOTICE "ebd: Beyond-end write (%ld %ld)\n", offset, nbytes);
		return;
	}
	
	//Set key if flag is not set
	if(!key_set) {
		//Clear then set key
		//crypto_cipher_clear_flags(aes,0); //try using if key is not setting correctly
		
		//Check key and use default if invalid
		int length = strlen(key);
		if( length != 4 )
			key = "abcd";
		
		//Set key
		printk("EBD.c : Using key %s\n",key);
		crypto_cipher_setkey(aes,key,strlen(key));
		key_set = true;
	}
	
	if (write) { //Writing data. Encrypt from buffer then save in data.
		//Encrypt byte by byte
		for(i=0;i<nbytes;i=(crypto_cipher_blocksize(aes)+i)) {
			crypto_cipher_encrypt_one(aes,dev->data+offset+i, buffer+i);
		}
		
		//memcpy(dev->data + offset, buffer, nbytes);
	}
	else { //Reading data. Decrypt from data then place into buffer
		//Decrypt byte by byte
		for(i=0;i<nbytes;i=(crypto_cipher_blocksize(aes)+i)) {
			crypto_cipher_decrypt_one(aes,buffer+i,dev->data+offset+i);
		}
		//memcpy(buffer, dev->data + offset, nbytes);
	}
}

static void ebd_request(struct request_queue *q) {
	struct request *req;

	req = blk_fetch_request(q);
	while (req != NULL) {
		// blk_fs_request() was removed in 2.6.36 - many thanks to
		// Christian Paro for the heads up and fix...
		//if (!blk_fs_request(req)) {
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
			printk (KERN_NOTICE "Skip non-CMD request\n");
			__blk_end_request_all(req, -EIO);
			continue;
		}
		ebd_transfer(&Device, blk_rq_pos(req), blk_rq_cur_sectors(req),
				req->buffer, rq_data_dir(req));
		if ( ! __blk_end_request_cur(req, 0) ) {
			req = blk_fetch_request(q);
		}
	}
}

/*
 * The HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which
 * calls this. We need to implement getgeo, since we can't
 * use tools such as fdisk to partition the drive otherwise.
 */
int ebd_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
	long size;

	/* We have no real geometry, of course, so make something up. */
	size = Device.size * (logical_block_size / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;
	return 0;
}

/*
 * The device operations structure.
 */
static struct block_device_operations ebd_ops = {
		.owner  = THIS_MODULE,
		.getgeo = ebd_getgeo
};

static int __init ebd_init(void) {
	/*
	 * Set up our internal device.
	 */
	Device.size = nsectors * logical_block_size;
	spin_lock_init(&Device.lock);
	Device.data = vmalloc(Device.size);
	if (Device.data == NULL)
		return -ENOMEM;
	/*
	 * Get a request queue.
	 */
	Queue = blk_init_queue(ebd_request, &Device.lock);
	if (Queue == NULL)
		goto out;
	blk_queue_logical_block_size(Queue, logical_block_size);
	/*
	 * Get registered.
	 */
	major_num = register_blkdev(major_num, "ebd");
	if (major_num < 0) {
		printk(KERN_WARNING "ebd: unable to get major number\n");
		goto out;
	}
	/*
	 * And the gendisk structure.
	 */
	Device.gd = alloc_disk(16);
	if (!Device.gd)
		goto out_unregister;
	Device.gd->major = major_num;
	Device.gd->first_minor = 0;
	Device.gd->fops = &ebd_ops;
	Device.gd->private_data = &Device;
	strcpy(Device.gd->disk_name, "ebd0");
	set_capacity(Device.gd, nsectors);
	Device.gd->queue = Queue;
	add_disk(Device.gd);
	
	//Initialize AES cipher
	aes = crypto_alloc_cipher("aes",0,16);
	printk("Allocated AES cipher.\n");

	return 0;

//Added freeing of cipher
out_unregister:
	unregister_blkdev(major_num, "ebd");
out:
	vfree(Device.data);
	crypto_free_cipher(aes);
	return -ENOMEM;
}

static void __exit ebd_exit(void)
{
	del_gendisk(Device.gd);
	put_disk(Device.gd);
	unregister_blkdev(major_num, "ebd");
	blk_cleanup_queue(Queue);
	vfree(Device.data);
}

module_init(ebd_init);
module_exit(ebd_exit);