/*
 * libshveu: A library for controlling SH-Mobile VEU
 * Copyright (C) 2009 Renesas Technology Corp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */

/*
 * SuperH VEU color space conversion and stretching
 * Based on MPlayer Vidix driver by Magnus Damm
 * Modified by Takanari Hayama to support NV12->RGB565 conversion
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <pthread.h>
#include <semaphore.h>

#include <inttypes.h>
#include <unistd.h>
#include <errno.h>

#include <uiomux/uiomux.h>
#include "shveu/veu_colorspace.h"
#include "shveu_regs.h"

#define FMT_MASK (SHVEU_RGB565 | SHVEU_YCbCr420 | SHVEU_YCbCr422)

struct uio_map {
	unsigned long address;
	unsigned long size;
	void *iomem;
};


/* global variables */
struct uio_map sh_veu_uio_mmio, sh_veu_uio_mem;
UIOMux *uiomux;

/* Helper functions for reading registers. */

static unsigned long read_reg(struct uio_map *ump, int reg_nr)
{
	volatile unsigned long *reg = ump->iomem + reg_nr;

	return *reg;
}

static void write_reg(struct uio_map *ump, unsigned long value, int reg_nr)
{
	volatile unsigned long *reg = ump->iomem + reg_nr;

	*reg = value;
}

static int sh_veu_is_veu2h(void)
{
	return sh_veu_uio_mmio.size == 0x27c;
}

static int sh_veu_is_veu3f(void)
{
	return sh_veu_uio_mmio.size == 0xcc;
}

static void set_scale(struct uio_map *ump, int vertical,
		      int size_in, int size_out, int zoom)
{
	unsigned long fixpoint, mant, frac, value, vb;

	/* calculate FRAC and MANT */

	fixpoint = (4096 * (size_in - 1)) / (size_out + 1);
	mant = fixpoint / 4096;
	frac = fixpoint - (mant * 4096);

	if (frac & 0x07) {
		frac &= ~0x07;

		if (size_out > size_in)
			frac -= 8;	/* round down if scaling up */
		else
			frac += 8;	/* round up if scaling down */
	}

	/* Fix calculation for 1 to 1 scaling */
	if (size_in == size_out){
		mant = 0;
		frac = 0;
	}

	/* set scale */

	value = read_reg(ump, VRFCR);

	if (vertical) {
		value &= ~0xffff0000;
		value |= ((mant << 12) | frac) << 16;
	} else {
		value &= ~0xffff;
		value |= (mant << 12) | frac;
	}

	write_reg(ump, value, VRFCR);

	/* set clip */

	value = read_reg(ump, VRFSR);

	if (vertical) {
		value &= ~0xffff0000;
		value |= size_out << 16;
	} else {
		value &= ~0xffff;
		value |= size_out;
	}

	write_reg(ump, value, VRFSR);

	/* VEU3F needs additional VRPBR register handling */
#ifdef KERNEL2_6_33
	if (sh_veu_is_veu3f()) {
#endif
	    if (zoom)
	        vb = 64;
	    else {
	        if ((mant >= 8) && (mant < 16))
	            value = 4;
	        else if ((mant >= 4) && (mant < 8))
	            value = 2;
	        else
	            value = 1;

	        vb = 64 * 4096 * value;
	        vb /= 4096 * mant + frac;
	    }

	    /* set resize passband register */
	    value = read_reg(ump, VRPBR);
	    if (vertical) {
	        value &= ~0xffff0000;
	        value |= vb << 16;
	    } else {
	        value &= ~0xffff;
	        value |= vb;
	    }
	    write_reg(ump, value, VRPBR);
#ifdef KERNEL2_6_33
	}
#endif
}

static int sh_veu_probe(int verbose, int force)
{
	int ret;

	uiomux = uiomux_open();
	if (!uiomux)
		return errno;

	ret = uiomux_get_mmio (uiomux, UIOMUX_SH_VEU,
		&sh_veu_uio_mmio.address,
		&sh_veu_uio_mmio.size,
		&sh_veu_uio_mmio.iomem);
	if (!ret)
		return errno;

	ret = uiomux_get_mem (uiomux, UIOMUX_SH_VEU,
		&sh_veu_uio_mem.address,
		&sh_veu_uio_mem.size,
		&sh_veu_uio_mem.iomem);
	if (!ret)
		return errno;

	return 0;
}

static int sh_veu_init(void)
{
	/* reset VEU */
	write_reg(&sh_veu_uio_mmio, 0x100, VBSRR);
	return 0;
}

int shveu_open(void)
{
	int ret=0;

	ret = sh_veu_probe(0, 0);
	if (ret < 0)
		return ret;

	return 0;
}

void shveu_close(void)
{
	uiomux_close(uiomux);
	uiomux = NULL;
}

