
#ifndef __SHVEU_H__
#define __SHVEU_H__

#ifdef __cplusplus
extern "C" {
#endif

/** \mainpage
 *
 * \section intro SHVEU: A library for accessing the VEU.
 *
 * This is the documentation for the SHVEU C API.
 *
 * Features:
 *  - Simple interface to colorspace conversion, rotation, scaling
 * 
 * \subsection contents Contents
 * 
 * - \link shveu.h shveu.h \endlink, \link veu_colorspace.h veu_colorspace.h \endlink:
 * Documentation of the SHVEU C API
 *
 * - \link configuration Configuration \endlink:
 * Customizing libshveu
 *
 * - \link building Building \endlink:
 * 
 */

/** \defgroup configuration Configuration
 * \section configure ./configure
 *
 * It is possible to customize the functionality of libshveu
 * by using various ./configure flags when building it from source.
 *
 * For general information about using ./configure, see the file
 * \link install INSTALL \endlink
 *
 */

/** \defgroup install Installation
 * \section install INSTALL
 *
 * \include INSTALL
 */

/** \defgroup building Building against libshveu
 *
 *
 * \section autoconf Using GNU autoconf
 *
 * If you are using GNU autoconf, you do not need to call pkg-config
 * directly. Use the following macro to determine if libshveu is
 * available:
 *
 <pre>
 PKG_CHECK_MODULES(SHVEU, shveu >= 0.5.0,
                   HAVE_SHVEU="yes", HAVE_SHVEU="no")
 if test "x$HAVE_SHVEU" = "xyes" ; then
   AC_SUBST(SHVEU_CFLAGS)
   AC_SUBST(SHVEU_LIBS)
 fi
 </pre>
 *
 * If libshveu is found, HAVE_SHVEU will be set to "yes", and
 * the autoconf variables SHVEU_CFLAGS and SHVEU_LIBS will
 * be set appropriately.
 *
 * \section pkg-config Determining compiler options with pkg-config
 *
 * If you are not using GNU autoconf in your project, you can use the
 * pkg-config tool directly to determine the correct compiler options.
 *
 <pre>
 SHVEU_CFLAGS=`pkg-config --cflags shveu`

 SHVEU_LIBS=`pkg-config --libs shveu`
 </pre>
 *
 */

/** \file
 * The libshveu C API.
 *
 */

/**
 * Probe VEU devices.
 * \retval the number of found VEUs
 */
int shveu_probe(int force);

/**
 * Open a VEU device.
 * \retval 0 Success
 */
int shveu_open(unsigned int veu_index);

/**
 * Close a VEU device.
 */
void shveu_close(unsigned int veu_index);

#include <shveu/veu_colorspace.h>

#ifdef __cplusplus
}
#endif

#endif /* __SHVEU_H__ */

