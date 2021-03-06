dnl MPD_OPTIONAL_FUNC(name, func, macro)
dnl
dnl Allow the user to enable or disable the use of a function.  If the
dnl option is not specified, the function is auto-detected.
AC_DEFUN([MPD_OPTIONAL_FUNC], [
	AC_ARG_ENABLE([$1],
		AS_HELP_STRING([--enable-$1],
			[use the function "$1" (default: auto)]),
		[test xenable_$1 = xyes && AC_DEFINE([$3], 1, [Define to use $1])],
		[AC_CHECK_FUNC([$2],
			[AC_DEFINE([$3], 1, [Define to use $1])],)])
])
