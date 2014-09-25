/* sheet.c */

#include "bootpack.h"

#define SHEET_USE		1

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{
    struct SHTCTL *ctl;
    int i;
    ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof (struct SHTCTL));
    if (ctl == 0) 
    {
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1; /* 当前没有sheet */
    for (i = 0; i < MAX_SHEETS; i++) 
    {
        ctl->sheets0[i].flags = 0; /* 标记为未使用 */
    }
err:
    return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
    struct SHEET *sht;
    int i;
    for (i = 0; i < MAX_SHEETS; i++) 
    {
        if (ctl->sheets0[i].flags == 0) 
        {
            sht = &ctl->sheets0[i];
            sht->flags = SHEET_USE; /* 正在使用*/
            sht->height = -1; /* 隐藏之 */
            return sht;
        }
    }
    return 0;	/* 所有的sheet都在使用中 */
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv; //设定透明色
    return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, c, *vram = ctl->vram;
	struct SHEET *sht;
	for (h = 0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		/* 使用vx0 - vy1, 对 bx0 - by1 逆推 */
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				c = buf[by * sht->bxsize + bx];
				if (c != sht->col_inv) {
					vram[vy * ctl->xsize + vx] = c;
				}
			}
		}
	}
	return;
}

void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height)
{
    int h, old = sht->height; /* 储存设置前的高度信息 */

    /* 修正高度 */
    if (height > ctl->top + 1) 
    {
        height = ctl->top + 1;
    }
    if (height < -1) 
    {
        height = -1; // 图层将被隐藏
    }
    sht->height = height; /* 指定新高度 */

    /* 对sheets[]重新排列 */
    if (old > height) 
    {	/* 高度比以前低 */
        if (height >= 0) 
        {
            /* 把图层往上移 */
            for (h = old; h > height; h--) 
            {
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } 
        else 
        {	/* 隐藏 height = -1 即是删除? */
            if (ctl->top > old) 
            {
                /* 把上面的降下来  */
                for (h = old; h < ctl->top; h++) 
                {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--; /* 图层减少一个, 图层高度下降 */
        }
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
    } 
    else if (old < height) 
    {	/* 比以前高 */
        if (old >= 0) 
        {
            /* 其他图层下移 */
            for (h = old; h < height; h++) 
            {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } 
        else 
        {	/* 以前是隐藏的 old = -1; */
            /* 将图层上移 */
            for (h = ctl->top; h >= height; h--) 
            {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++; /* 图层增加一个 */
        }
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
    }
    return;
}

void sheet_refresh(struct SHTCTL *ctl, struct SHEET *sht, int bx0, int by0, int bx1, int by1)
{
	if (sht->height >= 0) { /* 图层正在显示, 刷新 */
		sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1);
	}
	return;
}

void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0)
{
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0)  /* 如果图层正在显示, 按新图层的信息刷新画面 */
      {
		sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
		sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
	}
	return;
}


/* 释放内存的函数 并没有看到free...*/
void sheet_free(struct SHTCTL *ctl, struct SHEET *sht)
{
    if (sht->height >= 0) 
    {
        sheet_updown(ctl, sht, -1); /* 将图层设为隐藏 */
    }
    sht->flags = 0; /* 图层改为未使用 */
    return;
}
