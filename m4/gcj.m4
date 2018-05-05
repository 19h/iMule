# Check for Java compiler.                                  -*- Autoconf -*-
# For now we only handle the GNU compiler.

# Copyright (C) 1999, 2000, 2003, 2005  Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([AM_PROG_GCJ],[
AC_CHECK_PROGS(GCJ, "${host}-gcj" "${host_cpu}-${host_os}-gcj" gcj gcj-3.2 gcj-3.1 gcj-3.0 gcj-2.95, none)
test -z "$GCJ" && AC_MSG_ERROR([no acceptable gcj found in \$PATH])
if test "x${GCJFLAGS-unset}" = xunset; then
   GCJFLAGS="-g -O2"
fi
AC_SUBST(GCJFLAGS)
_AM_IF_OPTION([no-dependencies],, [_AM_DEPENDENCIES(GCJ)])
])

AC_DEFUN([AM_PROG_GJAVAH],[
AC_CHECK_PROGS(GJAVAH, "${host}-gjavah" "${host_cpu}-${host_os}-gjavah" gjavah, none)
])
AC_DEFUN([AM_PROG_GJAR],[
AC_CHECK_PROGS(JAR, fastjar "${host}-gjar" "${host_cpu}-${host_os}-gjar" gjar, none)
])
