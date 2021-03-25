#ifndef _MAIN_H_
#define _MAIN_H_


#include <stm32h7xx.h>
#include <cmsis_os2.h>
#include <string.h>

/******************************************************************************/

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned  long long u64;
typedef long long s64;



#define in_isr()        __get_IPSR()
#define disable_irq()   __disable_irq()
#define restore_irq(pm) __set_PRIMASK(pm)



/******************************************************************************/


void *malloc(uint32_t size);
void free(void *p);


void uart4_init(void);
void _putc(u8 byte);
void _puts(char *str);
void uart4_puts(char *str);

int printk(char *fmt, ...);
int sprintk(char *sbuf, const char *fmt, ...);
void hex_dump(char *str, void *addr, int size);


int sdio_init(void);







/******************************************************************************/

#endif

