/*       
 *         _______                    _    _  _____ ____  
 *        |__   __|                  | |  | |/ ____|  _ \
 *           | | ___  ___ _ __  _   _| |  | | (___ | |_) |
 *           | |/ _ \/ _ \ '_ \| | | | |  | |\___ \|  _ < 
 *           | |  __/  __/ | | | |_| | |__| |____) | |_) |
 *           |_|\___|\___|_| |_|\__, |\____/|_____/|____/ 
 *                               __/ |                    
 *                              |___/                     
 *
 * TeenyUSB - light weight usb stack for STM32 micro controllers
 * 
 * Copyright (c) 2019 XToolBox  - admin@xtoolbox.org
 *                         www.tusb.org
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "tusbh_hid.h"
#include "string.h"


typedef __PACK_BEGIN struct _usb_hid_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    __PACK_BEGIN struct {
        uint8_t  bDescriptorType;
        uint16_t wDescriptorLength;
    } __PACK_END report_desc[1];
} __PACK_END usb_hid_descriptor_t;


tusbh_hid_driver_t **tusbh_hid_driver_list = NULL;

static int tusbh_hid_get_report_desc(tusbh_device_t *dev, tusbh_interface_t* interface, void *buf, int len);


static void tusbh_hid_find_driver(tusbh_device_t* dev, tusbh_interface_t* interface)
{
	tusbh_hid_info_t* info = tusbh_get_info(interface, tusbh_hid_info_t);
	tusbh_hid_driver_t **list, *drv;

	list = tusbh_hid_driver_list;
	if(list==NULL)
		return;

	while(1){
		drv = *list++;
		if(drv==NULL)
			break;

		if(drv->vid && drv->vid != dev->device_desc.idVendor)
			continue;
		if(drv->pid && drv->pid != dev->device_desc.idProduct)
			continue;
		if(drv->main_items){
			int retv = match_report_desc(info->report_desc, info->report_desc_len, drv->main_items, drv->items_nr);
			if(retv < drv->items_nr)
				continue;
		}

		break;
	}
	if(drv==NULL)
		return;

	drv->info = info;
	info->driver = drv;
	printk("\n%s Matched!\n", drv->name);

	if(drv->init)
		drv->init(info);

}

static int tusbh_hid_init(tusbh_device_t* dev, tusbh_interface_t* interface, int cfg_offset)
{
    usb_interface_descriptor_t* itf = (usb_interface_descriptor_t*)(dev->config_desc+cfg_offset);
    uint16_t wTotalLength = ((usb_config_descriptor_t*)dev->config_desc)->wTotalLength;
    uint8_t ep_index = 0;
    uint8_t itf_cnt = 0;

	interface->ep_num = itf->bNumEndpoints;
    create_info_pool(interface, tusbh_hid_info_t);
    memset(interface->info_pool,0,sizeof(tusbh_hid_info_t));

    tusbh_hid_info_t* info = tusbh_get_info(interface, tusbh_hid_info_t);
	info->dev = dev;
	info->interface = interface;

    memset(interface->endpoints, 0, sizeof(interface->endpoints));

    while(cfg_offset < wTotalLength){
        uint8_t len = dev->config_desc[cfg_offset];
        uint8_t t = dev->config_desc[cfg_offset+1];
        if(t == USB_ENDPOINT_DESCRIPTOR_TYPE){
            usb_endpoint_descriptor_t* ep = (usb_endpoint_descriptor_t*)(dev->config_desc+cfg_offset);
            if(ep_index < TUSBH_MAX_EP){
                setup_ep_info(&interface->endpoints[ep_index], ep, interface);
                if(ep->bEndpointAddress & 0x80){
                    if(!info->ep_in){
                        info->ep_in = &interface->endpoints[ep_index];
                    }
                }else{
                    if(!info->ep_out){
                        info->ep_out = &interface->endpoints[ep_index];
                    }
                }
            }else{
                TUSB_ITF_INFO("Endpoint count large than TUSBH_MAX_EP in HUB interface\n");
            }
            ep_index++;
        }else if(t == USB_HID_DESCRIPTOR_TYPE){
            usb_hid_descriptor_t* hid = (usb_hid_descriptor_t*)(dev->config_desc+cfg_offset);
            if(hid->bNumDescriptors > 0){
                if(hid->report_desc[0].bDescriptorType != USB_REPORT_DESCRIPTOR_TYPE){
                    TUSB_ITF_INFO("Warning: HID class descriptor type is not REPORT, %02x\n", hid->report_desc[0].bDescriptorType);
                }
                uint8_t* p = (uint8_t*) &hid->report_desc[0].wDescriptorLength;
                info->report_desc_len = p[0] | (p[1]<<8);
            }
            if(hid->bNumDescriptors > 1){
                TUSB_ITF_INFO("Warning: HID class descriptor count large than 1\n");
            }
        }else if(t == USB_INTERFACE_DESCRIPTOR_TYPE){
            if(itf_cnt > 0){
                break;
            }
            itf_cnt++;
        }else if(t == USB_IAD_DESCRIPTOR_TYPE){
            break;
        }
        cfg_offset+=len;
    }

	if(ep_index != interface->ep_num){
        TUSB_ITF_INFO("Endpoint count error espect %d, got %d\n", interface->ep_num, ep_index);
        return -1;
    }

	if(!info->ep_in){
        TUSB_ITF_INFO("Fail to get hid in endpoint\n");
        return -1;
    }

	// allocate a buffer for the IN ep, so it will start automatically
	info->ep_in->data = tusbh_malloc( EP_MPS(info->ep_in->desc) );
	if(!info->ep_in->data){
		TUSB_ITF_INFO("Fail to allocate memory for HID in endpoint\n");
		return -1;
	}

	if(tusbh_ep_allocate_pipe(info->ep_in) < 0){
		TUSB_ITF_INFO("Fail to allocate pipe for HID in endpoint\n");
		return -1;
	}

	if( (info->ep_in->desc->bmAttributes & USBD_EP_TYPE_MASK) != USBD_EP_TYPE_INTR){
        TUSB_ITF_INFO("HID in ep attr error %d\n", info->ep_in->desc->bmAttributes);
        return -1;
    }
    if( info->ep_out){
        if( (info->ep_out->desc->bmAttributes & USBD_EP_TYPE_MASK) != USBD_EP_TYPE_INTR){
            TUSB_ITF_INFO("HID out ep attr error %d\n", info->ep_out->desc->bmAttributes);
            return -1;
        }
    }
    
	if(info->report_desc_len > 0){
		info->report_desc = tusbh_malloc(info->report_desc_len);
		if(info->report_desc){
			int r = tusbh_hid_get_report_desc(dev, interface, info->report_desc, info->report_desc_len);
			if(r<0){
				TUSB_ITF_INFO("Fail to get report descriptor\n");
				return -1;
			}
		}else{
			TUSB_ITF_INFO("Fail to allocate memory for report descriptor\n");
		}
	}

	TUSB_ITF_INFO("HID interface init\n", interface);
    TUSB_ITF_INFO("Ep  in %02x\n", info->ep_in->desc->bEndpointAddress);
    if(info->ep_out){
        TUSB_ITF_INFO("Ep out %02x\n", info->ep_out->desc->bEndpointAddress);
    }

	int retv = tusbh_hid_set_idle(dev, 0, 0);
	TUSB_ITF_INFO("HID_SET_IDLE: %d\n", retv);

	tusbh_hid_find_driver(dev, interface);
	if(info->driver==NULL){
		TUSB_ITF_INFO("Unknow HID device!\n");
		dump_report_desc(info->report_desc, info->report_desc_len);
		match_report_desc(info->report_desc, info->report_desc_len, NULL, 0);
	}

	return cfg_offset;
}

static int tusbh_hid_deinit(tusbh_device_t* dev, tusbh_interface_t* interface)
{
	tusbh_hid_info_t* info = tusbh_get_info(interface, tusbh_hid_info_t);

	TUSB_ITF_INFO("HID interface deinit\n");

	if(info->driver){
		if(info->driver->exit){
			info->driver->exit(info);
		}
	}

	if(info->ep_in){
		if(info->ep_in->data){
			tusbh_free(info->ep_in->data);
			info->ep_in->data = 0;
		}
		tusbh_ep_free_pipe(info->ep_in);
	}

	if(info->ep_out){
		if(info->ep_out->data){
			tusbh_free(info->ep_out->data);
			info->ep_out->data = 0;
		}
		tusbh_ep_free_pipe(info->ep_out);
	}

	if(info->report_desc){
		tusbh_free(info->report_desc);
		info->report_desc = 0;
	}

	tusbh_free_info(interface);
	return 0;
}


/******************************************************************************/

