AUTOMAKE_OPTIONS = foreign

if DEBUG
AM_CFLAGS =$(DEPS_CFLAGS) $(CHECK_CFLAGS) -g
else
AM_CFLAGS = $(DEPS_CFLAGS) $(CHECK_CFLAGS)
#AM_CFLAGS = -fprofile-arcs -ftest-coverage $(DEPS_CFLAGS) $(CHECK_CFLAGS)
endif

noinst_PROGRAMS = persistence_client_library_test \
                  persistence_client_library_dbus_test \
                  persistence_client_library_benchmark \
                  persistence_admin_service_mockup \
                  persistence_lifeCycle_mockup

persistence_client_library_dbus_test_SOURCES = persistence_client_library_dbus_test.c
persistence_client_library_dbus_test_LDADD = $(DEPS_LIBS) \
   $(top_srcdir)/src/libpersistence_client_library.la

persistence_client_library_benchmark_SOURCES = persistence_client_library_benchmark.c
persistence_client_library_benchmark_LDADD = $(DEPS_LIBS) \
   $(top_srcdir)/src/libpersistence_client_library.la
   
persistence_client_library_test_SOURCES = persistence_client_library_test.c
persistence_client_library_test_LDADD = $(DEPS_LIBS) $(CHECK_LIBS) \
   $(top_srcdir)/src/libpersistence_client_library.la
   
persistence_admin_service_mockup_SOURCES = persistence_admin_service_mockup.c
persistence_admin_service_mockup_LDADD = $(DEPS_LIBS) 

persistence_lifeCycle_mockup_SOURCES = persistence_lifeCycle_mockup.c
persistence_lifeCycle_mockup_LDADD = $(DEPS_LIBS)     
  
TESTS=persistence_client_library_test
