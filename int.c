/* int.c*/

#include "bootpack.h"
#include <stdio.h>

void init_pic(void)
/* PIC初始化*/
{
	io_out8(PIC0_IMR,  0xff  ); //禁止主PIC的所有中断
	io_out8(PIC1_IMR,  0xff  ); //禁止从PIC的所有中断

    // 主PIC设定
	io_out8(PIC0_ICW1, 0x11  ); //边沿触发模式 (edge trigger mode)
	io_out8(PIC0_ICW2, 0x20  ); //IRQ0~7 由 INT 20~27 接收
	io_out8(PIC0_ICW3, 1 << 2); //PIC1从PIC由IRQ2 连接
	io_out8(PIC0_ICW4, 0x01  ); //无缓冲区模式

    //从PIC设定
	io_out8(PIC1_ICW1, 0x11  ); //边沿触发模式 (edge trigger mode)      
	io_out8(PIC1_ICW2, 0x28  ); // IRQ0~7 由 INT 28~2f 接收
	io_out8(PIC1_ICW3, 2     ); //PIC1由IRQ2 连接
	io_out8(PIC1_ICW4, 0x01  ); //无缓冲区模式

	io_out8(PIC0_IMR,  0xfb  ); //11111011 PIC1 以外全部禁止
	io_out8(PIC1_IMR,  0xff  ); //11111111 禁止所有中断 

	return;
}

// 特殊机种的处理
void inthandler27(int *esp)
{
	io_out8(PIC0_OCW2, 0x67); 
	return;
}
