
#include "main.h"
#include "rtx_os.h"

/******************************************************************************/


void device_init(void)
{
	RCC->AHB3ENR  = 0x00005001;  /* QSPI FMC MDMA*/
	RCC->AHB1ENR  = 0x08000003;  /* OTG2_FS DMA1 DMA1 */
	RCC->AHB2ENR  = 0xe0000200;  /* SRAM3 SRAM2 SRAM1 SDMMC2 */
	RCC->AHB4ENR  = 0x000007ff;  /* GPIO */

	RCC->APB1LENR = 0x0008c000;  /* USART4 SPI3 SPI2 */
	RCC->APB1HENR = 0x00000000;  /* */
	RCC->APB2ENR  = 0x00001000;  /* SPI1 */
	RCC->APB3ENR  = 0x00000000;  /**/
	RCC->APB4ENR  = 0x00010001;  /* RTC SYSCFG */
	
	RCC->AHB1LPENR &= ~0x14000000;

	GPIOA->MODER  = 0x2a800002;
	GPIOA->OTYPER = 0x00000000;
	GPIOA->OSPEEDR= 0x16800001;
	GPIOA->PUPDR  = 0x00000000;
	GPIOA->AFR[0] = 0x00000009;
	GPIOA->AFR[1] = 0x000aa000;

	GPIOB->MODER  = 0xa02a22a0;
	GPIOB->OTYPER = 0x00000000;
	GPIOB->OSPEEDR= 0xa02012a0;
	GPIOB->PUPDR  = 0x00000000;
	GPIOB->AFR[0] = 0x0a099900;
	GPIOB->AFR[1] = 0x99000588;

	GPIOC->MODER  = 0x042800a1;
	GPIOC->OTYPER = 0x00000000;
	GPIOC->OSPEEDR= 0x042800a1;
	GPIOC->PUPDR  = 0x00000000;
	GPIOC->AFR[0] = 0x00005500;
	GPIOC->AFR[1] = 0x00000990;

	GPIOD->MODER  = 0x08002000;
	GPIOD->OTYPER = 0x00000000;
	GPIOD->OSPEEDR= 0x08002000;
	GPIOD->PUPDR  = 0x00000000;
	GPIOD->AFR[0] = 0x0b000000;
	GPIOD->AFR[1] = 0x00900000;

	GPIOE->MODER  = 0x00001420;
	GPIOE->OTYPER = 0x00000000;
	GPIOE->OSPEEDR= 0x00000020;
	GPIOE->PUPDR  = 0x00000000;
	GPIOE->AFR[0] = 0x00000900;
	GPIOE->AFR[1] = 0x00000000;


	uart4_init();
}


void HardFault_Handler(void)
{
	char hbuf[128];
	_puts("\r\n\r\nHardFault!\r\n");
	sprintk(hbuf, "HFSR=%08x CFSR=%08x BFAR=%08x MMFAR=%08x\n",
			SCB->HFSR, SCB->CFSR, SCB->BFAR, SCB->MMFAR);
	_puts(hbuf);
	while(1);
}

void BusFault_Handler(void)
{
	_puts("\r\n\r\nBusFault!\r\n");
}

void UsageFault_Handler(void)
{
	_puts("\r\n\r\nUsageFault!\r\n");
}


/******************************************************************************/

void    *osRtxMemoryAlloc(void *mem, uint32_t size, uint32_t type);
uint32_t osRtxMemoryFree (void *mem, void *block);

void *malloc(uint32_t size)
{
	void *p = osRtxMemoryAlloc(osRtxInfo.mem.common, size, 0);
	//printk("malloc: %08x  %d\n", p, size);
	return p;
}

void free(void *p)
{
	//printk("free: %08x\n", p);
	osRtxMemoryFree(osRtxInfo.mem.common, p);
}


/******************************************************************************/

int tusbh_main();

void main_task(void *arg)
{
	int cnt = 0;

	device_init();

	printk("\n\nSTM32H750 RTX start!\n\n");

	sdio_init();
	
	tusbh_main();

	while(1){
		GPIOE->BSRR = 0x0060<<16;
		osDelay(50);

		GPIOE->BSRR = 0x0060;
		osDelay(50);

		cnt += 1;
	}

}


int main(void)
{
	osKernelInitialize();
	osThreadNew(main_task, NULL, NULL);
	osKernelStart();

	return 0;
}

