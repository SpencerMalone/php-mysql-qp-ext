#ifndef PHP_MYSQL_QP_H
#define PHP_MYSQL_QP_H

extern zend_module_entry mysql_qp_module_entry;
#define phpext_mysql_qp_ptr &mysql_qp_module_entry

#define PHP_MYSQL_QP_VERSION "0.1.0"

#ifdef PHP_WIN32
#	define PHP_MYSQL_QP_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_MYSQL_QP_API __attribute__ ((visibility("default")))
#else
#	define PHP_MYSQL_QP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/* Function declarations */
PHP_MINIT_FUNCTION(mysql_qp);
PHP_MSHUTDOWN_FUNCTION(mysql_qp);
PHP_MINFO_FUNCTION(mysql_qp);

PHP_FUNCTION(mysql_parse_query);
PHP_FUNCTION(mysql_build_query);
PHP_FUNCTION(mysql_validate_query);
PHP_FUNCTION(mysql_decompose_query);
PHP_FUNCTION(mysql_reconstruct_query);

/* Module globals */
ZEND_BEGIN_MODULE_GLOBALS(mysql_qp)
	zend_bool initialized;
ZEND_END_MODULE_GLOBALS(mysql_qp)

#ifdef ZTS
#define MYSQL_QP_G(v) TSRMG(mysql_qp_globals_id, zend_mysql_qp_globals *, v)
#else
#define MYSQL_QP_G(v) (mysql_qp_globals.v)
#endif

#endif /* PHP_MYSQL_QP_H */