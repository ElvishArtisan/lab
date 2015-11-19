## automake.am
##
## Automake.am for the Linux Audio Backstop
##
## by Fred Gleason <fredg@paravelsystems.com>
##
##   (C) Copyright 1996-2004 Fred Gleason <fredg@paravelsystems.com>
##
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 2 of
##   the License, or (at your option) any later version.
##
##   This program is distributed in the hope that it will be useful,
##   but WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
##
##   You should have received a copy of the GNU General Public
##   License along with this program; if not, write to the Free Software
##   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##
##
## Use automake to process this into a Makefile.in

SUBDIRS = html

AM_CFLAGS = -Wall
pkglibdir = /srv/www/cgi-bin

install-exec-local:	
	cp rc.labd /etc/init.d/labd
	cp wcatch.cgi $(pkglibdir)
	chmod 4755 $(pkglibdir)/wcatch.cgi

uninstall-local:
	rm -f /etc/init.d/labd
	rm -f $(pkglibdir)/wcatch.cgi

bin_PROGRAMS = lab
sbin_PROGRAMS = labd
noinst_PROGRAMS = wcatch.cgi
lab_SOURCES = lab.c curs_screen.c conflib.c wavlib.c\
              conflib.h lab.h wavlib.h
labd_SOURCES = labd.c traffic.c globals.c error.c common.c wavlib.c conflib.c\
               labd.h conflib.h wavlib.h
lab_LDADD = -lm -lncurses
labd_LDADD = -lm -lpthread
wcatch_cgi_SOURCES = wcatch.c common.c traffic.c globals.c cgilib.c conflib.c\
                     conflib.h cgilib.h
man_MANS = lab.1 lab.conf.5 lab.traffic.5 labd.8
EXTRA_DIST = $(man_MANS) autogen.sh rc.labd lab.spec.in lab.conf.example\
             lab.traffic.example get_target.sh get_distro.sh

rpm:	dist
	cp lab-$(VERSION).tar.gz $(HOME)/rpmbuild/SOURCES/
	$(RPMBUILD) -ba --target $(ARCH)-$(VENDOR)-linux lab.spec
	mv $(HOME)/rpmbuild/RPMS/$(ARCH)/lab-$(VERSION)-@RPM_RELEASE@.$(ARCH).rpm $(top_srcdir)/
	mv $(HOME)/rpmbuild/SRPMS/lab-$(VERSION)-@RPM_RELEASE@.src.rpm $(top_srcdir)/
	rm $(HOME)/rpmbuild/SOURCES/lab-$(VERSION).tar.gz
	rm -rf $(HOME)/rpmbuild/BUILD/lab-$(VERSION)

CLEANFILES = *~
DISTCLEANFILES = *.rpm\
                 *.tar.gz\
                 depcomp\
                 install-sh\
                 missing\
                 mkinstalldirs\
                 lab.spec\
                 rc.labd

MAINTAINERCLEANFILES = *~\
                       aclocal.m4\
                       configure\
                       Makefile.in
