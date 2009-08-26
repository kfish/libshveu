/*
 * libshcodecs: A library for controlling SH-Mobile hardware codecs
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

/** \file
 * Image/Video processing: Scale, rotate, crop, color conversion
 */

#ifndef __VEU_COLORSPACE_H__
#define __VEU_COLORSPACE_H__

/** Rotation */
typedef enum {
	SHVEU_NO_ROT=0,	/**< No rotation */
	SHVEU_ROT_90,	/**< Rotate 90 degrees clockwise */
} E_VEU_ROTATION;

/** Image formats */
typedef enum {
	
	SHVEU_RGB565=0,	/**< RGB565 */
	SHVEU_YCbCr420,	/**< YCbCr 4:2:0 */
	SHVEU_YCbCr422,	/**< YCbCr 4:2:2 */
} E_VEU_FORMAT;

/** Start a (scale|rotate) & crop between YCbCr 4:2:0 & RG565 surfaces
 * \param veu_index Index of which VEU to use
 * \param src_py Pointer to Y or RGB plane of source image
 * \param src_pc Pointer to CbCr plane of source image (ignored for RGB)
 * \param src_width Width in pixels of source image
 * \param src_height Height in pixels of source image
 * \param src_pitch Line pitch of source image
 * \param src_format Format of source image
 * \param dst_py Pointer to Y or RGB plane of destination image
 * \param dst_pc Pointer to CbCr plane of destination image (ignored for RGB)
 * \param dst_width Width in pixels of destination image
 * \param dst_height Height in pixels of destination image
 * \param dst_pitch Line pitch of destination image
 * \param dst_fmt Format of destination image
 * \param rotate Rotation to apply
 * \retval 0 Success
 * \retval -1 Error: Attempt to perform simultaneous scaling and rotation
 */
int
shveu_start(
	unsigned int veu_index,
	unsigned char *src_py,
	unsigned char *src_pc,
	unsigned long src_width,
	unsigned long src_height,
	unsigned long src_pitch,
	E_VEU_FORMAT src_fmt,
	unsigned char *dst_py,
	unsigned char *dst_pc,
	unsigned long dst_width,
	unsigned long dst_height,
	unsigned long dst_pitch,
	E_VEU_FORMAT dst_fmt,
	E_VEU_ROTATION rotate);

/** Wait for a VEU operation to complete. The operation is started by a call to shveu_start.
 * \param veu_index Index of which VEU to use
 */
void
shveu_wait(
	unsigned int veu_index);

/** Perform (scale|rotate) & crop between YCbCr 4:2:0 & RG565 surfaces
 * \param veu_index Index of which VEU to use
 * \param src_py Pointer to Y or RGB plane of source image
 * \param src_pc Pointer to CbCr plane of source image (ignored for RGB)
 * \param src_width Width in pixels of source image
 * \param src_height Height in pixels of source image
 * \param src_pitch Line pitch of source image
 * \param src_format Format of source image
 * \param dst_py Pointer to Y or RGB plane of destination image
 * \param dst_pc Pointer to CbCr plane of destination image (ignored for RGB)
 * \param dst_width Width in pixels of destination image
 * \param dst_height Height in pixels of destination image
 * \param dst_pitch Line pitch of destination image
 * \param dst_fmt Format of destination image
 * \param rotate Rotation to apply
 * \retval 0 Success
 * \retval -1 Error: Attempt to perform simultaneous scaling and rotation
 */
int
shveu_operation(
	unsigned int veu_index,
	unsigned char *src_py,
	unsigned char *src_pc,
	unsigned long src_width,
	unsigned long src_height,
	unsigned long src_pitch,
	E_VEU_FORMAT src_fmt,
	unsigned char *dst_py,
	unsigned char *dst_pc,
	unsigned long dst_width,
	unsigned long dst_height,
	unsigned long dst_pitch,
	E_VEU_FORMAT dst_fmt,
	E_VEU_ROTATION rotate);

/** Perform scale from RG565 to YCbCr 4:2:0 surface
 * \param rgb565_in Pointer to input RGB565 image
 * \param y_out Pointer to output Y plane
 * \param c_out Pointer to output CbCr plane
 * \param width Width in pixels of image
 * \param height Height in pixels of image
 * \retval 0 Success
 */
int shveu_rgb565_to_nv12(
	unsigned char *rgb565_in,
	unsigned char *y_out,
	unsigned char *c_out,
	unsigned long width,
	unsigned long height);

/** Perform color conversion & crop from YCbCr 4:2:0 to RG565 surface
 * \param y_in Pointer to input Y plane
 * \param c_in Pointer to input CbCr plane
 * \param rgb565_out Pointer to output RGB565 image
 * \param width Width in pixels of image
 * \param height Height in pixels of image
 * \param pitch_in Line pitch of input image
 * \param pitch_out Line pitch of output image
 * \retval 0 Success
 */
int
shveu_nv12_to_rgb565(
	unsigned char *y_in,
	unsigned char *c_in,
	unsigned char *rgb565_out,
	unsigned long width,
	unsigned long height,
	unsigned long pitch_in,
	unsigned long pitch_out);


#endif				/* __VEU_COLORSPACE_H__ */
