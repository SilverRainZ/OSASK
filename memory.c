/*memory.c*/

#include "bootpack.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 检测CPU是386还是486 */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* 如果是386, 设定AC=1后,AC仍然会自动恢复为0*/
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* 禁止高速缓存 */
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /*  允许缓存 */
		store_cr0(cr0);
	}
	return i;
}

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/* 可用内存块的数目*/
	man->maxfrees = 0;		/* 观察可用状况, frees的最大值 */
	man->lostsize = 0;		/* 释放失败的内存大小总和 */
	man->losts = 0;			/* 释放失败次数 */
	return;
}

unsigned int memman_total(struct MEMMAN *man)
/* 报告空余内存大小的合计 */
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
/* 分配内存 */
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			/* 找到合适的内存 */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* 如果该条目为空, 减掉一条有用信息 */
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; /* 结构体赋值是可行的 */
				}
			}
			return a;   // 返回地址
		}
	}
	return 0; /* 没有可用空间 */
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* 释放内存 */
{
	int i, j;
	/* 将free按照addr排列 */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) 
    {
		/* 前面有可用内存 */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) 
        {
			/* 可以和前面的可用内存合并 */
			man->free[i - 1].size += size;
			if (i < man->frees) 
            {
				/* 可以与后面的内存合并 */
				if (addr + size == man->free[i].addr) 
                {
					/* 合并后面的 free[i]*/
					man->free[i - 1].size += man->free[i].size;
					man->frees--;
					for (; i < man->frees; i++) 
                    {
						man->free[i] = man->free[i + 1]; 
					}
				}
			}
			return 0; /* 成功释放 */
		}
	}
	/* 不可与前面的空间合并 */
	if (i < man->frees) 
    {
		/* 后面可以合并 */
		if (addr + size == man->free[i].addr) 
        {
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; 
		}
	}
	/* 无法和任何内存块合并 */
	if (man->frees < MEMMAN_FREES) {
		/* free[i] 向后移动 */
		for (j = man->frees; j > i; j--) 
        {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) 
        {
			man->maxfrees = man->frees; /* 更新最大值 */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; 
	}
	/* 无法移动  */
	man->losts++;
	man->lostsize += size;
	return -1; /* 失败 */
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
/* 以 4K(2^n)的数目为单位可以最大程度地利用and指令, 加快速度  */
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}
