SUBDIRS = ui dtd icons

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdevhelp-3.0.pc

assistantdir = $(datadir)/devhelp/assistant
assistant_DATA = 		\
	assistant.js		\
	assistant.css

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = devhelp.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

gsettings_SCHEMAS = org.gnome.Devhelp.gschema.xml
@GSETTINGS_RULES@

EXTRA_DIST =			\
	org.gnome.Devhelp.gschema.xml	\
	libdevhelp-3.0.pc.in	\
	$(desktop_in_files)	\
	$(assistant_DATA)

CLEANFILES =			\
	$(DESKTOP_FILES)	\
	$(gsettingsschema_DATA)	\
	$(defaults_DATA)

DISTCLEANFILES =		\
	$(desktop_DATA)
