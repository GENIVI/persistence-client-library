AUTOMAKE_OPTIONS = foreign

if DEBUG
AM_CFLAGS = $(DEPS_CFLAGS) $(CHECK_CFLAGS) -g
#AM_CFLAGS = -fprofile-arcs -ftest-coverage  $(DEPS_CFLAGS) $(CHECK_CFLAGS) -g
else
AM_CFLAGS = $(DEPS_CFLAGS) $(CHECK_CFLAGS)
#AM_CFLAGS = -fprofile-arcs -ftest-coverage $(DEPS_CFLAGS) $(CHECK_CFLAGS)
endif

noinst_PROGRAMS = persistence_client_library_test \
                  persistence_client_library_dbus_test \
                  persistence_client_library_benchmark \
                  persistence_admin_service_mockup \
                  persistence_lifeCycle_mockup \
                  persistence_pfs_test

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

persistence_pfs_test_SOURCES = persistence_pfs_test.c
persistence_pfs_test_LDADD = $(DEPS_LIBS) \
   $(top_srcdir)/src/libpersistence_client_library.la

TESTS=persistence_client_library_test



lib_LTLIBRARIES = libearlyperscustom.la 
libearlyperscustom_la_SOURCES = persistence_test_customlib.c
libearlyperscustom_la_LDFLAGS = -avoid-version -module -shared -export-dynamic $(LDFLAGS)
libearlyperscustom_la_CFLAGS = -DLIBIDENT='"early"'

lib_LTLIBRARIES += libsecureperscustom.la 
libsecureperscustom_la_SOURCES = persistence_test_customlib.c
libsecureperscustom_la_LDFLAGS = -avoid-version -module -shared -export-dynamic $(LDFLAGS)
libsecureperscustom_la_CFLAGS = -DLIBIDENT='"secure"'


lib_LTLIBRARIES += libhwinfoperscustom.la
libhwinfoperscustom_la_SOURCES = persistence_test_customlib.c
libhwinfoperscustom_la_LDFLAGS = -avoid-version -module -shared -export-dynamic $(LDFLAGS)
libhwinfoperscustom_la_CFLAGS = -DLIBIDENT='"hwinfo"' 

lib_LTLIBRARIES += libcustom1perscustom.la 
libcustom1perscustom_la_SOURCES = persistence_test_customlib.c
libcustom1perscustom_la_LDFLAGS = -avoid-version -module -shared -export-dynamic $(LDFLAGS)
libcustom1perscustom_la_CFLAGS = -DLIBIDENT='"custom1"' 

lib_LTLIBRARIES += libemergencyperscustom.la 
libemergencyperscustom_la_SOURCES = persistence_test_customlib.c
libemergencyperscustom_la_LDFLAGS = -avoid-version -module -shared -export-dynamic $(LDFLAGS)
libemergencyperscustom_la_CFLAGS = -DLIBIDENT='"emergency"' 

lib_LTLIBRARIES += libcustom2perscustom.la 
libcustom2perscustom_la_SOURCES = persistence_test_customlib.c
libcustom2perscustom_la_LDFLAGS = -avoid-version -module -shared -export-dynamic $(LDFLAGS)
libcustom2perscustom_la_CFLAGS = -DLIBIDENT='"custom2"'

lib_LTLIBRARIES += libcustom3perscustom.la 
libcustom3perscustom_la_SOURCES = persistence_test_customlib.c
libcustom3perscustom_la_LDFLAGS = -avoid-version -module -shared -export-dynamic $(LDFLAGS) 
libcustom3perscustom_la_CFLAGS = -DLIBIDENT='"custom3"' 
