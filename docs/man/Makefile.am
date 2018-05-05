SELECTED_MANPAGES = \
	$(AMULECMD_MANPAGES) \
	$(WEBSERVER_MANPAGES) \
	$(AMULE_GUI_MANPAGES) \
	$(MONOLITHIC_MANPAGES) \
	$(AMULE_DAEMON_MANPAGES) \
	$(ED2K_MANPAGES)

EXTRA_DIST = \
	imule.1 imuled.1 imulecmd.1 imulegui.1 imuleweb.1 ed2k.1 \
	imule.de.1 imuled.de.1 imulecmd.de.1 imulegui.de.1 imuleweb.de.1 ed2k.de.1 \
	imule.es.1 imuled.es.1 imulecmd.es.1 imulegui.es.1 imuleweb.es.1 ed2k.es.1 \
	imule.fr.1 imuled.fr.1 imulecmd.fr.1 imulegui.fr.1 imuleweb.fr.1 ed2k.fr.1 \
	imule.hu.1 imuled.hu.1 imulecmd.hu.1 imulegui.hu.1 imuleweb.hu.1 ed2k.hu.1 \
	imule.it.1 imuled.it.1 imulecmd.it.1 imulegui.it.1 imuleweb.it.1 ed2k.it.1 \
	imule.ru.1 imuled.ru.1 imulecmd.ru.1 imulegui.ru.1 imuleweb.ru.1 ed2k.ru.1 \
	imule.tr.1 imuled.tr.1 imulecmd.tr.1 imulegui.tr.1 imuleweb.tr.1 ed2k.tr.1

EXTRA_DIST += po4a.config

EXTRA__DIST__SUBDIRS = po

include $(top_srcdir)/automake/dist-hook.am
include $(top_srcdir)/automake/manpages.am

MAINTAINERCLEANFILES = Makefile.in