static int tusbh_hid_get_report_desc(tusbh_device_t *dev, tusbh_interface_t* interface, void *buf, int len)
{
	int retv;

	retv = tusbh_control_xfer(dev, USB_D2H | USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_STANDARD, USB_REQ_GET_DESCRIPTOR,
				(USB_REPORT_DESCRIPTOR_TYPE<<8), interface->desc->bInterfaceNumber, buf, len);
	return retv;
}


int tusbh_hid_set_idle(tusbh_device_t *dev, int idle, int report_id)
{
	int retv = tusbh_control_xfer(dev, USB_H2D | USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_CLASS, USB_HID_SET_IDLE,
                    (idle<<8)|report_id, 0, NULL, 0);
	return retv;
}

int tusbh_hid_set_protocol(tusbh_device_t *dev, int protocol)
{
	int retv = tusbh_control_xfer(dev, USB_H2D | USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_CLASS, USB_HID_SET_PROTOCOL,
                    protocol, 0, NULL, 0);
	return retv;
}

int tusbh_hid_set_report(tusbh_device_t *dev, int report_type, int report_id, void *buf, int len)
{
	int retv = tusbh_control_xfer(dev, USB_H2D | USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_CLASS, USB_HID_SET_REPORT,
                    (report_type<<8)|report_id, 0, buf, len);
	return retv;
}

