dnl Configury specific to the libfabric lnx provider

dnl Called to configure this provider
dnl
dnl Arguments:
dnl
dnl $1: action if configured successfully
dnl $2: action if not configured successfully
dnl
AC_DEFUN([FI_LNX_CONFIGURE],[
       # Determine if we can support the lnx provider
       lnx_happy=0
       AS_IF([test x"$enable_lnx" != x"no"], [lnx_happy=1])
       AS_IF([test $lnx_happy -eq 1], [$1], [$2])
])
