/*
 * Tool to scale raw images for the display
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_NCURSES_SUBDIR
#include <ncurses/ncurses.h>
#else
#include <ncurses.h>
#endif
#include <uiomux/uiomux.h>
#include "shveu/shveu.h"
#include "display.h"

/* RGB565 colors */
#define BLACK 0x0000
#define RED   0xF800
#define GREEN 0x07E0
#define BLUE  0x001F


static void
usage (const char * progname)
{
	printf ("Usage: %s [options] [input-filename]\n", progname);
	printf ("Processes raw image data using the SH-Mobile VEU and displays on screen.\n");
	printf ("\n");
	printf ("If no input filename is specified, a simple image will be created.\n");
	printf ("\nInput options\n");
	printf ("  -c, --input-colorspace (RGB565, NV12, YCbCr420, YCbCr422)\n");
	printf ("                         Specify input colorspace\n");
	printf ("  -s, --input-size       Set the input image size (qcif, cif, qvga, vga, d1, 720p)\n");
	printf ("\nControl keys\n");
	printf ("  +/-                    Zoom in/out\n");
	printf ("  Cursor keys            Pan\n");
	printf ("  =                      Reset zoom and panning\n");
	printf ("  q                      Quit\n");
	printf ("\nMiscellaneous options\n");
	printf ("  -h, --help             Display this help and exit\n");
	printf ("  -v, --version          Output version information and exit\n");
	printf ("\nFile extensions are interpreted as follows unless otherwise specified:\n");
	printf ("  .yuv    YCbCr420\n");
	printf ("  .rgb    RGB565\n");
	printf ("\n");
	printf ("Please report bugs to <linux-sh@vger.kernel.org>\n");
}

void
print_short_options (char * optstring)
{
	char *c;

	for (c=optstring; *c; c++) {
		if (*c != ':') printf ("-%c ", *c);
	}

	printf ("\n");
}

#ifdef HAVE_GETOPT_LONG
void
print_options (struct option long_options[], char * optstring)
{
	int i;
	for (i=0; long_options[i].name != NULL; i++)  {
		printf ("--%s ", long_options[i].name);
	}

	print_short_options (optstring);
}
#endif

static int set_size (char * arg, int * w, int * h)
{
	if (arg) {
		if (!strncasecmp (arg, "qcif", 4)) {
			*w = 176;
			*h = 144;
		} else if (!strncasecmp (arg, "cif", 3)) {
			*w = 352;
			*h = 288;
		} else if (!strncasecmp (arg, "qvga", 4)) {
			*w = 320;
			*h = 240;
		} else if (!strncasecmp (arg, "vga", 3)) {
			*w = 640;
			*h = 480;
                } else if (!strncasecmp (arg, "d1", 2)) {
                        *w = 720;
                        *h = 480;
		} else if (!strncasecmp (arg, "720p", 4)) {
			*w = 1280;
			*h = 720;
		} else {
			return -1;
		}

		return 0;
	}

	return -1;
}

static const char * show_size (int w, int h)
{
	if (w == -1 && h == -1) {
		return "<Unknown size>";
	} else if (w == 176 && h == 144) {
		return "QCIF";
	} else if (w == 352 && h == 288) {
		return "CIF";
	} else if (w == 320 && h == 240) {
		return "QVGA";
	} else if (w == 640 && h == 480) {
		return "VGA";
	} else if (w == 720 && h == 480) {
		return "D1";
	} else if (w == 1280 && h == 720) {
		return "720p";
	}

	return "";
}

static int set_colorspace (char * arg, int * c)
{
	if (arg) {
		if (!strncasecmp (arg, "rgb565", 6) ||
		    !strncasecmp (arg, "rgb", 3)) {
			*c = V4L2_PIX_FMT_RGB565;
		} else if (!strncasecmp (arg, "YCbCr420", 8) ||
			   !strncasecmp (arg, "420", 3) ||
			   !strncasecmp (arg, "NV12", 4)) {
			*c = V4L2_PIX_FMT_NV12;
		} else if (!strncasecmp (arg, "YCbCr422", 8) ||
			   !strncasecmp (arg, "422", 3)) {
			*c = V4L2_PIX_FMT_NV16;
		} else {
			return -1;
		}

		return 0;
	}

	return -1;
}

