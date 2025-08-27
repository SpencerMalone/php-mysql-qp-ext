#include "php.h"
#include "../include/php_mysql_qp.h"
#include "../include/mysql_query_parser.h"
#include <mysql.h>
#include <string.h>

/* Global MySQL connection for syntax-only parsing */
static MYSQL *syntax_mysql = NULL;
static int syntax_initialized = 0;

/* Initialize syntax-only parser connection */
int mysql_connect_syntax_parser(void) {
    if (syntax_initialized) {
        return SUCCESS;
    }
    
    syntax_mysql = mysql_init(NULL);
    if (syntax_mysql == NULL) {
        return FAILURE;
    }
    
    /* Connect without selecting a database for pure syntax checking */
    if (!mysql_real_connect(syntax_mysql, "localhost", "root", "", NULL, 0, NULL, 0)) {
        mysql_close(syntax_mysql);
        syntax_mysql = NULL;
        return FAILURE;
    }
    
    syntax_initialized = 1;
    return SUCCESS;
}

/* Cleanup syntax parser connection */
void mysql_disconnect_syntax_parser(void) {
    if (syntax_mysql) {
        mysql_close(syntax_mysql);
        syntax_mysql = NULL;
    }
    syntax_initialized = 0;
}

/* Validate query syntax only (ignoring table/data constraints) */
int mysql_validate_syntax_only(const char *query, size_t query_len) {
    MYSQL_STMT *stmt;
    int result = 0;
    unsigned int error_code;
    
    if (!syntax_initialized && mysql_connect_syntax_parser() != SUCCESS) {
        return 0;
    }
    
    stmt = mysql_stmt_init(syntax_mysql);
    if (!stmt) {
        return 0;
    }
    
    /* Try to prepare the statement */
    if (mysql_stmt_prepare(stmt, query, query_len) == 0) {
        result = 1;  /* Valid syntax */
    } else {
        error_code = mysql_stmt_errno(stmt);
        /* Only treat as invalid if it's a syntax error (1064) */
        /* Other errors like missing tables (1146) should be considered valid syntax */
        if (error_code == 1064) {
            result = 0;  /* Invalid syntax */
        } else {
            result = 1;  /* Valid syntax, but other issues (tables, etc.) */
        }
    }
    
    mysql_stmt_close(stmt);
    return result;
}