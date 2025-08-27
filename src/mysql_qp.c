#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "../include/php_mysql_qp.h"
#include "../include/mysql_query_parser.h"
#include "../include/query_decomposer.h"

/* Module globals */
ZEND_DECLARE_MODULE_GLOBALS(mysql_qp)

/* Argument info for functions */
ZEND_BEGIN_ARG_INFO_EX(arginfo_mysql_parse_query, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, query, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysql_build_query, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, parse_tree, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysql_validate_query, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, query, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysql_decompose_query, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, query, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mysql_reconstruct_query, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, components, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

/* Function entries */
static const zend_function_entry mysql_qp_functions[] = {
	PHP_FE(mysql_parse_query, arginfo_mysql_parse_query)
	PHP_FE(mysql_build_query, arginfo_mysql_build_query)
	PHP_FE(mysql_validate_query, arginfo_mysql_validate_query)
	PHP_FE(mysql_decompose_query, arginfo_mysql_decompose_query)
	PHP_FE(mysql_reconstruct_query, arginfo_mysql_reconstruct_query)
	PHP_FE_END
};

/* Module entry */
zend_module_entry mysql_qp_module_entry = {
	STANDARD_MODULE_HEADER,
	"mysql_qp",
	mysql_qp_functions,
	PHP_MINIT(mysql_qp),
	PHP_MSHUTDOWN(mysql_qp),
	NULL,
	NULL,
	PHP_MINFO(mysql_qp),
	PHP_MYSQL_QP_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_MYSQL_QP
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(mysql_qp)
#endif

/* Module initialization */
PHP_MINIT_FUNCTION(mysql_qp)
{
	MYSQL_QP_G(initialized) = 1;
	if (mysql_connect_parser() != SUCCESS) {
		php_error_docref(NULL, E_WARNING, "Failed to initialize MySQL parser connection");
	}
	return SUCCESS;
}

/* Module shutdown */
PHP_MSHUTDOWN_FUNCTION(mysql_qp)
{
	mysql_disconnect_parser();
	MYSQL_QP_G(initialized) = 0;
	return SUCCESS;
}

/* Module info */
PHP_MINFO_FUNCTION(mysql_qp)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "MySQL Query Parser", "enabled");
	php_info_print_table_row(2, "Version", PHP_MYSQL_QP_VERSION);
	php_info_print_table_end();
}

/* Function implementations using real MySQL parser */
PHP_FUNCTION(mysql_parse_query)
{
	char *query;
	size_t query_len;
	mysql_query_result *result;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STRING(query, query_len)
	ZEND_PARSE_PARAMETERS_END();

	result = mysql_parse_query_real(query, query_len);
	
	array_init(return_value);
	add_assoc_bool(return_value, "is_valid", result->is_valid);
	add_assoc_long(return_value, "query_type", result->query_type);
	
	if (result->error_message) {
		add_assoc_string(return_value, "error", result->error_message);
		add_assoc_long(return_value, "error_code", result->error_code);
	}
	
	if (result->normalized_query) {
		add_assoc_string(return_value, "normalized_query", result->normalized_query);
	}
	
	add_assoc_long(return_value, "parameter_count", result->parameter_count);
	
	mysql_free_query_result(result);
}

PHP_FUNCTION(mysql_build_query)
{
	zval *parse_tree;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ARRAY(parse_tree)
	ZEND_PARSE_PARAMETERS_END();

	/* TODO: Implement query building from parse tree */
	RETURN_STRING("SELECT 1");
}

PHP_FUNCTION(mysql_validate_query)
{
	char *query;
	size_t query_len;
	int is_valid;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STRING(query, query_len)
	ZEND_PARSE_PARAMETERS_END();

	is_valid = mysql_validate_syntax_only(query, query_len);
	RETURN_BOOL(is_valid);
}

PHP_FUNCTION(mysql_decompose_query)
{
	char *query;
	size_t query_len;
	query_components *components;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STRING(query, query_len)
	ZEND_PARSE_PARAMETERS_END();

	components = mysql_decompose_query(query, query_len);
	
	array_init(return_value);
	add_assoc_string(return_value, "type", components->type ? components->type : "UNKNOWN");
	add_assoc_zval(return_value, "fields", components->fields);
	add_assoc_zval(return_value, "tables", components->tables);
	add_assoc_zval(return_value, "joins", components->joins);
	add_assoc_zval(return_value, "where_conditions", components->where_conditions);
	add_assoc_zval(return_value, "group_by", components->group_by);
	add_assoc_zval(return_value, "having", components->having);
	add_assoc_zval(return_value, "order_by", components->order_by);
	add_assoc_zval(return_value, "limit_clause", components->limit_clause);
	add_assoc_zval(return_value, "values", components->values);
	add_assoc_zval(return_value, "parameters", components->parameters);
	
	/* Clean up the components structure - PHP now owns the zval data */
	if (components->type) efree(components->type);
	
	/* Free the zval containers (PHP owns the array data now) */
	if (components->fields) efree(components->fields);
	if (components->tables) efree(components->tables);
	if (components->joins) efree(components->joins);
	if (components->where_conditions) efree(components->where_conditions);
	if (components->group_by) efree(components->group_by);
	if (components->having) efree(components->having);
	if (components->order_by) efree(components->order_by);
	if (components->limit_clause) efree(components->limit_clause);
	if (components->values) efree(components->values);
	if (components->parameters) efree(components->parameters);
	
	efree(components);
}

PHP_FUNCTION(mysql_reconstruct_query)
{
	zval *components_array;
	query_components *components;
	char *rebuilt_query;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ARRAY(components_array)
	ZEND_PARSE_PARAMETERS_END();

	/* Convert PHP array back to components structure */
	components = emalloc(sizeof(query_components));
	memset(components, 0, sizeof(query_components));
	
	zval *type = zend_hash_str_find(Z_ARRVAL_P(components_array), "type", 4);
	if (type && Z_TYPE_P(type) == IS_STRING) {
		components->type = estrdup(Z_STRVAL_P(type));
	}
	
	components->fields = zend_hash_str_find(Z_ARRVAL_P(components_array), "fields", 6);
	components->tables = zend_hash_str_find(Z_ARRVAL_P(components_array), "tables", 6);
	components->where_conditions = zend_hash_str_find(Z_ARRVAL_P(components_array), "where_conditions", 16);
	components->order_by = zend_hash_str_find(Z_ARRVAL_P(components_array), "order_by", 8);
	components->limit_clause = zend_hash_str_find(Z_ARRVAL_P(components_array), "limit_clause", 12);

	rebuilt_query = mysql_reconstruct_query(components);
	
	RETVAL_STRING(rebuilt_query);
	
	if (components->type) efree(components->type);
	efree(components);
	efree(rebuilt_query);
}