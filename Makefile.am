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
	src/main.c

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
