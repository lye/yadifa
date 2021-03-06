dnl ############################################################################
dnl 
dnl Copyright (c) 2011, EURid. All rights reserved.
dnl The YADIFA TM software product is provided under the BSD 3-clause license:
dnl  
dnl Redistribution and use in source and binary forms, with or without 
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl 
dnl        * Redistributions of source code must retain the above copyright 
dnl          notice, this list of conditions and the following disclaimer.
dnl        * Redistributions in binary form must reproduce the above copyright
dnl          notice, this list of conditions and the following disclaimer in
dnl          the documentation and/or other materials provided with the 
dnl          distribution.
dnl        * Neither the name of EURid nor the names of its contributors may be
dnl          used to endorse or promote products derived from this software 
dnl          without specific prior written permission.
dnl 
dnl THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
dnl AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
dnl IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
dnl ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
dnl LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
dnl CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
dnl SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
dnl INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
dnl CONTRACT, STRICT LIABILITY,OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
dnl ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
dnl POSSIBILITY OF SUCH DAMAGE.
dnl 
dnl ############################################################################
dnl
dnl       SVN Program:
dnl               $URL: http://trac.s.of.be.eurid.eu:80/svn/sysdevel/projects/yadifa/trunk/configure.ac $
dnl
dnl       Last Update:
dnl               $Date: 2012-09-19 10:46:55 +0200 (Wed, 19 Sep 2012) $
dnl               $Revision: 2237 $
dnl
dnl       Purpose:
dnl              configure for YADIFA (domain name server)
dnl
dnl ############################################################################

AC_PREREQ(2.61)
AC_REVISION([$Revision: 2237 $])
AC_INIT([yadifa],m4_esyscmd([echo $(cat VERSION)-$(grep -r \$Revision: configure.ac Makefile.am sbin/* lib/* | sed -e 's/^.*\$Revision: *//' -e 's/\$.*//' -e 's/ *//' | sort -n | /usr/bin/tail -1 | tr -d '\n') | tr -d '\n']),[info@yadifa.eu])

m4_include([m4/eurid.m4])
m4_include([m4/yadifa.m4])

AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([-Wall -Werror tar-pax])

echo -n "enabled modules: "

AC_CONFIG_SUBDIRS([lib/dnscore lib/dnsdb lib/dnszone sbin/yadifad])

dnstools=0

AC_CONFIG_HEADERS([yadifa-config.h])

AC_CONFIG_MACRO_DIR([m4])

LT_INIT([disable-shared])

AC_DARWIN_LIBTOOL
AC_PROG_LIBTOOL

AC_DEFUN([SYSCONF], [/$prefix/PACKAGE/PACKAGE.conf])

AC_CANONICAL_BUILD

AC_CONFIG_FILES([Makefile etc/Makefile doc/Makefile var/Makefile])

AC_SYS_LARGEFILE

AC_CPU_CHECK
AC_COMPILER_CHECK
AC_MEMALIGN_CHECK
AC_PTHREAD_SPINLOCK_CHECK

AC_YADIFA_FEATURES

AC_CHECK_LTO

AC_EURID_SUMMARY

AC_YADIFA_SUMMARY

AC_OUTPUT