int
shveu_start(
	unsigned int veu_index,
	unsigned long src_py,
	unsigned long src_pc,
	unsigned long src_width,
	unsigned long src_height,
	unsigned long src_pitch,
	shveu_format_t src_fmt,
	unsigned long dst_py,
	unsigned long dst_pc,
	unsigned long dst_width,
	unsigned long dst_height,
	unsigned long dst_pitch,
	shveu_format_t dst_fmt,
	shveu_rotation_t rotate)
{
	/* Ignore veu_index as we only support one VEU at the moment */
	struct uio_map *ump = &sh_veu_uio_mmio;

#ifdef DEBUG
	fprintf(stderr, "%s IN\n", __FUNCTION__);
	fprintf(stderr, "src_fmt=%X: src_width=%d, src_height=%d src_pitch=%d\n",
		src_fmt, src_width, src_height, src_pitch);
	fprintf(stderr, "dst_fmt=%x: dst_width=%d, dst_height=%d dst_pitch=%d\n",
		dst_fmt, dst_width, dst_height, dst_pitch);
	fprintf(stderr, "rotate=%d\n", rotate);
#endif

	/* Rotate can't be performed at the same time as a scale! */
	if (rotate && (src_width != dst_height))
		return -1;
	if (rotate && (dst_width != src_height))
		return -1;

	if ((src_fmt != SHVEU_YCbCr420) &&
	    (src_fmt != SHVEU_YCbCr422) &&
	    (src_fmt != SHVEU_RGB565))
		return -1;
	if ((dst_fmt != SHVEU_YCbCr420) &&
	    (dst_fmt != SHVEU_YCbCr422) &&
	    (dst_fmt != SHVEU_RGB565))
		return -1;

	/* VESWR/VEDWR restrictions */
	if ((src_pitch % 2) || (dst_pitch % 2))
		return -1;

	/* VESSR restrictions */
	if ((src_height < 16) || (src_height > 4092) ||
	    (src_width  < 16) || (src_width  > 4092))
		return -1;

	/* Scaling limits */
	if (sh_veu_is_veu2h()) {
		if ((dst_width > 8*src_width) || (dst_height > 8*src_height))
			return -1;
	} else {
		if ((dst_width > 16*src_width) || (dst_height > 16*src_height))
			return -1;
	}
	if ((dst_width < src_width/16) || (dst_height < src_height/16))
		return -1;

	uiomux_lock (uiomux, UIOMUX_SH_VEU);

	/* reset */
	sh_veu_init();

	/* source */
	write_reg(ump, (unsigned long)src_py, VSAYR);
	write_reg(ump, (unsigned long)src_pc, VSACR);

	write_reg(ump, (src_height << 16) | src_width, VESSR);

	if (src_fmt == SHVEU_RGB565)
		src_pitch *= 2;
	write_reg(ump, src_pitch, VESWR);
	write_reg(ump, 0, VBSSR);	/* not using bundle mode */


	/* dest */
	if (rotate) {
		int src_vblk  = (src_height+15)/16;
		int src_sidev = (src_height+15)%16 + 1;
		int dst_density = 2;	/* for RGB565 and YCbCr422 */
		int offset;

		if ((dst_fmt & FMT_MASK) == SHVEU_YCbCr420)
			dst_density = 1;
		offset = ((src_vblk-2)*16 + src_sidev) * dst_density;

		write_reg(ump, (unsigned long)dst_py + offset, VDAYR);
		write_reg(ump, (unsigned long)dst_pc + offset, VDACR);
	} else {
		write_reg(ump, (unsigned long)dst_py, VDAYR);
		write_reg(ump, (unsigned long)dst_pc, VDACR);
	}

	if (dst_fmt == SHVEU_RGB565)
		dst_pitch *= 2;
	write_reg(ump, dst_pitch, VEDWR);

	/* byte/word swapping */
	{
		unsigned long vswpr = 0;
		if (src_fmt == SHVEU_RGB565)
			vswpr |= 0x6;
		else
			vswpr |= 0x7;
		if (dst_fmt == SHVEU_RGB565)
			vswpr |= 0x60;
		else
			vswpr |= 0x70;
		write_reg(ump, vswpr, VSWPR);
#if DEBUG
		fprintf(stderr, "vswpr=0x%X\n", vswpr);
#endif
	}


	/* transform control */
	{
		unsigned long vtrcr = 0;
		if ((src_fmt & FMT_MASK) == SHVEU_RGB565) {
			vtrcr |= VTRCR_RY_SRC_RGB;
			vtrcr |= VTRCR_SRC_FMT_RGB565;
		} else {
			vtrcr |= VTRCR_RY_SRC_YCBCR;
			if ((src_fmt & FMT_MASK) == SHVEU_YCbCr420)
				vtrcr |= VTRCR_SRC_FMT_YCBCR420;
			else
				vtrcr |= VTRCR_SRC_FMT_YCBCR422;
		}

		if ((dst_fmt & FMT_MASK) == SHVEU_RGB565) {
			vtrcr |= VTRCR_DST_FMT_RGB565;
		} else {
			if ((dst_fmt & FMT_MASK) == SHVEU_YCbCr420)
				vtrcr |= VTRCR_DST_FMT_YCBCR420;
			else
				vtrcr |= VTRCR_DST_FMT_YCBCR422;
		}

		if ((src_fmt & FMT_MASK) != (dst_fmt & FMT_MASK)) {
			vtrcr |= VTRCR_TE_BIT_SET;
			if ((src_fmt & YCBCR_FULL_RANGE) || (dst_fmt & YCBCR_FULL_RANGE))
				vtrcr |= VTRCR_FULL_COLOR_CONV;
			if ((src_fmt & YCBCR_BT709) || (dst_fmt & YCBCR_BT709))
				vtrcr |= VTRCR_BT709;
		}
		write_reg(ump, vtrcr, VTRCR);
#if DEBUG
		fprintf(stderr, "vtrcr=0x%X\n", vtrcr);
#endif
	}

	/* Is this a VEU2H on SH7723? */
	if (ump->size > VBSRR) {
		/* color conversion matrix */
		write_reg(ump, 0x0cc5, VMCR00);
		write_reg(ump, 0x0950, VMCR01);
		write_reg(ump, 0x0000, VMCR02);
		write_reg(ump, 0x397f, VMCR10);
		write_reg(ump, 0x0950, VMCR11);
		write_reg(ump, 0x3cdd, VMCR12);
		write_reg(ump, 0x0000, VMCR20);
		write_reg(ump, 0x0950, VMCR21);
		write_reg(ump, 0x1023, VMCR22);
		write_reg(ump, 0x00800010, VCOFFR);
	}

        if ((dst_width*dst_height) > (src_width*src_height)) {
                set_scale(ump, 0, src_width,  dst_width, 1);
                set_scale(ump, 1, src_height, dst_height, 1);
        }else{
                set_scale(ump, 0, src_width,  dst_width, 0);
                set_scale(ump, 1, src_height, dst_height, 0);
        }

	if (rotate) {
		write_reg(ump, 1, VFMCR);
		write_reg(ump, 0, VRFCR);
	}
	else {
		write_reg(ump, 0, VFMCR);
	}

	/* enable interrupt in VEU */
	write_reg(ump, 1, VEIER);

	/* start operation */
	write_reg(ump, 1, VESTR);

#ifdef DEBUG
	fprintf(stderr, "%s OUT\n", __FUNCTION__);
#endif

	return 0;
}

