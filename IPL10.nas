; hello-os
; TAB=4
; const
CYLS	EQU	10
	
	ORG	0x7c00	

	JMP	entry
	DB	0x90
	DB	"OS 0.01 "	
	DW	512	
	DB	1
	DW	1	
	DB	2
	DW	224	
	DW	2880	
	DB	0xf0	
	DW	9
	DW	18		
	DW	2		
	DD	0	
	DD	2880	
	DB	0,0,0x29	
	DD	0xffffffff	
	DB	"OS ver 0.01"	
	DB	"FAT12   "	
	RESB	18		
	
entry:
	MOV	AX,0
	MOV	SS,AX
	MOV	SP,0x7c00
	MOV	DS,AX
	
	MOV	AX,0x0820	; 缓存位置 = ES:BX
	MOV	ES,AX
	MOV	CH,0	; 柱面0
	MOV	DH,0	; 磁头0
	MOV	CL,2	; 扇区2

readloop:
	MOV	SI,0	; 记录失败次数
retry:
	MOV	AH,0x02 ; 读盘
	MOV	AL,1	; 一个扇区
	MOV	BX,0
	MOV	DL,0x00 ; 驱动器A:
	INT	0x13	; 调用磁盘中断
	JNC	next	
	ADD	SI,1
	CMP	SI,5	; 失败5次跳出
	JAE	error
	MOV	AH,0x00
	MOV	DL,0x00	; 指定驱动器A:
	INT	0x13	; 重置并重试
	JMP	retry	
next:
	MOV	AX,ES
	ADD	AX,0x0020
	MOV	ES,AX	; 段寄存器往后移0x0020 实际偏移 0x0020*0x0010 = 0x0200 = 512d
	ADD	CL,1
	CMP	CL,18	; 读到18扇区	
	JBE	readloop; <= 则跳
	MOV	CL,1
	ADD	DH,1	; 换一个磁头
	CMP	DH,2
	JB	readloop; <则跳
	MOV	DH,0
	ADD	CH,1	; 柱面+1
	CMP	CH,CYLS
	JB	readloop

	MOV	[0x0ff0],CH
	JMP	0xc200

error:			; 错误提示
	MOV	AX,0
	MOV	ES,AX
	MOV	AX,msg
	MOV	BP,AX	; ES:BP = 串地址
	MOV	CX,14	; 串长度
	MOV	AX,0x1301	;AH = 0x13，AL = 0x01
	MOV	BX,0x000c	; 页号BH = 0 黑底红字 BL = 0x0c
	MOV	DL,0
	INT	0x10
	
fin:
	HLT	
	JMP	fin

msg:
	DB	0x0a, 0x0a	
	DB	"Load error."
	DB	0x0a
	DB	0

	RESB	0x7dfe-$
	DB	0x55, 0xaa

