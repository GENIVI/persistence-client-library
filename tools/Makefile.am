AUTOMAKE_OPTIONS = foreign

if DEBUG
AM_CFLAGS =$(DEPS_CFLAGS) $(CHECK_CFLAGS) -g
else
AM_CFLAGS = $(DEPS_CFLAGS) $(CHECK_CFLAGS)
#AM_CFLAGS = -fprofile-arcs -ftest-coverage $(DEPS_CFLAGS) $(CHECK_CFLAGS)
endif

bin_PROGRAMS = persistence_client_tool \
                  persistence_db_viewer

persistence_client_tool_SOURCES = persistence_client_tool.c
persistence_client_tool_LDADD = $(DEPS_LIBS) \
   $(top_builddir)/src/libpersistence_client_library.la  

persistence_db_viewer_SOURCES = persistence_db_viewer.c
persistence_db_viewer_LDADD = $(DEPS_LIBS)      

