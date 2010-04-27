## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_builddir) \
           -I$(top_srcdir)/include

SHVEUDIR = ../libshveu
SHVEU_LIBS = $(SHVEUDIR)/libshveu.la

if HAVE_NCURSES
ncurses_programs = shveu-display
endif

bin_PROGRAMS = shveu-convert $(ncurses_programs)

shveu_convert_SOURCES = shveu-convert.c
shveu_convert_LDADD = $(SHVEU_LIBS) $(UIOMUX_LIBS)

shveu_display_SOURCES = shveu-display.c display.c
shveu_display_LDADD = $(SHVEU_LIBS) $(UIOMUX_LIBS) -lncurses
