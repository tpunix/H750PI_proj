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

// OS port for rt-thread

#include "tusbh.h"
#include "tusbh_os.h"
#include "cmsis_os2.h"
#include "rtx_os.h"
#include <string.h>

void tusb_delay_ms(uint32_t ms)
{
	if(ms<10)
		ms = 10;
	ms = (ms+9)/10;
	osDelay(ms);
}


tusbh_msg_q_t* tusbh_mq_create(void)
{
	osMessageQueueId_t mq = osMessageQueueNew(16, sizeof(tusbh_message_t), NULL);
    return (tusbh_msg_q_t*)mq;
}

void tusbh_mq_free(tusbh_msg_q_t* mq)
{
	osMessageQueueDelete((osMessageQueueId_t)mq);
}

int tusbh_mq_init(tusbh_msg_q_t* mq)
{
	return 0;
}

int tusbh_mq_post(tusbh_msg_q_t* mq, const tusbh_message_t* msg)
{
	osStatus_t retv;

	retv = osMessageQueuePut((osMessageQueueId_t)mq, msg, 0, 0);
	//printk("tusbh_mq_post: retv=%d\n", retv);
	if(retv==osOK)
		return 0;

	return -1;
}

// return value: 1 - success, 0 
int tusbh_mq_get(tusbh_msg_q_t* mq, tusbh_message_t* msg)
{
	osStatus_t retv;

	retv = osMessageQueueGet((osMessageQueueId_t)mq, msg, NULL, osWaitForever);
	if(retv==osOK)
		return 1;

	return 0;
}

void tusbh_main_loop(void *arg)
{
	tusbh_msg_q_t *mq = (tusbh_msg_q_t*)arg;

	while(1){
		tusbh_msg_loop(mq);
	}
}

void tusbh_thread_start(tusbh_msg_q_t* mq)
{
	osThreadNew((osThreadFunc_t)tusbh_main_loop, mq, NULL);
}


/// API for event

tusbh_evt_t* tusbh_evt_create(void)
{
	osSemaphoreId_t evt = osSemaphoreNew(8, 0, NULL);
	//printk("tusbh_evt_create: evt=%08x\n", evt);
    return (tusbh_evt_t*)evt;
}

void tusbh_evt_free(tusbh_evt_t* evt)
{
	osSemaphoreDelete((osSemaphoreId_t)evt);
}

int tusbh_evt_init(tusbh_evt_t* evt)
{
    return 0;
}

int tusbh_evt_set(tusbh_evt_t* evt)
{
	//printk("tusbh_evt_set: evt=%08x\n", evt);
	osSemaphoreRelease((osSemaphoreId_t)evt);
    return 0;
}

// return 0 for success, otherwise fail
int tusbh_evt_wait(tusbh_evt_t* evt, uint32_t timeout_ms)
{
	osStatus_t retv;

	//printk("tusbh_evt_wait: evt=%08x %d\n", evt, timeout_ms);
	retv = osSemaphoreAcquire((osSemaphoreId_t)evt, (timeout_ms+10)/10);
	if(retv==osOK)
		return 0;

    return -1;
}

int tusbh_evt_clear(tusbh_evt_t* evt)
{
    return 0;
}

// API for memory manage

void    *osRtxMemoryAlloc(void *mem, uint32_t size, uint32_t type);
uint32_t osRtxMemoryFree (void *mem, void *block);

// return memory must 32 bit aligned
// real allocate memory size muse be muple of 4 (USB FIFO requirement)
void* tusbh_malloc(uint32_t size)
{
	return malloc(size);
}

void tusbh_free(void* p)
{
	free(p);
}

