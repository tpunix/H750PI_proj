
#include "main.h"


/******************************************************************************/


#define BUF_SIZE 4096

static osMutexId_t umtx;
static u8 *qbuf = (u8*)0x30000000;
static u32 rp, wp, new_rp;


#define LOGSIZE 128
typedef struct {
	u16 rp;
	u16 wp;
	u16 ts;
	u8  rbuf[10];
}LOGBUF;

static LOGBUF logbuf[LOGSIZE];
static int logptr = 0;

static void dolog(u32 rp, u32 wp, int tsize)
{
	if(logptr<LOGSIZE){
		logbuf[logptr].rp = rp;
		logbuf[logptr].wp = wp;
		logbuf[logptr].ts = tsize;

		u8 *rbuf = logbuf[logptr].rbuf;
		int i, p;
		for(i=0; tsize && i<10; i++){
			p = rp+i-5;
			if(p<0)
				p += BUF_SIZE;
			if(p>=BUF_SIZE)
				p -= BUF_SIZE;
			rbuf[i] = qbuf[p];
		}
		logptr += 1;
	}
}

void showlog(void)
{
	int i, j, n;
	
	n = logptr;
	for(i=0; i<n; i++){
		printk("%3d: rp=%4d  wp=%4d  ts=%4d  ", i, logbuf[i].rp, logbuf[i].wp, logbuf[i].ts);
		for(j=0; j<10; j++){
			printk("%02x ", logbuf[i].rbuf[j]);
		}
		printk("\n");
	}

	logptr = 0;
}


static void uart4_dma_send(void *buf, int len)
{
	// CPU��write buffer����, �ᵼ������û�м�ʱд��RAM. DMA������һ�����������.
	// Ϊ�����������, �������__DMB/__DSBָ����ͬ��.
	__DMB();

	DMA1_Stream0->CR = 0;
	DMA1_Stream0->NDTR = len;
	DMA1_Stream0->PAR = (u32)&UART4->TDR;
	DMA1_Stream0->M0AR = (u32)buf;
	DMA1_Stream0->CR = 0x00000441;

	UART4->ICR = USART_ICR_TCCF;
	UART4->CR1 |= USART_CR1_TCIE;
	UART4->CR3 |= USART_CR3_DMAT;
}


static void uart4_send_start(void)
{
	int key = disable_irq();

	if(!(UART4->CR3&USART_CR3_DMAT)){
		// TODO: ÿ��ֻ���Ͳ�����BUF_SIZE/2������
		if(rp>wp){
			new_rp = 0;
			uart4_dma_send(qbuf+rp, BUF_SIZE-rp);
		}else if(rp<wp){
			new_rp = wp;
			uart4_dma_send(qbuf+rp, wp-rp);
		}
	}

	restore_irq(key);
}


void UART4_IRQHandler(void)
{
	// clear Stream0 TCIF
	DMA1->LIFCR = 0x0000007d;
	DMA1_Stream0->CR = 0;

	UART4->CR1 &= ~USART_CR1_TCIE;
	UART4->ICR = USART_ICR_TCCF;
	UART4->CR3 &= ~USART_CR3_DMAT;

	rp = new_rp;
	uart4_send_start();
}


static int get_free(void)
{
	int free;
	int key = disable_irq();

	if(wp>=rp){
		free = rp+BUF_SIZE-wp-1;
	}else{
		free = rp-wp-1;
	}

	restore_irq(key);
	
	return free;
}

/******************************************************************************/


static int uart4_put_buf(void *data, int length)
{
	int free, tail, retv;
	u8 *dptr = (u8*)data;

	if(!in_isr())
		osMutexAcquire(umtx, osWaitForever);

	free = get_free();
	if(length>free){
		retv = -1;
		goto _exit;
	}

	if(wp>=rp){
		tail = BUF_SIZE-wp;
		if(length>tail){
			memcpy(qbuf+wp, dptr, tail);
			wp = length-tail;
			memcpy(qbuf, dptr+tail, wp);
		}else{
			memcpy(qbuf+wp, dptr, length);
			wp += length;
			if(wp==BUF_SIZE)
				wp = 0;
		}
	}else{
		memcpy(qbuf+wp, dptr, length);
		wp += length;
	}

	uart4_send_start();
	retv = 0;

_exit:
	if(!in_isr())
		osMutexRelease(umtx);
	return retv;
}


void uart4_puts(char *str)
{
	int retv;
	int len = strlen(str);

	do{
		retv = uart4_put_buf(str, len);
		if(in_isr())
			break;
		if(retv){
			while(get_free()<(BUF_SIZE/2)){
				osDelay(10);
			};
		}
	}while(retv);
}


/******************************************************************************/


void uart4_init(void)
{
	rp = 0;
	wp = 0;

	umtx = osMutexNew(NULL);

	UART4->CR1  = 0x0;          // Reset USART1
	UART4->CR1  = 0x2000000c;   // FIFO TX RX
	UART4->CR2  = 0;
	UART4->CR3  = 0x64000000;
	UART4->BRR  = 0x0;          // Reset
	UART4->BRR  = 0x0364;       // ���ò��ر��ʼĴ���Ϊ115200(100M/115200)
	UART4->RQR  = 0x0018;
	UART4->CR1 |= 0x0001;       // Enable UART4

	DMAMUX1_Channel0->CCR = 64; // 63=UART4_RX  64=UART4_TX

	NVIC_EnableIRQ(UART4_IRQn);
}


void _putc(uint8_t byte)
{
	while((UART4->ISR & 0x80)==0);
	UART4->TDR = byte;
}


void _puts(char *str)
{
	char *p = str;

	for(p=str; *p; p++) {
		if (*p == '\n')
			_putc('\r');
		_putc(*p);
	}
}


/******************************************************************************/
