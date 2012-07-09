AUTOMAKE_OPTIONS = foreign 1.10
ACLOCAL_AMFLAGS = -I m4

MAINTAINERCLEANFILES = \
	config.log config.status \
	$(srcdir)/Makefile.in \
	$(srcdir)/config.h.in $(srcdir)/config.h.in~ $(srcdir)/configure \
	$(srcdir)/install-sh $(srcdir)/ltmain.sh $(srcdir)/missing \
	$(srcdir)/depcomp $(srcdir)/aclocal.m4 $(srcdir)/ylwrap \
	$(srcdir)/config.guess $(srcdir)/config.sub \
	$(srcdir)/m4/ltsugar.m4 $(srcdir)/m4/libtool.m4 \
	$(srcdir)/m4/ltversion.m4 $(srcdir)/m4/lt~obsolete.m4 \
	$(srcdir)/m4/ltoptions.m4
EXTRA_DIST = .gitignore

dist_doc_DATA = \
	README.markdown

pkgconfig_DATA = linenoise.pc

include_HEADERS = linenoise.h
lib_LTLIBRARIES = liblinenoise.la
noinst_PROGRAMS = linenoise_example

liblinenoise_la_SOURCES = linenoise.c linenoise.h linenoise.exports
liblinenoise_la_LDFLAGS = $(AM_LDFLAGS) \
	-no-undefined \
	-export-symbols $(srcdir)/linenoise.exports \
	-version-info @LINENOISE_LT_CURRENT@:@LINENOISE_LT_REVISION@:@LINENOISE_LT_AGE@

linenoise_example_SOURCES = example.c
linenoise_example_LDADD = liblinenoise.la