static char * show_colorspace (int c)
{
	switch (c) {
	case V4L2_PIX_FMT_RGB565:
		return "RGB565";
	case V4L2_PIX_FMT_NV12:
		return "YCbCr420";
	case V4L2_PIX_FMT_NV16:
		return "YCbCr422";
	}

	return "<Unknown colorspace>";
}

static off_t filesize (char * filename)
{
	struct stat statbuf;

	if (filename == NULL || !strcmp (filename, "-"))
		return -1;

	if (stat (filename, &statbuf) == -1) {
		perror (filename);
		return -1;
	}

	return statbuf.st_size;
}

static off_t imgsize (int colorspace, int w, int h)
{
	int n=0, d=1;

	switch (colorspace) {
	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_NV16:
		/* 2 bytes per pixel */
		n=2; d=1;
	       	break;
       case V4L2_PIX_FMT_NV12:
		/* 3/2 bytes per pixel */
		n=3; d=2;
		break;
       default:
		return -1;
	}

	return (off_t)(w*h*n/d);
}

static int guess_colorspace (char * filename, int * c)
{
	char * ext;
	off_t size;

	if (filename == NULL || !strcmp (filename, "-"))
		return -1;

	/* If the colorspace is already set (eg. explicitly by user args)
	 * then don't try to guess */
	if (*c != -1) return -1;

	ext = strrchr (filename, '.');
	if (ext == NULL) return -1;

	if (!strncasecmp (ext, ".yuv", 4)) {
		*c = V4L2_PIX_FMT_NV12;
		return 0;
	} else if (!strncasecmp (ext, ".rgb", 4)) {
		*c = V4L2_PIX_FMT_RGB565;
		return 0;
	}

	return -1;
}

static int guess_size (char * filename, int colorspace, int * w, int * h)
{
	off_t size;
	int n=0, d=1;

	/* If the size is already set (eg. explicitly by user args)
	 * then don't try to guess */
	if (*w != -1 && *h != -1) return -1;

	if ((size = filesize (filename)) == -1) {
		return -1;
	}

	switch (colorspace) {
	case V4L2_PIX_FMT_RGB565:
	case V4L2_PIX_FMT_NV16:
		/* 2 bytes per pixel */
		n=2; d=1;
	       	break;
       case V4L2_PIX_FMT_NV12:
		/* 3/2 bytes per pixel */
		n=3; d=2;
		break;
       default:
		return -1;
	}

	if (*w==-1 && *h==-1) {
		/* Image size unspecified */
		if (size == 176*144*n/d) {
			*w = 176; *h = 144;
		} else if (size == 352*288*n/d) {
			*w = 352; *h = 288;
		} else if (size == 320*240*n/d) {
			*w = 320; *h = 240;
		} else if (size == 640*480*n/d) {
			*w = 640; *h = 480;
		} else if (size == 720*480*n/d) {
			*w = 720; *h = 480;
		} else if (size == 1280*720*n/d) {
			*w = 1280; *h = 720;
		} else {
			return -1;
		}
	} else if (*h != -1) {
		/* Height has been specified */
		*w = size * d / (*h * n);
	} else if (*w != -1) {
		/* Width has been specified */
		*h = size * d / (*w * n);
	}

	return 0;
}


static void draw_rect_rgb565(void *surface, uint16_t color, int x, int y, int w, int h, int span)
{
	uint16_t *pix = (uint16_t *)surface + y*span + x;
	int xi, yi;

	for (yi=0; yi<h; yi++) {
		for (xi=0; xi<w; xi++) {
			*pix++ = color;
		}
		pix += (span-w);
	}
}

