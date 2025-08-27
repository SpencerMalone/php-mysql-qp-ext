#ifndef MYSQL_QUERY_PARSER_H
#define MYSQL_QUERY_PARSER_H

#include <mysql.h>

/* Query parser result structure */
typedef struct {
    int is_valid;
    int query_type;
    char *error_message;
    int error_code;
    char *normalized_query;
    int parameter_count;
    char **parameter_names;
    zval *parse_tree;
} mysql_query_result;

/* Query types enum */
enum mysql_query_type {
    QUERY_TYPE_UNKNOWN = 0,
    QUERY_TYPE_SELECT = 1,
    QUERY_TYPE_INSERT = 2,
    QUERY_TYPE_UPDATE = 3,
    QUERY_TYPE_DELETE = 4,
    QUERY_TYPE_CREATE = 5,
    QUERY_TYPE_DROP = 6,
    QUERY_TYPE_ALTER = 7,
    QUERY_TYPE_SHOW = 8,
    QUERY_TYPE_DESCRIBE = 9,
    QUERY_TYPE_EXPLAIN = 10
};

/* Function declarations */
mysql_query_result* mysql_parse_query_real(const char *query, size_t query_len);
int mysql_validate_query_real(const char *query, size_t query_len);
char* mysql_build_query_real(zval *parse_tree);
void mysql_free_query_result(mysql_query_result *result);
int mysql_get_query_type(const char *query);
int mysql_connect_parser(void);
void mysql_disconnect_parser(void);
int mysql_connect_syntax_parser(void);
void mysql_disconnect_syntax_parser(void);
int mysql_validate_syntax_only(const char *query, size_t query_len);

#endif /* MYSQL_QUERY_PARSER_H */