## Process this file with automake to produce Makefile.in

INCLUDES = $(INCLTDL) \
           -I$(top_builddir) \
           -I$(top_srcdir)/include

EXTRA_DIST = Version_script.in

AM_CFLAGS =

# Libraries to build
lib_LTLIBRARIES = libshveu.la

noinst_HEADERS = shveu_regs.h

libshveu_la_SOURCES = \
	veu_colorspace.c

libshveu_la_CFLAGS = -v -Wall -O2 -I $(srcdir) -fPIC -fno-common
libshveu_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@
libshveu_la_LIBADD =
