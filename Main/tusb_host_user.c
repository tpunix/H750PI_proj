
#include "teeny_usb.h"
#include "tusbh.h"
#include "tusbh_hid.h"
#include "tusbh_msc.h"

#include "tntfs.h"


/******************************************************************************/


static int usb_msc_read_sector(void *itf, u8 *buf, u32 start, int size)
{
	return tusbh_msc_block_read((tusbh_interface_t*)itf, 0, start, size, buf);
}


static int usb_msc_write_sector(void *itf, u8 *buf, u32 start, int size)
{
	return tusbh_msc_block_write((tusbh_interface_t*)itf, 0, start, size, buf);
}


int msc_mount(tusbh_interface_t* interface, int max_lun, tusbh_block_info_t* blocks)
{
	tusbh_msc_info_t* info = tusbh_get_info(interface, tusbh_msc_info_t);
	BLKDEV *bdev;
	int retv;


	bdev = tusbh_malloc(sizeof(BLKDEV));
	memset(bdev, 0, sizeof(BLKDEV));
	info->user = bdev;

	bdev->itf = interface;
	bdev->sectors = blocks->block_count;
	bdev->read_sector  = usb_msc_read_sector;
	bdev->write_sector = usb_msc_write_sector;

	retv = f_initdev(bdev, "usb", 0);
	if(retv)
		return retv;

	retv = f_mount(bdev, 0);
	if(retv==0){
		f_list("usb0:/");
		printk("\n");
	}

	return 0;
}


int msc_umount(tusbh_interface_t* interface)
{
	tusbh_msc_info_t* info = tusbh_get_info(interface, tusbh_msc_info_t);
	BLKDEV *bdev = (BLKDEV*)info->user;

	if(bdev){
		f_removedev(bdev);
		tusbh_free(bdev);
	}

	return 0;
}


/******************************************************************************/


static HID_ITEM dualshock3_items[] = {
	{0x00, 0xa0, 0x01, 0x00, 0x00, 0x01, 0x0004, 0x0001},
	{0x01, 0x80, 0x03, 0x08, 0x01, 0x00, 0x0000, 0x0001}, // pad
	{0x01, 0x80, 0x02, 0x01, 0x13, 0x13, 0x0001, 0x0009},
	{0x01, 0x80, 0x03, 0x01, 0x0d, 0x00, 0x0000, 0xff00}, // pad
	{0x01, 0x80, 0x02, 0x08, 0x04, 0x04, 0x0030, 0x0001},
	{0x01, 0x80, 0x02, 0x08, 0x27, 0x01, 0x0001, 0x0001},
	{0x01, 0x90, 0x02, 0x08, 0x30, 0x01, 0x0001, 0x0001},
	{0x01, 0xb0, 0x02, 0x08, 0x30, 0x01, 0x0001, 0x0001},
	{0x02, 0xb0, 0x02, 0x08, 0x30, 0x01, 0x0001, 0x0001},
	{0xee, 0xb0, 0x02, 0x08, 0x30, 0x01, 0x0001, 0x0001},
	{0xef, 0xb0, 0x02, 0x08, 0x30, 0x01, 0x0001, 0x0001},
};
#define ds3_items_size (sizeof(dualshock3_items)/sizeof(HID_ITEM))


int dualshock3_init(tusbh_hid_info_t* info)
{
	tusbh_device_t* dev = info->dev;

	//u8 cmd[4] = {0x42, 0x0c, 0x00, 0x00};
	//tusbh_hid_set_report(dev, HID_FEATURE_REPORT, 0xf4, cmd, 4);
	u8 cmd[64];
	tusbh_hid_get_report(dev, HID_FEATURE_REPORT, 0xf2, cmd, 64);

	return 0;
}


