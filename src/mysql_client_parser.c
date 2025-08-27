#include "php.h"
#include "../include/php_mysql_qp.h"
#include "../include/mysql_query_parser.h"
#include <mysql.h>
#include <string.h>

/* Global MySQL connection for parsing */
static MYSQL *parser_mysql = NULL;
static int parser_initialized = 0;

/* Initialize parser connection */
int mysql_connect_parser(void) {
    if (parser_initialized) {
        return SUCCESS;
    }
    
    parser_mysql = mysql_init(NULL);
    if (parser_mysql == NULL) {
        return FAILURE;
    }
    
    /* Connect to a local MySQL instance for parsing */
    /* Note: In production, this should be configurable */
    if (!mysql_real_connect(parser_mysql, "localhost", "root", "", "mysql_qp_test", 0, NULL, 0)) {
        mysql_close(parser_mysql);
        parser_mysql = NULL;
        return FAILURE;
    }
    
    parser_initialized = 1;
    return SUCCESS;
}

/* Cleanup parser connection */
void mysql_disconnect_parser(void) {
    if (parser_mysql) {
        mysql_close(parser_mysql);
        parser_mysql = NULL;
    }
    parser_initialized = 0;
}

/* Determine query type from SQL statement */
int mysql_get_query_type(const char *query) {
    if (!query) return QUERY_TYPE_UNKNOWN;
    
    /* Skip whitespace */
    while (*query && isspace(*query)) query++;
    
    if (strncasecmp(query, "SELECT", 6) == 0) return QUERY_TYPE_SELECT;
    if (strncasecmp(query, "INSERT", 6) == 0) return QUERY_TYPE_INSERT;
    if (strncasecmp(query, "UPDATE", 6) == 0) return QUERY_TYPE_UPDATE;
    if (strncasecmp(query, "DELETE", 6) == 0) return QUERY_TYPE_DELETE;
    if (strncasecmp(query, "CREATE", 6) == 0) return QUERY_TYPE_CREATE;
    if (strncasecmp(query, "DROP", 4) == 0) return QUERY_TYPE_DROP;
    if (strncasecmp(query, "ALTER", 5) == 0) return QUERY_TYPE_ALTER;
    if (strncasecmp(query, "SHOW", 4) == 0) return QUERY_TYPE_SHOW;
    if (strncasecmp(query, "DESCRIBE", 8) == 0) return QUERY_TYPE_DESCRIBE;
    if (strncasecmp(query, "EXPLAIN", 7) == 0) return QUERY_TYPE_EXPLAIN;
    
    return QUERY_TYPE_UNKNOWN;
}

/* Validate query using MySQL PREPARE */
int mysql_validate_query_real(const char *query, size_t query_len) {
    MYSQL_STMT *stmt;
    int result = 0;
    
    if (!parser_initialized && mysql_connect_parser() != SUCCESS) {
        return 0;
    }
    
    stmt = mysql_stmt_init(parser_mysql);
    if (!stmt) {
        return 0;
    }
    
    /* Try to prepare the statement - this validates the syntax */
    if (mysql_stmt_prepare(stmt, query, query_len) == 0) {
        result = 1;  /* Valid */
    }
    
    mysql_stmt_close(stmt);
    return result;
}

/* Parse query using MySQL PREPARE + EXPLAIN */
mysql_query_result* mysql_parse_query_real(const char *query, size_t query_len) {
    mysql_query_result *result;
    MYSQL_STMT *stmt;
    MYSQL_RES *res;
    char explain_query[4096];
    
    result = emalloc(sizeof(mysql_query_result));
    memset(result, 0, sizeof(mysql_query_result));
    
    /* Always determine query type first, regardless of validity */
    result->query_type = mysql_get_query_type(query);
    
    if (!parser_initialized && mysql_connect_parser() != SUCCESS) {
        result->is_valid = 0;
        result->error_message = estrdup("Could not connect to MySQL for parsing");
        return result;
    }
    
    /* Then validate with PREPARE */
    stmt = mysql_stmt_init(parser_mysql);
    if (!stmt) {
        result->is_valid = 0;
        result->error_message = estrdup("Could not create MySQL statement");
        return result;
    }
    
    if (mysql_stmt_prepare(stmt, query, query_len) != 0) {
        result->is_valid = 0;
        result->error_code = mysql_stmt_errno(stmt);
        result->error_message = estrdup(mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return result;
    }
    
    result->is_valid = 1;
    result->parameter_count = mysql_stmt_param_count(stmt);
    result->normalized_query = estrndup(query, query_len);
    
    mysql_stmt_close(stmt);
    
    /* For SELECT queries, get EXPLAIN information */
    if (result->query_type == QUERY_TYPE_SELECT) {
        snprintf(explain_query, sizeof(explain_query), "EXPLAIN FORMAT=JSON %.*s", (int)query_len, query);
        
        if (mysql_real_query(parser_mysql, explain_query, strlen(explain_query)) == 0) {
            res = mysql_store_result(parser_mysql);
            if (res) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row && row[0]) {
                    /* TODO: Parse JSON explain output into parse_tree zval */
                    /* For now, just store it as a string */
                }
                mysql_free_result(res);
            }
        }
    }
    
    return result;
}

/* Free query result structure */
void mysql_free_query_result(mysql_query_result *result) {
    if (!result) return;
    
    if (result->error_message) efree(result->error_message);
    if (result->normalized_query) efree(result->normalized_query);
    if (result->parameter_names) {
        for (int i = 0; i < result->parameter_count; i++) {
            if (result->parameter_names[i]) efree(result->parameter_names[i]);
        }
        efree(result->parameter_names);
    }
    if (result->parse_tree) {
        zval_ptr_dtor(result->parse_tree);
        efree(result->parse_tree);
    }
    
    efree(result);
}

/* Build query from parse tree (placeholder) */
char* mysql_build_query_real(zval *parse_tree) {
    /* TODO: Implement query reconstruction from parse tree */
    return estrdup("SELECT 1 /* reconstructed */");
}