##################################################
#  Project         Persistence Management - PCL
#  (c) copyright   2014
#  Company         XS Embedded GmbH
##################################################

EXTRA_OECONF += "--enable-tests"

do_install_append() {
   echo "do_install_append() > persistence-client-library.bbappend"
   install -d ${D}/usr/bin
   install -m 0755 ${S}/test/.libs/persistence_client_library_dbus_test ${D}${bindir}
   install -m 0755 ${S}/test/.libs/persistence_client_library_test      ${D}${bindir}
   install -m 0755 ${S}/test/.libs/persistence_client_library_benchmark ${D}${bindir}
   install -d ${D}/Data
   install -m 0644 ${S}/test/data/Data.tar.gz                           ${D}/Data
}

PACKAGES += "${PN}-testenv"
RDEPENDS_${PN} += "${PN}-testenv"

FILES_${PN}-testenv = " \
   ${bindir}/persistence_client_library_dbus_test \
   ${bindir}/persistence_client_library_test \
   ${bindir}/persistence_client_library_benchmark \
   /Data/Data.tar.gz \
"

