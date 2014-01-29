gentedatadir = $(pkgdatadir)
dist_gentedata_DATA = \
	backward \
	data.txt

desktopdir = $(datadir)/applications
desktop_DATA = gente.desktop

AM_CPPFLAGS = \
	$(GENTE_CFLAGS) \
	-DGENTE_DATA_DIR="\"$(pkgdatadir)\""

bin_PROGRAMS = gente

gente_SOURCES = \
	gente.c \
	cc-timezone-map.h \
	cc-timezone-map.c \
	tz.c \
	tz.h \
	timedated.c \
	timedated.h \
	gente-resources.c \
	gente-resources.h
gente_LDADD = \
	$(GENTE_LIBS) \
	-lm

# This requires running d-bus session and accessible timedate1 daemon
# FIXME: need to find a way how to filter out unnecessary d-bus stuff (introspectable, properties)
#timedated1-interface.xml:
#	gdbus introspect					\
#		--xml						\
#		--system					\
#		--dest org.freedesktop.timedate1		\
#		--object-path /org/freedesktop/timedate1	\
#		 > timedated1-interface.xml

dbus_built_sources = timedated.c timedated.h
timedated.c: timedated.h
timedated.h: Makefile.am timedated1-interface.xml
	gdbus-codegen \
		--interface-prefix org.freedesktop. \
		--generate-c-code timedated \
		$(srcdir)/timedated1-interface.xml


resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/gente.gresource.xml)
gente-resources.c: gente.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name gente $<
gente-resources.h: gente.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name gente $<

BUILT_SOURCES = \
	$(dbus_built_sources) \
	gente-resources.h \
	gente-resources.c

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	timedated1-interface.xml \
	$(resource_files) \
	gente.gresource.xml
