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
dnl ##############################################################################
dnl
dnl      SVN Program:
dnl              $URL: http://trac.s.of.be.eurid.eu:80/svn/sysdevel/projects/yadifa/trunk/sbin/yadifad/configure.ac $
dnl
dnl      Creation Date:
dnl              2008-03-12
dnl
dnl      Last Update:
dnl              $Date: 2012-09-19 10:46:55 +0200 (Wed, 19 Sep 2012) $
dnl              $Revision: 2237 $
dnl
dnl      Author:
dnl             G. Van Emelen (Gery@VanEmelen.net)
dnl				Eric Diaz Fernandez
dnl
dnl      Purpose:
dnl      		 Yadifa
dnl				 Domain Name Server        
dnl
dnl      Last Change:
dnl              $Author: geryve $
dnl
dnl ##############################################################################

AC_PREREQ(2.61)
AC_REVISION([$Revision: 2237 $])
# 1
# svn log 2> /dev/null|head -2|grep ^r|sed 's/.\([1-9]*\).*/\1/'

# 2
#
# grep \$Revision: *|sed -e 's/^.*\$Revision: *//' -e 's/\$.*//' -e 's/ *//'|sort -n|tail -1| tr -d '\n')|tr -d '\n'

# 3
#
# grep -r \$Revision: ../../* |sed -e 's/^.*\$Revision: *//' -e 's/\$.*//' -e 's/ *//'|sort -n|tail -1| tr -d '\n')|tr -d '\n'


AC_INIT([yadifa],m4_esyscmd([echo $(cat VERSION)-$(grep \$Revision: * | sed -e 's/^.*\$Revision: *//' -e 's/\$.*//' -e 's/ *//' | sort -n | /usr/bin/tail -1 | tr -d '\n') | tr -d '\n']),[bugreport@yadifa.eu])

m4_include([../../m4/eurid.m4])
m4_include([../../m4/yadifa.m4])

AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([-Wall -Werror tar-pax])
AC_CONFIG_HEADER([server-config.h])

AC_CONFIG_MACRO_DIR([m4])

AC_DEFUN([SYSCONF], [/$prefix/PACKAGE/PACKAGE.conf])

AC_CANONICAL_BUILD

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

LT_INIT([disable-shared])

AC_DARWIN_LIBTOOL
AC_PROG_LIBTOOL

dnl Checks for libraries.
AC_CHECK_LIB([c], [printf])
AC_CHECK_LIB([pthread], [pthread_mutex_init])

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdlib.h string.h sys/time.h syslog.h unistd.h])
# Check for Darwin or Linux
AC_CHECK_HEADERS([linux/limits.h sys/syslimits.h i386/limits.h ppc/limits.h])
# Check for Darwin or Linux
AC_CHECK_HEADERS([sys/types.h i386/types.h])
#
AC_CHECK_HEADERS([sys/socket.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
dnl AC_PROG_RANLIB
AC_CHECK_TYPES(uint64_t)
AC_CHECK_TYPES(int64_t) 
AC_CHECK_TYPES(long long)
AC_CHECK_TYPES(u_char)

dnl Add some defines.

AC_DEFINE_UNQUOTED([TCP_LISTENQ], [1024],[TCP queue])
AC_DEFINE_UNQUOTED([MAX_INTERFACES], [5],[max nic interfaces])
AC_DEFINE_UNQUOTED([CLEARED_SOCKET], [-1],[cleared socket])

dnl Add some macros
AH_BOTTOM([
#ifdef  DEBUG
#define DPRINTF(p) printf p 
#else
#define DPRINTF(p) /* nothing */
#endif /* DEBUG */
])

AC_SYS_LARGEFILE

AC_CPU_CHECK
AC_COMPILER_CHECK
AC_MEMALIGN_CHECK
AC_PTHREAD_SPINLOCK_CHECK

AC_YADIFA_FEATURES

AC_YADIFA_ENABLE_DNSCORE
AC_YADIFA_ENABLE_DNSDB
AC_YADIFA_ENABLE_DNSZONE
AC_YADIFA_ADD_LIBS

AC_CHECK_LTO

dnl Checks for library functions.
AC_FUNC_FORK

dnl AC_FUNC_MALLOC does weird things:
dnl	http://sources.redhat.com/ml/automake/2003-05/msg00023.html
dnl	http://www.mail-archive.com/autoconf@gnu.org/msg08935.html
dnl AC_FUNC_MALLOC goals:
dnl malloc(0) will not return NULL
dnl realloc(NULL,x) will not crash (occurs on rare systems)
dnl
dnl AC_FUNC_MALLOC

AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([bzero memset select socket])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
echo
echo
echo
echo "Features used:"
echo "--------------"
case "$enable_tcl" in
	yes)
	echo "TCL ................................. : enabled"
	echo "TCLVERSION .......................... : ${TCLVERSION}"
	;;
	*)
	;;
esac

if [[ ! "${OPENSSL}" = "" ]]
then
	echo "Using OpenSLL from .................. : ${OPENSSL}"
fi

case "$enable_mirror" in
	yes)
	echo "MIRROR............................... : yes"
	;;
	*)
	;;
esac

echo
echo "Install prefix ...................... : $prefix"
echo "Install sysconf ..................... : $sysconfdir"
echo
echo

