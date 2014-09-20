/* mouse.c */

#include "bootpack.h"

struct FIFO8 mousefifo;

void inthandler2c(int *esp)
/*来自PS/2 鼠标的中断 */
{
	unsigned char data;
	io_out8(PIC1_OCW2, 0x64);	/* 通知PIC IRQ-12 的受理已经完成*/
	io_out8(PIC0_OCW2, 0x62);	/* 通知PIC IRQ-02 的受理已经完成*/
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&mousefifo, data);
	return;
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(struct MOUSE_DEC *mdec)
{
    /*激活鼠标*/
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);   //顺利的话ACK(0xfa)会被传送过来
    mdec->phase = 0;    //进入等待0xfa的阶段
	return; 
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0) {
		/* 等待鼠标的0xfa阶段*/
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		/* 等待鼠标第一字节的阶段 */
        if ((dat & 0xc8) == 0x08)   // why? P149
        {
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
		return 0;
	}
	if (mdec->phase == 2) {
		/* 等待鼠标第二字节的阶段 */
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3)
    {
		/* 等待鼠标第三字节的阶段 */
        mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;    //鼠标键的状态在低三位
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) 
        {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) 
        {
			mdec->y |= 0xffffff00;
		}
		mdec->y = - mdec->y;    // 鼠标y方向和画面符号相反
		return 1;
	}
	return -1; /* unreachable statment*/
}
