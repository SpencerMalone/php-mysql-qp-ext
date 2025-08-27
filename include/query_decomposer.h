#ifndef QUERY_DECOMPOSER_H
#define QUERY_DECOMPOSER_H

#include <zend.h>

/* Query component structure */
typedef struct {
    char *type;          /* SELECT, INSERT, UPDATE, DELETE, etc. */
    zval *fields;        /* SELECT fields or INSERT/UPDATE columns */
    zval *tables;        /* FROM/INTO tables with aliases */
    zval *joins;         /* JOIN clauses */
    zval *where_conditions;  /* WHERE clause components */
    zval *group_by;      /* GROUP BY fields */
    zval *having;        /* HAVING conditions */
    zval *order_by;      /* ORDER BY clauses */
    zval *limit_clause;  /* LIMIT/OFFSET */
    zval *values;        /* INSERT VALUES or UPDATE SET */
    zval *parameters;    /* Prepared statement parameters */
} query_components;

/* Function declarations */
query_components* mysql_decompose_query(const char *query, size_t query_len);
char* mysql_reconstruct_query(query_components *components);
void mysql_free_query_components(query_components *components);

/* Helper functions for specific query types */
int extract_select_components(const char *query, query_components *components);
int extract_insert_components(const char *query, query_components *components);
int extract_update_components(const char *query, query_components *components);
int extract_delete_components(const char *query, query_components *components);

/* Query builder functions */
char* build_select_query(query_components *components);
char* build_insert_query(query_components *components);
char* build_update_query(query_components *components);
char* build_delete_query(query_components *components);

/* Utility functions */
int parse_field_list(const char *fields_str, zval *fields_array);
int parse_table_list(const char *tables_str, zval *tables_array);
int parse_where_clause(const char *where_str, zval *where_array);

#endif /* QUERY_DECOMPOSER_H */