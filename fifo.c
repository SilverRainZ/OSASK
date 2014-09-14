/* FIFO*/

#include "bootpack.h"

#define FLAGS_OVERRUN		0x0001

void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf)
/* FIFO缓冲区初始化*/
{
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; /* 缓冲区大小*/
	fifo->flags = 0;
	fifo->p = 0; /* 队列尾*/
	fifo->q = 0; /* 队列头*/
	return;
}

int fifo8_put(struct FIFO8 *fifo, unsigned char data)
/* 入队*/
{
	if (fifo->free == 0) {
		/* 队列已满*/
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->p] = data;
	fifo->p++;
	if (fifo->p == fifo->size) {
		fifo->p = 0;
	}
	fifo->free--;
	return 0;
}

int fifo8_get(struct FIFO8 *fifo)
/* 出队*/
{
	int data;
	if (fifo->free == fifo->size) {
		/* 队列空 */
		return -1;
	}
	data = fifo->buf[fifo->q];
	fifo->q++;
	if (fifo->q == fifo->size) {
		fifo->q = 0;
	}
	fifo->free++;
	return data;
}

int fifo8_status(struct FIFO8 *fifo)
/* 取得状态*/
{
	return fifo->size - fifo->free;
}
