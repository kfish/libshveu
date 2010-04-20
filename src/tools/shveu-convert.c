#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <uiomux/uiomux.h>

#include "shveu/shveu.h"

static int input_w = -1;
static int input_h = -1;
static int output_w = -1;
static int output_h = -1;
static int input_colorspace = -1;
static int output_colorspace = -1;

/* Rotation: default none */
static int rotation = SHVEU_NO_ROT;

static void
usage (const char * progname)
{
        printf ("Usage: %s [options] [input-filename [output-filename]]\n", progname);
        printf ("Convert raw image data using the SH-Mobile VEU.\n");
	printf ("\n");
        printf ("If no output filename is specified, data is output to stdout.\n");
        printf ("Specify '-' to force output to be written to stdout.\n");
	printf ("\n");
        printf ("If no input filename is specified, data is read from stdin.\n");
        printf ("Specify '-' to force input to be read from stdin.\n");
        printf ("\nInput options\n");
        printf ("  -c, --input-colorspace (RGB565, NV12, YCbCr420, YCbCr422)\n");
        printf ("                         Specify input colorspace\n");
        printf ("  -s, --input-size       Set the input image size (qcif, cif, qvga, vga)\n");
        printf ("\nOutput options\n");
        printf ("  -o filename, --output filename\n");
        printf ("                         Specify output filename (default: stdout)\n");
        printf ("  -C, --output-colorspace (RGB565, NV12, YCbCr420, YCbCr422)\n");
        printf ("                         Specify output colorspace\n");
        printf ("\nTransform options\n");
	printf ("  Note that the VEU does not support combined rotation and scaling.\n");
        printf ("  -S, --output-size      Set the output image size (qcif, cif, qvga, vga)\n");
	printf ("                         [default is same as input size, ie. no rescaling]\n");
        printf ("  -r, --rotate           Rotate the image 90 degrees clockwise\n");
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

int set_size (char * arg, int * w, int * h)
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
	}

	return "";
}

int set_colorspace (char * arg, int * c)
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