int tusbh_hid_get_report(tusbh_device_t *dev, int report_type, int report_id, void *buf, int len)
{
	int retv = tusbh_control_xfer(dev, USB_D2H | USB_REQ_RECIPIENT_INTERFACE | USB_REQ_TYPE_CLASS, USB_HID_GET_REPORT,
                    (report_type<<8)|report_id, 0, buf, len);
	return retv;
}


int tusbh_hid_send_data(tusbh_ep_info_t* ep, void* data, uint32_t len)
{
    int pipe_num = ep->pipe_num;
    if(pipe_num<0){
        pipe_num = tusbh_ep_allocate_pipe(ep);
    }
    if( pipe_num < 0 ){
        return pipe_num;
    }
    tusb_host_xfer_data(ep_host(ep), pipe_num, 1, data, len);
    return 0;
}


/******************************************************************************/


static int hid_xfered(tusbh_ep_info_t* ep)
{
    tusbh_hid_info_t* info = tusbh_get_info(ep->interface, tusbh_hid_info_t);
    tusb_hc_data_t* hc = &ep_host(ep)->hc[ep->pipe_num];
    tusbh_device_t* dev = ep_device(ep);
    
    if(ep == info->ep_out){
        if(info->driver && info->driver->on_send_done){
            info->driver->on_send_done(ep, (channel_state_t)hc->state);
        }
		return 0;
    }
    
    if(ep->desc->bEndpointAddress != info->ep_in->desc->bEndpointAddress){
        TUSB_DEV_INFO("HID Mouse Wrong ep xfered handler, espect %02x, got %02x\n", info->ep_in->desc->bEndpointAddress, ep->desc->bEndpointAddress);
        return -1;
    }
    
    if(hc->state != TUSB_CS_TRANSFER_COMPLETE){
        return -1;
    }
    
    if(ep == info->ep_in){
		if(info->driver && info->driver->on_recv_data){
            info->driver->on_recv_data(ep, (uint8_t*)ep->data, ep->data_len);
        }else{
            TUSB_EP_INFO("HID: got %d bytes\n", (int)ep->data_len);
        }
    }
    
    return 0;
}

tusbh_class_t  tusbh_class_hid = {
    .vid = 0,
    .pid = 0,
    .bInterfaceClass = USB_CLASS_HID,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .init = tusbh_hid_init,
    .deinit = tusbh_hid_deinit,
    .data_xfered = hid_xfered,
    .desc = "HID Class",
};
