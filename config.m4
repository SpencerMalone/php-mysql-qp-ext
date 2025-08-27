dnl config.m4 for extension mysql_qp

PHP_ARG_ENABLE([mysql-qp],
  [whether to enable mysql-qp support],
  [AS_HELP_STRING([--enable-mysql-qp],
    [Enable mysql-qp support])],
  [no])

if test "$PHP_MYSQL_QP" != "no"; then
  dnl Check for MySQL
  AC_MSG_CHECKING([for MySQL])
  
  dnl Use mysql_config if available
  AC_PATH_PROG(MYSQL_CONFIG, mysql_config, no)
  
  if test "$MYSQL_CONFIG" = "no"; then
    AC_MSG_ERROR([mysql_config not found. Please install MySQL development package])
  fi
  
  MYSQL_CFLAGS=`$MYSQL_CONFIG --cflags`
  MYSQL_LIBS=`$MYSQL_CONFIG --libs`
  MYSQL_VERSION=`$MYSQL_CONFIG --version`
  
  AC_MSG_RESULT([found MySQL $MYSQL_VERSION])
  
  PHP_EVAL_INCLINE($MYSQL_CFLAGS)
  PHP_EVAL_LIBLINE($MYSQL_LIBS, MYSQL_QP_SHARED_LIBADD)
  
  dnl Define extension
  AC_DEFINE(HAVE_MYSQL_QP, 1, [Whether you have MySQL Query Parser])
  
  dnl Add source files
  PHP_NEW_EXTENSION(mysql_qp, 
    src/mysql_qp.c src/query_parser.c src/php_bridge.c src/mysql_client_parser.c src/syntax_only_parser.c src/query_decomposer.c,
    $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
  
  PHP_SUBST(MYSQL_QP_SHARED_LIBADD)
fi