tusbh_hid_driver_t hid_dualshock3 = {
	.name = "DualShock3",

	.vid = 0,
	.pid = 0,
	.main_items = dualshock3_items,
	.items_nr = ds3_items_size,

	.init = dualshock3_init,
	.exit = NULL,
	.on_recv_data = NULL,
	.on_send_done = NULL,
};


////////////////////////////////


static HID_ITEM nspro_items[] = {
	{0x00, 0xa0, 0x01, 0x00, 0x00, 0x01, 0x0004, 0x0001},
	{0x30, 0x80, 0x02, 0x01, 0x0a, 0x0a, 0x0001, 0x0009},
	{0x30, 0x80, 0x02, 0x01, 0x04, 0x04, 0x000b, 0x0009},
	{0x30, 0x80, 0x03, 0x01, 0x02, 0x00, 0x0000, 0x0009},
	{0x30, 0x80, 0x02, 0x10, 0x04, 0x04, 0x0030, 0x0009},
	{0x30, 0x80, 0x02, 0x04, 0x01, 0x01, 0x0039, 0x0009},
	{0x30, 0x80, 0x02, 0x01, 0x04, 0x04, 0x000f, 0x0009},
	{0x30, 0x80, 0x03, 0x08, 0x34, 0x00, 0x0000, 0x0009},
	{0x21, 0x80, 0x03, 0x08, 0x3f, 0x01, 0x0001, 0xff00},
	{0x81, 0x80, 0x03, 0x08, 0x3f, 0x01, 0x0002, 0xff00},
	{0x01, 0x90, 0x83, 0x08, 0x3f, 0x01, 0x0003, 0xff00},
	{0x10, 0x90, 0x83, 0x08, 0x3f, 0x01, 0x0004, 0xff00},
	{0x80, 0x90, 0x83, 0x08, 0x3f, 0x01, 0x0005, 0xff00},
	{0x82, 0x90, 0x83, 0x08, 0x3f, 0x01, 0x0006, 0xff00},
};
#define nspro_items_size (sizeof(nspro_items)/sizeof(HID_ITEM))

int nspro_recv_data(tusbh_ep_info_t* ep, const uint8_t* data, uint32_t len)
{
	hex_dump("NSPRO", (void*)data, len);
	return 0;
}

int nspro_init(tusbh_hid_info_t* info)
{
#if 0
	tusbh_device_t* dev = info->dev;
	u8 cmd[64];
	memset(cmd, 0, 64);
	cmd[0] = 0x80;
	cmd[1] = 0x02;
	retv = tusbh_hid_set_report(dev, HID_OUTPUT_REPORT, 0x80, cmd, 64);
	printk("send 80 02: %d\n", retv);
#endif
	return 0;
}


tusbh_hid_driver_t hid_nspro = {
	.name = "Switch Pro Controller",

	.vid = 0,
	.pid = 0,
	.main_items = nspro_items,
	.items_nr = nspro_items_size,

	.init = nspro_init,
	.exit = NULL,
	.on_recv_data = nspro_recv_data,
	.on_send_done = NULL,
};


////////////////////////////////


tusbh_hid_driver_t *hid_drivers[] = {
	&hid_dualshock3,
	&hid_nspro,
	NULL,
};


/******************************************************************************/


static tusb_host_t *otg_fs;
static tusbh_root_hub_t root_otg_fs;


static tusbh_class_t *class_table[] = {
    &tusbh_class_hid,
    &tusbh_class_msc,
    0,
};


int tusbh_main()
{
	msc_mount_func = msc_mount;
	msc_umount_func =msc_umount;
	tusbh_hid_driver_list = hid_drivers;

	otg_fs = tusb_get_host(USB_CORE_ID_FS);
    HOST_PORT_POWER_ON_FS();

    root_otg_fs.id = "FS";
    root_otg_fs.support_classes = class_table;
	tusb_host_init(otg_fs, &root_otg_fs);

	tusb_open_host(otg_fs);

	tusbh_thread_start(root_otg_fs.mq);
	
	return 0;
}