static void scale(
	SHVEU *veu,
	DISPLAY *display,
	float scale,
	unsigned long py,
	unsigned long pc,
	unsigned long w,
	unsigned long h,
	unsigned long x,	/* Centre co-ordinates */
	unsigned long y,
	int src_fmt)
{
	unsigned char *bb_virt = display_get_back_buff_virt(display);
	unsigned long  bb_phys = display_get_back_buff_phys(display);
	int lcd_w = display_get_width(display);
	int lcd_h = display_get_height(display);
	int scaled_w = (int) (w * scale / 2.0);
	int scaled_h = (int) (h * scale / 2.0);
	int src_x1, src_y1, src_x2, src_y2;
	int dst_x1, dst_y1, dst_x2, dst_y2;
	float tmp;

	/* Clear the back buffer */
	draw_rect_rgb565(bb_virt, BLACK, 0, 0, lcd_w, lcd_h, lcd_w);

	src_x1 = 0;
	src_y1 = 0;
	src_x2 = w;
	src_y2 = h;

	dst_x1 = dst_x2 = lcd_w/2 + x;
	dst_y1 = dst_y2 = lcd_h/2 + y;

	dst_x1 -= scaled_w;
	dst_y1 -= scaled_h;
	dst_x2 += scaled_w;
	dst_y2 += scaled_h;

	/* Output is off screen? */
	if ((dst_x1 > lcd_w) || (dst_x2 < 0) || (dst_y1 > lcd_h) || (dst_y2 < 0))
		return;

	/* Crop source so that the output is on the lcd */
	if (dst_x1 < 0) {
		tmp = (-dst_x1) / (float)scaled_w;
		src_x1 = (w/2) * tmp;
		dst_x1 = 0;
	}
	if (dst_x2 > lcd_w) {
		tmp = (dst_x2 - lcd_w) / (float)scaled_w;
		src_x2 = w - ((w/2) * tmp);
		dst_x2 = lcd_w;
	}

	if (dst_y1 < 0) {
		tmp = (-dst_y1) / (float)scaled_h;
		src_y1 = (h/2) * tmp;
		dst_y1 = 0;
	}
	if (dst_y2 > lcd_h) {
		tmp = (dst_y2 - lcd_h) / (float)scaled_h;
		src_y2 = h - ((h/2) * tmp);
		dst_y2 = lcd_h;
	}

	/* Cropping */
	shveu_crop(veu, 0, src_x1, src_y1, src_x2, src_y2);
	shveu_crop(veu, 1, dst_x1, dst_y1, dst_x2, dst_y2);

	shveu_rescale(veu,
		py,      pc, w, h, src_fmt,
		bb_phys, 0,  lcd_w, lcd_h, V4L2_PIX_FMT_RGB565);

	display_flip(display);
}


int main (int argc, char * argv[])
{
	UIOMux *uiomux = NULL;
	SHVEU *veu = NULL;
	DISPLAY *display = NULL;
	char * infilename = NULL;
	FILE * infile = NULL;
	size_t nread;
	size_t input_size;
	int input_w = -1;
	int input_h = -1;
	int input_colorspace = -1;
	unsigned char *src_virt;
	unsigned long src_py, src_pc;
	int ret;
	float scale_factor=1.0;
	int read_image = 1;
	int x=0, y=0;
	int key;
	int run = 1;

	int show_version = 0;
	int show_help = 0;
	char * progname;
	int error = 0;

	int c;
	char * optstring = "hvc:s:";

#ifdef HAVE_GETOPT_LONG
	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'v'},
		{"input-colorspace", required_argument, 0, 'c'},
		{"input-size", required_argument, 0, 's'},
		{NULL,0,0,0}
	};
