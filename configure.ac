dnl configure.in
dnl 
dnl   (C) Copyright 1996-2015 Fred Gleason <fredg@paravelsystems.com>
dnl
dnl   This program is free software; you can redistribute it and/or modify
dnl   it under the terms of the GNU General Public License as
dnl   published by the Free Software Foundation; either version 2 of
dnl   the License, or (at your option) any later version.
dnl
dnl   This program is distributed in the hope that it will be useful,
dnl   but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl   GNU General Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public
dnl   License along with this program; if not, write to the Free Software
dnl   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
dnl
dnl Use autoconf to process this into a configure script
dnl

AC_INIT(lab,m4_esyscmd(cat PACKAGE_VERSION))
AM_INIT_AUTOMAKE
AC_SUBST(RPM_RELEASE,1)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

#
# Some Fully Resolved Install Paths
# (for the packaging system and friends)
#
if test ${prefix} = "NONE" ; then
  AC_SUBST(LOCAL_PREFIX,/usr/local)
else
  AC_SUBST(LOCAL_PREFIX,${prefix})
fi

#
# Determine the target architecture
#
AR_GCC_TARGET()
AC_SUBST(VENDOR,$ar_gcc_distro)
AC_SUBST(ARCH,$ar_gcc_arch)

#
# Determine Distro
#
AR_GET_DISTRO()
if test $ar_gcc_distro = suse ; then
  AC_SUBST(APACHE_PKG,"apache2")
  AC_SUBST(APACHE_CONFIG_DIR,"/etc/apache2/conf.d")
  AC_SUBST(HTMLDIR,"/srv/www/htdocs")
  AC_SUBST(CGIDIR,"/srv/www/cgi-bin")
else
  AC_SUBST(APACHE_PKG,"httpd")
  AC_SUBST(APACHE_CONFIG_DIR,"/etc/httpd/conf.d")
  AC_SUBST(HTMLDIR,"/var/www/html")
  AC_SUBST(CGIDIR,"/var/www/cgi-bin")
fi

AC_PROG_CC

AC_CHECK_LIB(ncurses,wrefresh,[],
	[AC_MSG_ERROR([*** NCurses not found -- unable to continue ***])],[])

AC_CHECK_HEADER(linux/gpio.h,[AC_DEFINE(HAVE_GPIO)],[])

#
# Configure RPM Build
#
AC_CHECK_PROG(RPMBUILD_FOUND,rpmbuild,[yes],[])
if test -z $RPMBUILD_FOUND ; then
  AC_SUBST(RPMBUILD,rpm)
else
  AC_SUBST(RPMBUILD,rpmbuild)
fi
if test -d /usr/src/redhat ; then
  AC_SUBST(RPM_ROOT,/usr/src/redhat)
  AC_SUBST(VENDOR,redhat)
else
  AC_SUBST(RPM_ROOT,/usr/src/packages)
  AC_SUBST(VENDOR,suse)
fi 

AC_OUTPUT( \
	   Makefile \
           lab.spec \
           rc.labd \
           html/Makefile \
)

chmod 755 rc.labd
