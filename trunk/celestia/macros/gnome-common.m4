# gnome-common.m4
# 
# This only for packages that are not in the GNOME CVS tree.

dnl GNOME_COMMON_INIT

AC_DEFUN([GNOME_COMMON_INIT],
[
	GNOME_ACLOCAL_DIR=`$ACLOCAL --print-ac-dir`/gnome
	AC_SUBST(GNOME_ACLOCAL_DIR)

	ACLOCAL="$ACLOCAL -I $GNOME_ACLOCAL_DIR"
])