#endif

	progname = argv[0];

	if ((argc == 2) && !strncmp (argv[1], "-?", 2)) {
#ifdef HAVE_GETOPT_LONG
		print_options (long_options, optstring);
#else
		print_short_options (optstring);
#endif
		exit (0);
	}

	while (1) {
#ifdef HAVE_GETOPT_LONG
		c = getopt_long (argc, argv, optstring, long_options, NULL);
#else
		c = getopt (argc, argv, optstring);
#endif
		if (c == -1) break;
		if (c == ':') {
			usage (progname);
			goto exit_err;
		}

		switch (c) {
		case 'h': /* help */
			show_help = 1;
			break;
		case 'v': /* version */
			show_version = 1;
			break;
		case 'c': /* input colorspace */
			set_colorspace (optarg, &input_colorspace);
			break;
		case 's': /* input size */
			set_size (optarg, &input_w, &input_h);
			break;
		default:
			break;
		}
	}

	if (show_version) {
		printf ("%s version " VERSION "\n", progname);
	}
      
	if (show_help) {
		usage (progname);
	}
      
	if (show_version || show_help) {
		goto exit_ok;
	}

	if (optind > argc) {
		usage (progname);
		goto exit_err;
	}

	if (optind < argc) {
		infilename = argv[optind++];
		printf ("Input file: %s\n", infilename);

		guess_colorspace (infilename, &input_colorspace);
		guess_size (infilename, input_colorspace, &input_w, &input_h);
	} else {
		printf ("No input file specified, drawing simple image\n");
		if (input_w == -1) {
			input_w = 320;
			input_h = 240;
		}
		input_colorspace = V4L2_PIX_FMT_RGB565;
	}

	/* Check that all parameters are set */
	if (input_colorspace == -1) {
		fprintf (stderr, "ERROR: Input colorspace unspecified\n");
		error = 1;
	}
	if (input_w == -1) {
		fprintf (stderr, "ERROR: Input width unspecified\n");
		error = 1;
	}
	if (input_h == -1) {
		fprintf (stderr, "ERROR: Input height unspecified\n");
		error = 1;
	}

	if (error) goto exit_err;

	printf ("Input colorspace:\t%s\n", show_colorspace (input_colorspace));
	printf ("Input size:\t\t%dx%d %s\n", input_w, input_h, show_size (input_w, input_h));

	input_size = imgsize (input_colorspace, input_w, input_h);

	if (infilename != NULL) {
		infile = fopen (infilename, "rb");
		if (infile == NULL) {
			fprintf (stderr, "%s: unable to open input file %s\n",
				 progname, infilename);
			goto exit_err;
		}
	}

	if ((uiomux = uiomux_open()) == 0) {
		fprintf (stderr, "Error opening UIOmux\n");
		goto exit_err;
	}

	if ((veu = shveu_open()) == 0) {
		fprintf (stderr, "Error opening VEU\n");
		goto exit_err;
	}

	if ((display = display_open()) == 0) {
		fprintf (stderr, "Error opening display\n");
		goto exit_err;
	}


	/* Set up memory buffers */
	src_virt = uiomux_malloc (uiomux, UIOMUX_SH_VEU, input_size, 32);
	if (!src_virt) {
		perror("uiomux_malloc");
		goto exit_err;
	}
	src_py = uiomux_virt_to_phys (uiomux, UIOMUX_SH_VEU, src_virt);
	src_pc = src_py + (input_w * input_h);

	if (!infilename) {
		/* Draw a simple picture */
		draw_rect_rgb565(src_virt, BLACK, 0,         0,         input_w,   input_h,   input_w);
		draw_rect_rgb565(src_virt, BLUE,  input_w/4, input_h/4, input_w/4, input_h/2, input_w);
		draw_rect_rgb565(src_virt, RED,   input_w/2, input_h/4, input_w/4, input_h/2, input_w);
	}


	/* ncurses init */
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);

	while (run) {

		if (infilename && read_image) {
			read_image = 0;
			/* Read input */
			if ((nread = fread (src_virt, 1, input_size, infile)) != input_size) {
				if (nread == 0 && feof (infile)) {
					break;
				} else {
					fprintf (stderr, "%s: error reading input file %s\n",
						 progname, infilename);
				}
			}
		}

		scale (veu, display, scale_factor, src_py, src_pc, input_w, input_h, x, y, input_colorspace);

		key = getch();
		switch (key)
		{
		case '+':
			scale_factor += 0.01;
			break;
		case '-':
			scale_factor -= 0.01;
			break;
		case '=':
			scale_factor = 1.0;
			x = 0;
			y = 0;
			break;
		case KEY_UP:
			y -= 1;
			break;
		case KEY_DOWN:
			y += 1;
			break;
		case KEY_LEFT:
			x -= 1;
			break;
		case KEY_RIGHT:
			x += 1;
			break;
		case ' ':
			read_image = 1;
			break;
		case 'q':
			run = 0;
			break;
		}
	}


	/* ncurses close */
	clrtoeol();
	refresh();
	endwin();

	display_close(display);
	shveu_close(veu);
	uiomux_free (uiomux, UIOMUX_SH_VEU, src_virt, input_size);
	uiomux_close (uiomux);


exit_ok:
	exit (0);

exit_err:
	if (display) display_close(display);
	if (veu)     shveu_close(veu);
	if (uiomux)  uiomux_close (uiomux);
	exit (1);
}
