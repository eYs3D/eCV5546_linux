/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * eYs3D eCV5546 SoC framebuffer main driver
 *
 * linux/drivers/video/eys3d/fb_ecv5546/ecv5546fb_main.h
 *
 * Author: Hammer Hsieh <eys3d@eys3d.com>
 *
 */
#ifndef __FB_eCV5546_H__
#define __FB_eCV5546_H__

#define eCV5546_FB_RESERVED_MEM

#define eCV5546_FB_PALETTE_LEN	1024

#define eCV5546_DISP_ALIGN(x, n)	(((x) + ((n) - 1)) & ~((n) - 1))

struct ecv5546fb_device {
	/*
	 * for framebuffer alloc/register/release
	 */	
	struct fb_info	*fb;
	/*
	 * buf_size_page = width * height * bbp >> 3
	 * color_mode : support bpp = 8, 16, 32
	 */
	int		width;
	int		height;
	int		color_mode;
	char	color_mode_name[24];
	/*
	 * buf_size_total = buf_num * buf_size_page
	 */
	int		buf_num;
	int		buf_size_page;
	int		buf_size_total;
	/*
	 * buf_mem_total: physical address for data buffer
	 * pal : physical address for palette buffer
	 */
	void __iomem	*buf_mem_total;
	void __iomem	*pal;
	/*
	 * osd_handle : for ecv5546 display osd handle
	 */
	u32		osd_handle;
};

/* for ecv5546fb_debug.c */
struct ecv5546_bmp_header {
	/* unused define */
	unsigned short reserved0;

	/* bmp file header */
	unsigned short identifier;
	unsigned int file_size;
	unsigned int reserved1;
	unsigned int data_offset;

	/* bmp info header */
	unsigned int header_size;
	unsigned int width;
	unsigned int height;
	unsigned short planes;
	unsigned short bpp;
	unsigned int compression;
	
	/* unused define */
	unsigned short reserved;
};

/* for ecv5546fb_debug.c */
unsigned int ecv5546fb_chan_by_field(unsigned char chan,
	struct fb_bitfield *bf);
int ecv5546fb_swapbuf(u32 buf_id, int buf_max);

extern struct fb_info *gsp_fbinfo;

#endif	/* __FB_eCV5546_H__ */