void
shveu_wait(unsigned int veu_index)
{
	uiomux_sleep(uiomux, UIOMUX_SH_VEU);
	write_reg(&sh_veu_uio_mmio, 0x100, VEVTR);   /* ack int, write 0 to bit 0 */
	uiomux_unlock(uiomux, UIOMUX_SH_VEU);
}

int
shveu_operation(
	unsigned int veu_index,
	unsigned long src_py,
	unsigned long src_pc,
	unsigned long src_width,
	unsigned long src_height,
	unsigned long src_pitch,
	shveu_format_t src_fmt,
	unsigned long dst_py,
	unsigned long dst_pc,
	unsigned long dst_width,
	unsigned long dst_height,
	unsigned long dst_pitch,
	shveu_format_t dst_fmt,
	shveu_rotation_t rotate)
{
	int ret = 0;

	ret = shveu_start(
		veu_index,
		src_py, src_pc, src_width, src_height, src_pitch, src_fmt,
		dst_py, dst_pc, dst_width, dst_height, dst_pitch, dst_fmt,
		rotate);

	if (ret == 0)
		shveu_wait(veu_index);

	return ret;
}



int
shveu_rgb565_to_nv12 (
	unsigned long rgb565_in,
	unsigned long y_out,
	unsigned long c_out,
	unsigned long width,
	unsigned long height)
{
	return shveu_operation(
		0,
		rgb565_in, 0,  width, height, width, SHVEU_RGB565,
		y_out,     c_out, width, height, width, SHVEU_YCbCr420,
		0);
}

int
shveu_nv12_to_rgb565(
	unsigned long y_in,
	unsigned long c_in,
	unsigned long rgb565_out,
	unsigned long width,
	unsigned long height,
	unsigned long pitch_in,
	unsigned long pitch_out)
{
	return shveu_operation(
		0,
		y_in,       c_in, width, height, pitch_in,  SHVEU_YCbCr420,
		rgb565_out, 0, width, height, pitch_out, SHVEU_RGB565,
		0);
}

