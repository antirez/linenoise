dnl -*- mode: m4; -*-

AC_PREREQ(2.60)

AC_INIT([linenoise],[0.0.4])

AC_CONFIG_AUX_DIR([.])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

LINENOISE_LT_CURRENT="0"
LINENOISE_LT_OLDEST="0"
LINENOISE_LT_REVISION="0"
LINENOISE_LT_AGE="$((${LINENOISE_LT_CURRENT}-${LINENOISE_LT_OLDEST}))"

AC_CONFIG_SRCDIR([./linenoise.h])

AC_ARG_ENABLE(
	[strict],
	[AS_HELP_STRING([--enable-strict],[enable strict compile mode @<:@disabled@:>@])],
	,
	[enable_strict="no"]
)

AC_ARG_ENABLE(
	[pedantic],
	[AS_HELP_STRING([--enable-pedantic],[enable pedantic compile mode @<:@disabled@:>@])],
	,
	[enable_pedantic="no"]
)

if test "${enable_pedantic}" = "yes"; then
	enable_strict="yes";
	CFLAGS="${CFLAGS} -pedantic"
fi
if test "${enable_strict}" = "yes"; then
	CFLAGS="${CFLAGS} -Wall -Wextra"
fi

AC_CANONICAL_HOST
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MKDIR_P
AC_PROG_SED
AC_PROG_MAKE_SET

#
# Libtool
#
ifdef(
	[LT_INIT],
	[LT_INIT],
	[AC_PROG_LIBTOOL]
)

AC_HEADER_STDC
AC_C_CONST
AC_TYPE_SIZE_T

#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

AC_CHECK_HEADERS(
	[ \
		termios.h unistd.h \
		sys/types.h sys/ioctl.h
	],
	,
	[AC_MSG_ERROR([Required header not found])]
)

#
# Output
#
pkgconfigdir="\$(libdir)/pkgconfig"

AC_SUBST([pkgconfigdir])
AC_SUBST([LINENOISE_LT_CURRENT])
AC_SUBST([LINENOISE_LT_REVISION])
AC_SUBST([LINENOISE_LT_AGE])
AC_SUBST([LINENOISE_LT_OLDEST])

AC_CONFIG_FILES([
	Makefile
	linenoise.pc
])
AC_OUTPUT
