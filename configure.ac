AC_PREREQ(2.60)
AC_INIT(pold, 0.1.0)

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign 1.11 silent-rules
	tar-pax no-dist-gzip dist-xz subdir-objects color-tests])
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_CONFIG_MACRO_DIR([m4])
m4_ifndef([AM_SILENT_RULES], [m4_define([AM_SILENT_RULES],[])])
AM_SILENT_RULES([yes])

#####################################################################
# Program checks and configurations
#####################################################################

AC_PROG_CC
AC_PROG_CC_C99
AC_C_TYPEOF
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL
AC_C_BIGENDIAN

AC_PROG_SED
AC_PROG_MKDIR_P
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([DBUS], [dbus-1])
PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([LOG], [libsystemd-journal],
	[AC_DEFINE([HAVE_SYSTEMD_JOURNAL], [1], [Define to 1 if libsystemd-journal is installed])],
	[AC_DEFINE([HAVE_SYSTEMD_JOURNAL], [0], [Define to 1 if libsystemd-journal is installed])]
)
PKG_CHECK_MODULES(LIBSOUP, [libsoup-2.4 >= 2.44])
PKG_CHECK_MODULES(JANSSON, [jansson >= 2.4])

#####################################################################
# Function and structure checks
#####################################################################



#####################################################################
# --with-
#####################################################################

AC_ARG_WITH([systemdunitdir], AC_HELP_STRING([--with-systemdunitdir=DIR],
	[path to systemd system service directory]), [path_systemdunit=${withval}],
		[path_systemdunit="`$PKG_CONFIG --variable=systemdsystemunitdir systemd`"])
if (test -n "${path_systemdunit}"); then
	SYSTEMD_UNITDIR="${path_systemdunit}"
	AC_SUBST(SYSTEMD_UNITDIR)
fi
AM_CONDITIONAL(SYSTEMD, test -n "${path_systemdunit}")

AC_ARG_WITH(system_user, AS_HELP_STRING([--with-system-user=<user>],[User for running pold as a system-wide instance (pold)]))
if test -z "$with_system_user" ; then
    POLD_SYSTEM_USER=`whoami`
else
    POLD_SYSTEM_USER=$with_system_user
fi
AC_SUBST(POLD_SYSTEM_USER)
AC_DEFINE_UNQUOTED(POLD_SYSTEM_USER,"$POLD_SYSTEM_USER", [User for running pold])

#####################################################################
# --enable-
#####################################################################

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [enable debug messages @<:@default=disabled@:>@]),
	[], [enable_debug=no])
AS_IF([test "x$enable_debug" = "xyes"], [
	AC_DEFINE(ENABLE_DEBUG, [1], [Debug messages.])
])

#####################################################################
# Default CFLAGS and LDFLAGS
#####################################################################

CC_CHECK_FLAGS_APPEND(with_cflags, [CFLAGS], [\
		       -pipe \
		       -Wall \
		       -W \
		       -Wextra \
		       -Wno-inline \
		       -Wvla \
		       -Wundef \
		       -Wformat=2 \
		       -Wlogical-op \
		       -Wsign-compare \
		       -Wformat-security \
		       -Wmissing-include-dirs \
		       -Wformat-nonliteral \
		       -Wold-style-definition \
		       -Wpointer-arith \
		       -Winit-self \
		       -Wdeclaration-after-statement \
		       -Wfloat-equal \
		       -Wmissing-prototypes \
		       -Wredundant-decls \
		       -Wmissing-declarations \
		       -Wmissing-noreturn \
		       -Wendif-labels \
		       -Wstrict-aliasing=2 \
		       -Wwrite-strings \
		       -Wno-long-long \
		       -Wno-overlength-strings \
		       -Wno-unused-parameter \
		       -Wno-missing-field-initializers \
		       -Wno-unused-result \
		       -Wnested-externs \
		       -Wchar-subscripts \
		       -Wtype-limits \
		       -Wuninitialized \
		       -Wshadow \
		       -fno-common \
		       -fdiagnostics-show-option \
		       -fvisibility=hidden \
		       -ffunction-sections \
		       -fdata-sections])
AC_SUBST([WARNINGFLAGS], $with_cflags)


CC_CHECK_FLAGS_APPEND([with_ldflags], [LDFLAGS], [ \
		       -Wl,--as-needed \
		       -Wl,--gc-sections])
AC_SUBST([GCLDFLAGS], $with_ldflags)

#####################################################################
# Generate files from *.in
#####################################################################
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	Makefile src/pold.service src/pold.conf
	src/default.policy src/pold.policy
])

#####################################################################

AC_OUTPUT
AC_MSG_RESULT([
	$PACKAGE $VERSION
	======

	prefix:			${prefix}
	rootprefix:		${rootprefix}
	sysconfdir:		${sysconfdir}
	libdir:			${libdir}
	rootlibdir:		${rootlibdir}
	bindir:			${bindir}

	compiler:		${CC}
	cflags:			${with_cflags} ${CFLAGS}
	ldflags:		${with_ldflags} ${LDFLAGS}

	debug:			${enable_debug}

	system user:		${POLD_SYSTEM_USER}
])
