

BOTPAK	EQU	0x00280000	
DSKCAC	EQU	0x00100000	
DSKCAC0	EQU	0x00008000	


CYLS	EQU	0x0ff0	; 设定启动区	
LEDS	EQU	0x0ff1	
VMODE	EQU	0x0ff2	; 关于颜色数目的信息，颜色的位数	
SCRNX	EQU	0x0ff4	; 分辨率 X(Screen X)	
SCRNY	EQU	0x0ff6	; 分辨率 Y(Screen Y)	
VRAM	EQU	0x0ff8	; 图像缓冲区的起始地址	

	ORG	0xc200		



	MOV	AL,0x13	; VGA 显卡	
	MOV	AH,0x00
	INT	0x10
	MOV	BYTE [VMODE],8	; 记录画面模式	
	MOV	WORD [SCRNX],320
	MOV	WORD [SCRNY],200
	MOV	DWORD [VRAM],0x000a0000


; 取得键盘上的LED灯的状态
	MOV	AH,0x02
	INT	0x16 		
	MOV	[LEDS],AL

; PIC关闭一切中断
;  根据AT兼容机的规格, 如果要初始化PIC, 
;  必须在CLI之前进行, 否则有时会挂起,
;  随后进行PIC的初始化

	MOV	AL,0xff
	OUT	0x21,AL
	NOP			    ; 避免连续执行out指令
	OUT	0xa1,AL

	CLI			    ; 禁止CPU级别的中断 


; 为了让CPU能够访问1MB以上的内存, 设定A20Gate,  
	CALL	waitkbdout  ; waitkbout 的作用是等待命令执行完毕
	MOV	AL,0xd1
	OUT	0x64,AL
	CALL	waitkbdout
	MOV	AL,0xdf	        ; 打开A20地址线	
	OUT	0x60,AL
	CALL	waitkbdout



[INSTRSET "i486p"]	    ; 使用486指令集, 为了能使用LGDT,EAX,CR0	

	LGDT	[GDTR0]		; 临时GDT
	MOV	EAX,CR0
	AND	EAX,0x7fffffff  ; 设置bit为0(为了禁止"颁") What's is it!
	OR	EAX,0x00000001	; 设置bit为1, 为了切换到保护模式
	MOV	CR0,EAX         ; 进入32位模式!
	JMP	pipelineflush   ; 立刻跳转

pipelineflush:
	MOV	AX,1*8		    ; 可读写的段32bit
	MOV	DS,AX
	MOV	ES,AX
	MOV	FS,AX
	MOV	GS,AX
	MOV	SS,AX


	MOV	ESI,bootpack    ; 调用memcpy拷贝bootpack	
	MOV	EDI,BOTPAK	
	MOV	ECX,512*1024/4
	CALL	memcpy

; 磁盘数据最终传送到其本来的位置

; 从启动扇区开始

	MOV	ESI,0x7c00	
	MOV	EDI,DSKCAC	
	MOV	ECX,512/4
	CALL	memcpy

; 其他的数据传送

	MOV	ESI,DSKCAC0+512 ; 源地址
	MOV	EDI,DSKCAC+512	; 目的地址
	MOV	ECX,0
	MOV	CL,BYTE [CYLS]
	IMUL ECX,512*18*2/4 ; 柱面数变换为字节数/4
	SUB	ECX,512/4       ; 减去IPL	
	CALL	memcpy


; 必须由asmhead完成的工作, 至此全部完成,
; 以后便交由bootpack完成

; bootpack 的启动
    
	MOV	EBX,BOTPAK
	MOV	ECX,[EBX+16]
	ADD	ECX,3		
	SHR	ECX,2	        ; 右移2位 = 除以4	
	JZ	skip		    ; 没有要传送的东西的时候跳? 
	MOV	ESI,[EBX+20]	; 
	ADD	ESI,EBX
	MOV	EDI,[EBX+12]	; 目的地址
	CALL	memcpy
skip:
	MOV	ESP,[EBX+12]	
	JMP	DWORD 2*8:0x0000001b    ; 将2*8 代入CS, 同时移动到0x1b号地址

waitkbdout:             ; 等待函数
	IN	 AL,0x64
	AND	 AL,0x02        ; 此处比书中少了一句
  ; IN   AL,0xs60       ; 空读, 为了清空数据接收缓冲区的垃圾数据
	JNZ	waitkbdout	    ; AND结果如果不是0继续这个函数
	RET

memcpy:                 ; 等待函数
	MOV	EAX,[ESI]       
	ADD	ESI,4
	MOV	[EDI],EAX
	ADD	EDI,4
	SUB	ECX,1
	JNZ	memcpy	        ; 循环写入	
	RET


	ALIGNB	16          ; 填充DB 0 直到地址%16 = 0, 保证对齐
GDT0:
	RESB	8	                        ; 空选择子	
	DW	0xffff,0x0000,0x9200,0x00cf	    ; 可以读写的段
	DW	0xffff,0x0000,0x9a28,0x0047	    ; 可执行的段

	DW	0
GDTR0:
	DW	8*3-1
	DD	GDT0

	ALIGNB	16
bootpack:
