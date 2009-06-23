## Process this file with automake to produce Makefile.in

DISTCHECK_CONFIGURE_FLAGS = --enable-gcc-werror

SUBDIRS = doc include src

# pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = shveu.pc

# Extra files to distribute in the source tarball.
EXTRA_DIST = shveu.pc.in shveu-uninstalled.pc.in
