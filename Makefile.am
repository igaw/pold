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

EXTRA_DIST = \
	policyserver/policyserver.py \
	policyserver/someuser.policies

# This a big hack, if you know how to do better, please send patches
install-exec-hook:
	-chown @POLD_SYSTEM_USER@ $(DESTDIR)$(sbindir)/pold
	$(AM_V_at)$(MKDIR_P) $(DESTDIR)$(localstatedir)/lib/pold/policies
	-chown @POLD_SYSTEM_USER@ $(DESTDIR)$(localstatedir)/lib/pold
	-chown @POLD_SYSTEM_USER@ $(DESTDIR)$(localstatedir)/lib/pold/policies
	$(INSTALL_DATA) -o @POLD_SYSTEM_USER@ src/default.policy \
			$(DESTDIR)$(localstatedir)/lib/pold/default.policy
	$(INSTALL_DATA) -o @POLD_SYSTEM_USER@ src/pold.policy \
			$(DESTDIR)$(localstatedir)/lib/pold/pold.policy
	$(INSTALL_DATA) -D src/pold.conf \
			$(DESTDIR)$(sysconfdir)/dbus-1/system.d/pold.conf

uninstall-hook:
	rm -rf $(DESTDIR)$(localstatedir)/lib/pold
	rm $(DESTDIR)$(sysconfdir)/dbus-1/system.d/pold.conf

#
# Tests
#

check_PROGRAMS = \
	test/policy-test \
	test/dbus-json-test \
	test/http-client-test

test_policy_test_SOURCES = \
	src/log.h \
	src/log.c \
	test/policy-test.c \
	test/gdbus.h \
	test/gdbus.c

test_policy_test_CFLAGS = \
	$(AM_CFLAGS) \
	$(WARNINGFLAGS) \
	$(DBUS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(JANSSON_CFLAGS)

test_policy_test_LDADD = \
	$(GCLDFLAGS) \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(LOG_LIBS) \
	$(JANSSON_LIBS)

test_dbus_json_test_SOURCES = \
	src/dbus-json.h \
	src/dbus-json.c \
	test/dbus-json-test.c

test_dbus_json_test_CFLAGS = \
	$(AM_CFLAGS) \
	$(WARNINGFLAGS) \
	$(DBUS_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(JANSSON_CFLAGS)

test_dbus_json_test_LDADD = \
	$(GCLDFLAGS) \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(JANSSON_LIBS)

test_http_client_test_SOURCES = \
	test/http-client-test.c

test_http_client_test_CFLAGS = \
	$(AM_CFLAGS) \
	$(WARNINGFLAGS) \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(LIBSOUP_CFLAGS)

test_http_client_test_LDADD = \
	$(GCLDFLAGS) \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(LOG_LIBS) \
	$(LIBSOUP_LIBS)

TESTS = $(check_PROGRAMS)