static char * show_rotation (int r)
{
	switch (r) {
	case SHVEU_NO_ROT:
		return "None";
	case SHVEU_ROT_90:
		return "90 degrees clockwise";
	}

	return "<Unknown rotation>";
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

int main (int argc, char * argv[])
{
	UIOMux * uiomux;

        char * infilename = NULL, * outfilename = NULL;
        FILE * infile, * outfile = NULL;
	size_t nread;
	size_t input_size, output_size;
	unsigned char * src_virt, * dest_virt;
	unsigned long src_py, src_pc, dest_py, dest_pc;
	void *veu;
	int ret;
	int frameno=0;

        int show_version = 0;
        int show_help = 0;
        char * progname;
	int error = 0;

        int c;
        char * optstring = "hvo:c:s:C:S:r";

#ifdef HAVE_GETOPT_LONG
        static struct option long_options[] = {
                {"help", no_argument, 0, 'h'},
                {"version", no_argument, 0, 'v'},
                {"output", required_argument, 0, 'o'},
                {"input-colorspace", required_argument, 0, 'c'},
                {"input-size", required_argument, 0, 's'},
                {"output-colorspace", required_argument, 0, 'C'},
                {"output-size", required_argument, 0, 'S'},
                {"rotate", no_argument, 0, 'r'},
                {NULL,0,0,0}
        };
#endif

        progname = argv[0];

        if (argc < 2) {
                usage (progname);
                return (1);
        }

        if (!strncmp (argv[1], "-?", 2)) {
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
                case 'o': /* output */
                        outfilename = optarg;
                        break;
                case 'c': /* input colorspace */
                        set_colorspace (optarg, &input_colorspace);
                        break;
                case 's': /* input size */
                        set_size (optarg, &input_w, &input_h);
                        break;
                case 'C': /* output colorspace */
                        set_colorspace (optarg, &output_colorspace);
                        break;
                case 'S': /* output size */
                        set_size (optarg, &output_w, &output_h);
                        break;
                case 'r': /* rotate */
                        rotation = SHVEU_ROT_90;
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

        if (optind >= argc) {
                usage (progname);
                goto exit_err;
        }

        infilename = argv[optind++];
	
	if (optind < argc) {
	        outfilename = argv[optind++];
	}

	printf ("Input file: %s\n", infilename);
	printf ("Output file: %s\n", outfilename);

	guess_colorspace (infilename, &input_colorspace);
	guess_colorspace (outfilename, &output_colorspace);
	/* If the output colorspace isn't given and can't be guessed, then default to
	 * the input colorspace (ie. no colorspace conversion) */
	if (output_colorspace == -1)
                output_colorspace = input_colorspace;

	guess_size (infilename, input_colorspace, &input_w, &input_h);
	/* If the output size isn't given and can't be guessed, then default to
	 * the input size (ie. no rescaling) */
	if (output_w == -1 && output_h == -1) {
		if (rotation == SHVEU_NO_ROT) {
                	output_w = input_w;
			output_h = input_h;
		} else {
			/* Swap width/height for rotation */
			output_w = input_h;
			output_h = input_w;
		}
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

	if (output_colorspace == -1) {
		fprintf (stderr, "ERROR: Output colorspace unspecified\n");
		error = 1;
	}
	if (output_w == -1) {
		fprintf (stderr, "ERROR: Output width unspecified\n");
		error = 1;
	}
	if (output_h == -1) {
		fprintf (stderr, "ERROR: Output height unspecified\n");
		error = 1;
	}

	if (error) goto exit_err;

	printf ("Input colorspace:\t%s\n", show_colorspace (input_colorspace));
	printf ("Input size:\t\t%dx%d %s\n", input_w, input_h, show_size (input_w, input_h));
	printf ("Output colorspace:\t%s\n", show_colorspace (output_colorspace));
	printf ("Output size:\t\t%dx%d %s\n", output_w, output_h, show_size (output_w, output_h));
	printf ("Rotation:\t\t%s\n", show_rotation (rotation));

	input_size = imgsize (input_colorspace, input_w, input_h);
	output_size = imgsize (output_colorspace, output_w, output_h);

	uiomux = uiomux_open ();

	/* Set up memory buffers */
	src_virt = uiomux_malloc (uiomux, UIOMUX_SH_VEU, input_size, 32);
	src_py = uiomux_virt_to_phys (uiomux, UIOMUX_SH_VEU, src_virt);
	if (input_colorspace == V4L2_PIX_FMT_RGB565) {
	        src_pc = 0;
	} else {
		src_pc = src_py + (input_w * input_h);
	}

	dest_virt = uiomux_malloc (uiomux, UIOMUX_SH_VEU, output_size, 32);
	dest_py = uiomux_virt_to_phys (uiomux, UIOMUX_SH_VEU, dest_virt);
	if (output_colorspace == V4L2_PIX_FMT_RGB565) {
	        dest_pc = 0;
	} else {
		dest_pc = dest_py + (output_w * output_h);
	}

        if (strcmp (infilename, "-") == 0) {
	        infile = stdin;
	} else {
	        infile = fopen (infilename, "rb");
		if (infile == NULL) {
                        fprintf (stderr, "%s: unable to open input file %s\n",
	                         progname, infilename);
                        goto exit_err;
		}
	}

	if (outfilename != NULL) {
                if (strcmp (outfilename, "-") == 0) {
                        outfile = stdout;
                } else {
                        outfile = fopen (outfilename, "wb");
                        if (outfile == NULL) {
                                fprintf (stderr, "%s: unable to open output file %s\n",
	                                 progname, outfilename);
                                goto exit_err;
                        }
                }
	}

        if ((veu = shveu_open ()) == 0) {
		fprintf (stderr, "Error opening VEU\n");
		goto exit_err;
	}

	while (1) {
#ifdef DEBUG
		fprintf (stderr, "%s: Converting frame %d\n", progname, frameno);
#endif

		/* Read input */
		if ((nread = fread (src_virt, 1, input_size, infile)) != input_size) {
			if (nread == 0 && feof (infile)) {
				break;
			} else {
				fprintf (stderr, "%s: error reading input file %s\n",
					 progname, infilename);
			}
		}

		ret = shveu_operation (veu, src_py, src_pc, input_w, input_h, input_w, input_colorspace,
				                  dest_py, dest_pc, output_w, output_h, output_w, output_colorspace,
					          rotation);

		if (ret == -1) {
			fprintf (stderr, "Illegal operation: cannot combine rotation and scaling\n");
			goto exit_err;
		}

		/* Write output */
		if (outfile && fwrite (dest_virt, 1, output_size, outfile) != output_size) {
				fprintf (stderr, "%s: error writing input file %s\n",
					 progname, outfilename);
		}

		frameno++;
	}

        shveu_close (veu);

	uiomux_free (uiomux, UIOMUX_SH_VEU, src_virt, input_size);
	uiomux_free (uiomux, UIOMUX_SH_VEU, dest_virt, output_size);
	uiomux_close (uiomux);

	if (infile != stdin) fclose (infile);

	if (outfile == stdout) {
		fflush (stdout);
	} else if (outfile) {
		fclose (outfile);
	}

	printf ("Frames:\t\t%d\n", frameno);

exit_ok:
        exit (0);

exit_err:
        exit (1);
}
