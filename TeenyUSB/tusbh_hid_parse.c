

#include "tusbh_hid.h"
#include "string.h"


static const char *tag_string[64] = {
	NULL,            "UsagePage",     "Usage",     NULL, /* 0 */
	NULL,            "LogicMin",      "UsageMin",  NULL, /* 1 */
	NULL,            "LogicMax",      "UsageMax",  NULL, /* 2 */
	NULL,            "PhysicalMin",   NULL,        NULL, /* 3 */
	NULL,            "PhysicalMax",   NULL,        NULL, /* 4 */
	NULL,            "UnitExp",       NULL,        NULL, /* 5 */
	NULL,            "Unit",          NULL,        NULL, /* 6 */
	NULL,            "ReportSize",    NULL,        NULL, /* 7 */
	"Input",         "ReportID",      NULL,        NULL, /* 8 */
	"Output",        "ReportCount",   NULL,        NULL, /* 9 */
	"Collection",    "Push",          NULL,        NULL, /* a */
	"Feature",       "Pop",           NULL,        NULL, /* b */
	"EndCollection", NULL,            NULL,        NULL, /* c */
	NULL,            NULL,            NULL,        NULL, /* d */
	NULL,            NULL,            NULL,        NULL, /* e */
	NULL,            NULL,            NULL,        NULL, /* f */
};

void dump_report_desc(u8 *desc_buf, int desc_size)
{
	int level;
	int i, p, tag, size, data;
	char *str;

	printk("\nHID report descriptor:\n\n");

	level = 0;
	p = 0;
	while(p<desc_size){
		tag = desc_buf[p++];
		size = ( 1<<(tag&3) )/2;
		tag &= 0xfc;

		data = 0;
		for(i=size-1; i>=0; i--){
			data |= desc_buf[p+i]<<(i*8);
		}

		if(tag==0xc0)
			level -= 1;

		i = level*4;
		while(i){
			printk(" ");
			i -= 1;
		}

		str = (char*)tag_string[tag>>2];
		if(str==NULL)
			str = "Unknow";
		if(size){
			printk("%s %02x\n", str, data);
		}else{
			printk("%s\n", str);
		}
		if(tag==0xc0)
			printk("\n");
		if(tag==0xa0)
			level += 1;

		p += size;
	}

}

int match_report_desc(u8 *desc_buf, int desc_size, HID_ITEM *items, int items_size)
{
	HID_ITEM g_item;
	int i, p, tag, size, data;
	int match_item;

	memset(&g_item, 0, sizeof(g_item));

	if(items==NULL){
		printk("\nMain items:\n");
	}

	match_item = 0;
	p = 0;
	while(p<desc_size){
		tag = desc_buf[p++];
		size = ( 1<<(tag&3) )/2;
		tag &= 0xfc;

		data = 0;
		for(i=size-1; i>=0; i--){
			data |= desc_buf[p+i]<<(i*8);
		}

		if((tag&0x0c)==0){
			// Main item
			if((tag==0xa0 && data==1) || tag==0x80 || tag==0x90 || tag==0xb0){
				g_item.type = tag;
				g_item.flag = data;
				if(items){
					if(memcmp(&g_item, items, sizeof(g_item))==0){
						match_item += 1;
						items += 1;
						if(match_item==items_size){
							break;
						}
					}
				}else{
					printk("  0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%04x, 0x%04x\n",
						g_item.id, g_item.type, g_item.flag, g_item.size, g_item.count,
						g_item.usize, g_item.usage, g_item.page);
				}
			}
			// reset local item
			g_item.usage = 0;
			g_item.usize = 0;
		}else if((tag&0x0c)==4){
			// Global item
			switch(tag>>4){
			case 0: g_item.page  = data; break; // UsagePage
			case 7: g_item.size  = data; break; // ReportSize
			case 8: g_item.id    = data; break; // ReportID
			case 9: g_item.count = data; break; // ReportCount
			default: break;
			}
		}else if((tag&0x0c)==8){
			switch(tag>>4){
			case 0: // Usage
				if(g_item.usage==0){
					g_item.usage = data;
					g_item.usize = 1;
				}else{
					g_item.usize += 1;
				}
				break;
			case 1: // UsageMin
				g_item.usage = data;
				g_item.usize = 1;
				break;
			case 2: // UsageMax
				g_item.usize = data - g_item.usage + 1;
				break;
			}
		}

		p += size;
	}

	return match_item;
}


