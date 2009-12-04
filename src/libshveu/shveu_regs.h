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

#ifndef __SHVEU_REGS_H__
#define __SHVEU_REGS_H__

#define YCBCR_COMP_RANGE (0 << 16)
#define YCBCR_FULL_RANGE (1 << 16)
#define YCBCR_BT601      (0 << 17)
#define YCBCR_BT709      (1 << 17)

#define SH_VEU_RESERVE_TOP (512 << 10)
#define YUV_COLOR
#define CACHED_UV

#define VESTR 0x00		/* start register */
#define VESWR 0x10		/* src: line length */
#define VESSR 0x14		/* src: image size */
#define VSAYR 0x18		/* src: y/rgb plane address */
#define VSACR 0x1c		/* src: c plane address */
#define VBSSR 0x20		/* bundle mode register */
#define VEDWR 0x30		/* dst: line length */
#define VDAYR 0x34		/* dst: y/rgb plane address */
#define VDACR 0x38		/* dst: c plane address */
#define VTRCR 0x50		/* transform control */
#define VRFCR 0x54		/* resize scale */
#define VRFSR 0x58		/* resize clip */
#define VENHR 0x5c		/* enhance */
#define VFMCR 0x70		/* filter mode */
#define VVTCR 0x74		/* lowpass vertical */
#define VHTCR 0x78		/* lowpass horizontal */
#define VAPCR 0x80		/* color match */
#define VECCR 0x84		/* color replace */
#define VAFXR 0x90		/* fixed mode */
#define VSWPR 0x94		/* swap */
#define VEIER 0xa0		/* interrupt mask */
#define VEVTR 0xa4		/* interrupt event */
#define VSTAR 0xb0		/* status */
#define VBSRR 0xb4		/* reset */

#define VRPBR 0xc8		/* resize passband */

#define VMCR00 0x200		/* color conversion matrix coefficient 00 */
#define VMCR01 0x204		/* color conversion matrix coefficient 01 */
#define VMCR02 0x208		/* color conversion matrix coefficient 02 */
#define VMCR10 0x20c		/* color conversion matrix coefficient 10 */
#define VMCR11 0x210		/* color conversion matrix coefficient 11 */
#define VMCR12 0x214		/* color conversion matrix coefficient 12 */
#define VMCR20 0x218		/* color conversion matrix coefficient 20 */
#define VMCR21 0x21c		/* color conversion matrix coefficient 21 */
#define VMCR22 0x220		/* color conversion matrix coefficient 22 */
#define VCOFFR 0x224		/* color conversion offset */
#define VCBR   0x228		/* color conversion clip */

#define VTRCR_DST_FMT_YCBCR420 (0 << 22)
#define VTRCR_DST_FMT_YCBCR422 (1 << 22)
#define VTRCR_DST_FMT_YCBCR444 (2 << 22)
#define VTRCR_DST_FMT_RGB565   (6 << 16)
#define VTRCR_SRC_FMT_YCBCR420 (0 << 14)
#define VTRCR_SRC_FMT_YCBCR422 (1 << 14)
#define VTRCR_SRC_FMT_YCBCR444 (2 << 14)
#define VTRCR_SRC_FMT_RGB565   (3 << 8)
#define VTRCR_BT601            (0 << 3)
#define VTRCR_BT709            (1 << 3)
#define VTRCR_FULL_COLOR_CONV  (1 << 2)
#define VTRCR_TE_BIT_SET       (1 << 1)
#define VTRCR_RY_SRC_YCBCR     0
#define VTRCR_RY_SRC_RGB       1

#endif /* __SHVEU_REGS_H__ */
