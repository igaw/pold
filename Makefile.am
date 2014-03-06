DISTCLEAN_LOCAL_HOOKS =
EXTRA_DIST =
CLEANFILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I$(srcdir)/gdbus
	-I$(top_srcdir)/src \
	-DROOTPREFIX=\""$(rootprefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\"

gdbus_sources = \
	gdbus/gdbus.h \
	gdbus/mainloop.c \
	gdbus/watch.c \
	gdbus/object.c \
	gdbus/polkit.c

pold_sources = \
	src/log.h \
	src/log.c \
	src/main.c \
	src/fdo-dbus.h \
	src/fdo-dbus.c \
	src/dbus-common.c \
	src/dbus-common.h \
	src/policy.h \
	src/policy.c \
	src/connman-notification.h \
	src/connman-notification.c \
	src/connman-manager.h \
	src/connman-manager.c \
	src/pold-manager.h \
	src/pold-manager.c \
	src/session.h \
	src/session.c \
	src/http-client.h \
	src/http-client.c \
	src/dbus-json.h \
	src/dbus-json.c \
	src/dbus.h

sbin_PROGRAMS = src/pold
src_pold_SOURCES = \
	$(pold_sources) \
	$(gdbus_sources)

src_pold_CFLAGS = \
	$(WARNINGFLAGS) \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(LOG_CFLAGS) \
	$(LIBSOUP_CFLAGS) \
	$(JANSSON_CFLAGS)

src_pold_CPPFLAGS = $(AM_CPPFLAGS) -DSTORAGEDIR='"$(localstatedir)/lib/pold"'

src_pold_LDADD = \
	$(GCLDFLAGS) \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(LOG_LIBS) \
	$(LIBSOUP_LIBS) \
	$(JANSSON_LIBS)

src_pold_SHORTNAME = pold
