dnl $Id$



AC_DEFUN([CASC_TYPE_NAMESPACE],[

# Start macro CASC_TYPE_NAMESPACE

AC_MSG_CHECKING(whether namespace is broken)

AC_CACHE_VAL(btng_cv_type_namespace_broken, [

  dnl AC_LANG_SAVE
  dnl AC_LANG_CPLUSPLUS
  AC_LANG_PUSH([C++])
  AC_TRY_COMPILE(namespace test{ int i; }
		, using namespace test;,
    # namespace is not broken.
    btng_cv_type_namespace_broken=no
    ,
    # namespace is broken.
    btng_cv_type_namespace_broken=yes
  )	dnl End AC_TRY_COMPILE call

  AC_LANG_POP([C++])
  dnl AC_LANG_RESTORE

])	dnl End AC_CACHE_VAL call

AC_MSG_RESULT($btng_cv_type_namespace_broken)

if test "$btng_cv_type_namespace_broken" = yes; then
  AC_DEFINE([NAMESPACE_IS_BROKEN],1,Define if namespace is not properly supported)
fi


# End macro CASC_TYPE_NAMESPACE

])	dnl End of CASC_TYPE_NAMESPACE definition.