#! /bin/sh

# Global variables...
AUTOCONF="autoconf"
AUTOHEADER="autoheader"
AUTOM4TE="autom4te"
AUTOMAKE="automake"
ACLOCAL="aclocal"

# Please add higher versions first. The last version number is the minimum
# needed to compile KDE. Do not forget to include the name/version #
# separator if one is present, e.g. -1.2 where - is the separator.
KDE_AUTOCONF_VERS="-2.58 -2.57 257 -2.54 -2.53a -2.53 -2.52 -2.5x"
KDE_AUTOMAKE_VERS="-1.7 17 -1.6"

# We don't use variable here for remembering the type ... strings. Local 
# variables are not that portable, but we fear namespace issues with our
# includer.
checkAutoconf()
{
  for kde_autoconf_version in $KDE_AUTOCONF_VERS; do
    if test -x "`$WHICH $AUTOCONF$kde_autoconf_version 2>/dev/null`"; then
      AUTOCONF="`$WHICH $AUTOCONF$kde_autoconf_version`"
      AUTOHEADER="`$WHICH $AUTOHEADER$kde_autoconf_version`"
      AUTOM4TE="`$WHICH $AUTOM4TE$kde_autoconf_version`"
      break
  fi
  done
}

checkAutomake ()
{
  for kde_automake_version in $KDE_AUTOMAKE_VERS; do
    if test -x "`$WHICH $AUTOMAKE$kde_automake_version 2>/dev/null`"; then
      AUTOMAKE="`$WHICH $AUTOMAKE$kde_automake_version`"
      ACLOCAL="`$WHICH $ACLOCAL$kde_automake_version`"
      break
  fi
  done

  if test -n "$UNSERMAKE"; then 
     AUTOMAKE="$UNSERMAKE"
  fi
}

checkWhich ()
{
  WHICH=""
  for i in "type -p" "which" "type" ; do
    T=`$i sh 2> /dev/null`
    test -x "$T" && WHICH="$i" && break
  done
}

checkWhich
checkAutoconf
checkAutomake

export WHICH AUTOHEADER AUTOCONF AUTOM4TE AUTOMAKE ACLOCAL
