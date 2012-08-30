AUTOMAKE_OPTIONS = foreign

if DEBUG
AM_CFLAGS =$(DEPS_CFLAGS) $(CHECK_CFLAGS) -g
else
AM_CFLAGS =$(DEPS_CFLAGS) $(CHECK_CFLAGS)
#AM_CFLAGS = -fprofile-arcs -ftest-coverage $(DEPS_CFLAGS) $(CHECK_CFLAGS)
endif



noinst_PROGRAMS = persistence_client_library_test persistence_client_library_dbus_test
persistence_client_library_test_SOURCES = persistence_client_library_test.c
persistence_client_library_test_LDADD = $(DEPS_LIBS) $(CHECK_LIBS) \
   $(top_srcdir)/src/libpersistence_client_library.la


persistence_client_library_dbus_test_SOURCES = persistence_client_library_dbus_test.c
persistence_client_library_dbus_test_LDADD = $(DEPS_LIBS) $(top_srcdir)/src/libpersistence_client_library.la
   
TESTS=persistence_client_library_test
