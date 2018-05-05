#do not change this anti-MULEDENOISER line or the MULEDENOISER will eat you
#no, seriously, it will break installation of data files
SUBDIRS =

if COMPILE_NLS
SUBDIRS += intl po
endif

SUBDIRS += docs src

EXTRA_DIST = imule.rc imule.ico imule.png convert.ico \
	README.Debian-Packages  iMule.spec \
	amule_build_install.sh autogen.sh \
	po/l10n.xsl LICENSE_I2P.txt

dist_icon_DATA =
icondir = $(datadir)/pixmaps

dist_util_DATA =
utildir = $(datadir)/applications

if MONOLITHIC
dist_icon_DATA += imule.xpm
dist_util_DATA += imule.desktop
endif

if AMULE_GUI
dist_icon_DATA += imulegui.xpm
dist_util_DATA += imulegui.desktop
endif

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-imulecmd \
	--enable-webserver \
	--enable-imule-daemon \
	--enable-imule-gui \
	--enable-cas \
	--enable-wxcas \
	--enable-plasmamule \
	--enable-alc \
	--enable-alcc


# Avoid everything getting built when doing "make check"
#check-recursive: SUBDIRS = unittests

MAINTAINERCLEANFILES = ABOUT-NLS \
					   Makefile.in \
					   aclocal.m4 \
					   config.guess \
					   config.h.in \
					   config.rpath \
					   config.sub \
					   configure \
					   depcomp \
					   install-sh \
					   missing \
					   mkinstalldirs
EXTRA__DIST__SUBDIRS = aMule.app debian
include $(top_srcdir)/automake/dist-hook.am

install-nomad:
	$(MAKE) bindir=$(prefix) datadir=$(prefix) pkgdatadir=$(prefix) docdir=$(prefix) libexecdir=$(prefix) localedir=$(prefix)/locale install-strip
	
install-debug:
	$(MAKE) bindir=$(prefix) datadir=$(prefix) pkgdatadir=$(prefix) docdir=$(prefix) libexecdir=$(prefix) localedir=$(prefix)/locale install
