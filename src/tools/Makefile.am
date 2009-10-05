## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_builddir) \
           -I$(top_srcdir)/include

SHVEUDIR = ../libshveu
SHVEU_LIBS = $(SHVEUDIR)/libshveu.la

bin_PROGRAMS = shveu-convert

shveu_convert_SOURCES = shveu-convert.c
shveu_convert_LDADD = $(SHVEU_LIBS) $(UIOMUX_LIBS)
