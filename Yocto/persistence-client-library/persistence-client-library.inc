##################################################
#  Project         Persistence Management - PCL
#  (c) copyright   2014
#  Company         XS Embedded GmbH
##################################################

inherit autotools pkgconfig

DESCRIPTION = "The Persistence Management is responsible to handle persistent data, including all data read and modified often during a lifetime of an infotainment system."
HOMEPAGE    = "http://projects.genivi.org/persistence-client-library"
LICENSE     = "MPL-2.0"
INC_PR      = "r3"
S           = "${WORKDIR}/git"

DEPENDS        = "dlt-daemon dbus libcheck persistence-common-object"
RDEPENDS_${PN} = "node-state-manager"

PARALLEL_MAKE = ""

SRC_URI = " \
   git://git.projects.genivi.org/persistence/persistence-client-library.git;protocol=git \
"

FILES_${PN}      = " \
   ${libdir}/*.so \
   ${libdir}/*.so.* \
   ${sysconfdir} \
"

do_install_append() {
   echo "do_install_append() > persistence-client-library.inc"
   install -d ${D}${sysconfdir}/dbus-1/system.d
   install -m 0644 ${S}/config/org.genivi.persistence.admin.conf ${D}${sysconfdir}/dbus-1/system.d
}
