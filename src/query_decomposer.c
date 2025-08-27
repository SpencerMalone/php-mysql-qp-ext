#include "php.h"
#include "zend_smart_str.h"
#include "../include/php_mysql_qp.h"
#include "../include/query_decomposer.h"
#include "../include/mysql_query_parser.h"
#include <mysql.h>
#include <string.h>
#include <ctype.h>

/* strcasestr is available on macOS, no need to implement */

/* Initialize query components structure */
query_components* init_query_components() {
    query_components *components = emalloc(sizeof(query_components));
    memset(components, 0, sizeof(query_components));
    
    components->fields = emalloc(sizeof(zval));
    components->tables = emalloc(sizeof(zval));
    components->joins = emalloc(sizeof(zval));
    components->where_conditions = emalloc(sizeof(zval));
    components->group_by = emalloc(sizeof(zval));
    components->having = emalloc(sizeof(zval));
    components->order_by = emalloc(sizeof(zval));
    components->limit_clause = emalloc(sizeof(zval));
    components->values = emalloc(sizeof(zval));
    components->parameters = emalloc(sizeof(zval));
    
    array_init(components->fields);
    array_init(components->tables);
    array_init(components->joins);
    array_init(components->where_conditions);
    array_init(components->group_by);
    array_init(components->having);
    array_init(components->order_by);
    array_init(components->limit_clause);
    array_init(components->values);
    array_init(components->parameters);
    
    return components;
}

/* Free query components structure */
void mysql_free_query_components(query_components *components) {
    if (!components) return;
    
    if (components->type) efree(components->type);
    
    if (components->fields) {
        zval_ptr_dtor(components->fields);
        efree(components->fields);
    }
    if (components->tables) {
        zval_ptr_dtor(components->tables);
        efree(components->tables);
    }
    if (components->joins) {
        zval_ptr_dtor(components->joins);
        efree(components->joins);
    }
    if (components->where_conditions) {
        zval_ptr_dtor(components->where_conditions);
        efree(components->where_conditions);
    }
    if (components->group_by) {
        zval_ptr_dtor(components->group_by);
        efree(components->group_by);
    }
    if (components->having) {
        zval_ptr_dtor(components->having);
        efree(components->having);
    }
    if (components->order_by) {
        zval_ptr_dtor(components->order_by);
        efree(components->order_by);
    }
    if (components->limit_clause) {
        zval_ptr_dtor(components->limit_clause);
        efree(components->limit_clause);
    }
    if (components->values) {
        zval_ptr_dtor(components->values);
        efree(components->values);
    }
    if (components->parameters) {
        zval_ptr_dtor(components->parameters);
        efree(components->parameters);
    }
    
    efree(components);
}

/* Skip whitespace */
const char* skip_whitespace(const char *str) {
    while (*str && isspace(*str)) str++;
    return str;
}

/* Find keyword in string (case insensitive) */
const char* find_keyword(const char *str, const char *keyword) {
    size_t keyword_len = strlen(keyword);
    const char *pos = str;
    
    while ((pos = strcasestr(pos, keyword)) != NULL) {
        /* Check if this is a word boundary */
        if ((pos == str || isspace(*(pos-1))) && 
            (isspace(*(pos + keyword_len)) || *(pos + keyword_len) == '\0' || 
             *(pos + keyword_len) == '(' || *(pos + keyword_len) == ',' || 
             *(pos + keyword_len) == ';')) {
            return pos;
        }
        pos++;
    }
    return NULL;
}

/* Extract field list from SELECT clause */
int parse_field_list(const char *fields_str, zval *fields_array) {
    char *fields_copy = estrdup(fields_str);
    char *token = strtok(fields_copy, ",");
    
    while (token != NULL) {
        /* Trim whitespace */
        while (isspace(*token)) token++;
        char *end = token + strlen(token) - 1;
        while (end > token && isspace(*end)) end--;
        *(end + 1) = '\0';
        
        add_next_index_string(fields_array, token);
        token = strtok(NULL, ",");
    }
    
    efree(fields_copy);
    return SUCCESS;
}

/* Extract table list from FROM clause */
int parse_table_list(const char *tables_str, zval *tables_array) {
    char *tables_copy = estrdup(tables_str);
    char *token = strtok(tables_copy, ",");
    
    while (token != NULL) {
        /* Trim whitespace */
        while (isspace(*token)) token++;
        char *end = token + strlen(token) - 1;
        while (end > token && isspace(*end)) end--;
        *(end + 1) = '\0';
        
        /* Parse table with potential alias */
        char *alias_pos = strstr(token, " AS ");
        if (!alias_pos) {
            alias_pos = strrchr(token, ' ');
        }
        
        zval table_info;
        array_init(&table_info);
        
        if (alias_pos && alias_pos > token) {
            *alias_pos = '\0';
            add_assoc_string(&table_info, "table", token);
            add_assoc_string(&table_info, "alias", alias_pos + (strncmp(alias_pos + 1, "AS ", 3) == 0 ? 4 : 1));
        } else {
            add_assoc_string(&table_info, "table", token);
            add_assoc_string(&table_info, "alias", "");
        }
        
        add_next_index_zval(tables_array, &table_info);
        token = strtok(NULL, ",");
    }
    
    efree(tables_copy);
    return SUCCESS;
}

/* Decompose SELECT query */
int extract_select_components(const char *query, query_components *components) {
    const char *select_pos = find_keyword(query, "SELECT");
    const char *from_pos = find_keyword(query, "FROM");
    const char *where_pos = find_keyword(query, "WHERE");
    const char *group_pos = find_keyword(query, "GROUP BY");
    const char *having_pos = find_keyword(query, "HAVING");
    const char *order_pos = find_keyword(query, "ORDER BY");
    const char *limit_pos = find_keyword(query, "LIMIT");
    
    components->type = estrdup("SELECT");
    
    /* Extract fields */
    if (select_pos && from_pos) {
        size_t fields_len = from_pos - select_pos - 6; /* Skip "SELECT" */
        char *fields_str = estrndup(select_pos + 6, fields_len);
        parse_field_list(fields_str, components->fields);
        efree(fields_str);
    }
    
    /* Extract tables */
    if (from_pos) {
        const char *end_pos = where_pos;
        if (!end_pos) end_pos = group_pos;
        if (!end_pos) end_pos = order_pos;
        if (!end_pos) end_pos = limit_pos;
        if (!end_pos) end_pos = query + strlen(query);
        
        size_t tables_len = end_pos - from_pos - 4; /* Skip "FROM" */
        char *tables_str = estrndup(from_pos + 4, tables_len);
        parse_table_list(tables_str, components->tables);
        efree(tables_str);
    }
    
    /* Extract WHERE clause */
    if (where_pos) {
        const char *end_pos = group_pos;
        if (!end_pos) end_pos = order_pos;
        if (!end_pos) end_pos = limit_pos;
        if (!end_pos) end_pos = query + strlen(query);
        
        size_t where_len = end_pos - where_pos - 5; /* Skip "WHERE" */
        char *where_str = estrndup(where_pos + 5, where_len);
        add_next_index_string(components->where_conditions, where_str);
        efree(where_str);
    }
    
    /* Extract ORDER BY */
    if (order_pos) {
        const char *end_pos = limit_pos;
        if (!end_pos) end_pos = query + strlen(query);
        
        size_t order_len = end_pos - order_pos - 8; /* Skip "ORDER BY" */
        char *order_str = estrndup(order_pos + 8, order_len);
        add_next_index_string(components->order_by, order_str);
        efree(order_str);
    }
    
    /* Extract LIMIT */
    if (limit_pos) {
        const char *end_pos = query + strlen(query);
        size_t limit_len = end_pos - limit_pos - 5; /* Skip "LIMIT" */
        char *limit_str = estrndup(limit_pos + 5, limit_len);
        add_next_index_string(components->limit_clause, limit_str);
        efree(limit_str);
    }
    
    return SUCCESS;
}

/* Main query decomposition function */
query_components* mysql_decompose_query(const char *query, size_t query_len) {
    query_components *components = init_query_components();
    
    /* Determine query type and extract components accordingly */
    int query_type = mysql_get_query_type(query);
    
    switch (query_type) {
        case QUERY_TYPE_SELECT:
            extract_select_components(query, components);
            break;
        case QUERY_TYPE_INSERT:
            // TODO: Implement INSERT extraction
            components->type = estrdup("INSERT");
            break;
        case QUERY_TYPE_UPDATE:
            // TODO: Implement UPDATE extraction  
            components->type = estrdup("UPDATE");
            break;
        case QUERY_TYPE_DELETE:
            // TODO: Implement DELETE extraction
            components->type = estrdup("DELETE");
            break;
        default:
            components->type = estrdup("UNKNOWN");
    }
    
    return components;
}

/* Build SELECT query from components - SAFE VERSION */
char* build_select_query(query_components *components) {
    /* Use smart_str for safe string building */
    smart_str str = {0};
    
    smart_str_appends(&str, "SELECT ");
    
    /* Add fields */
    if (Z_TYPE_P(components->fields) == IS_ARRAY) {
        zval *field;
        int first = 1;
        
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(components->fields), field) {
            if (!first) {
                smart_str_appends(&str, ", ");
            }
            if (Z_TYPE_P(field) == IS_STRING) {
                smart_str_appends(&str, Z_STRVAL_P(field));
            }
            first = 0;
        } ZEND_HASH_FOREACH_END();
    } else {
        smart_str_appends(&str, "*");  /* Default fallback */
    }
    
    /* Add FROM clause */
    if (Z_TYPE_P(components->tables) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(components->tables)) > 0) {
        smart_str_appends(&str, " FROM ");
        
        zval *table;
        int first = 1;
        
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(components->tables), table) {
            if (!first) {
                smart_str_appends(&str, ", ");
            }
            
            if (Z_TYPE_P(table) == IS_ARRAY) {
                zval *table_name = zend_hash_str_find(Z_ARRVAL_P(table), "table", 5);
                zval *alias = zend_hash_str_find(Z_ARRVAL_P(table), "alias", 5);
                
                if (table_name && Z_TYPE_P(table_name) == IS_STRING) {
                    smart_str_appends(&str, Z_STRVAL_P(table_name));
                }
                if (alias && Z_TYPE_P(alias) == IS_STRING && Z_STRLEN_P(alias) > 0) {
                    smart_str_appends(&str, " AS ");
                    smart_str_appends(&str, Z_STRVAL_P(alias));
                }
            }
            first = 0;
        } ZEND_HASH_FOREACH_END();
    }
    
    /* Add WHERE clause */
    if (Z_TYPE_P(components->where_conditions) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(components->where_conditions)) > 0) {
        smart_str_appends(&str, " WHERE");
        
        zval *condition;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(components->where_conditions), condition) {
            if (Z_TYPE_P(condition) == IS_STRING) {
                smart_str_appends(&str, Z_STRVAL_P(condition));
            }
            break; /* For now, just use the first condition */
        } ZEND_HASH_FOREACH_END();
    }
    
    /* Add ORDER BY clause */
    if (Z_TYPE_P(components->order_by) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(components->order_by)) > 0) {
        smart_str_appends(&str, " ORDER BY");
        
        zval *order;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(components->order_by), order) {
            if (Z_TYPE_P(order) == IS_STRING) {
                smart_str_appends(&str, Z_STRVAL_P(order));
            }
            break; /* For now, just use the first order */
        } ZEND_HASH_FOREACH_END();
    }
    
    /* Add LIMIT clause */
    if (Z_TYPE_P(components->limit_clause) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(components->limit_clause)) > 0) {
        smart_str_appends(&str, " LIMIT");
        
        zval *limit;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(components->limit_clause), limit) {
            if (Z_TYPE_P(limit) == IS_STRING) {
                smart_str_appends(&str, Z_STRVAL_P(limit));
            }
            break; /* For now, just use the first limit */
        } ZEND_HASH_FOREACH_END();
    }
    
    /* Finalize the smart string and return as emalloc'd string */
    smart_str_0(&str);
    
    if (str.s) {
        char *result = emalloc(ZSTR_LEN(str.s) + 1);
        memcpy(result, ZSTR_VAL(str.s), ZSTR_LEN(str.s));
        result[ZSTR_LEN(str.s)] = '\0';
        smart_str_free(&str);
        return result;
    } else {
        /* Fallback for empty result */
        char *fallback = emalloc(16);
        strcpy(fallback, "SELECT 1");
        return fallback;
    }
}

/* Main query reconstruction function */
char* mysql_reconstruct_query(query_components *components) {
    if (!components || !components->type) {
        return estrdup("/* Invalid components */");
    }
    
    if (strcmp(components->type, "SELECT") == 0) {
        return build_select_query(components);
    }
    
    /* Fallback for unsupported types */
    char *fallback = emalloc(256);
    snprintf(fallback, 256, "/* %s query reconstruction not yet implemented */", components->type);
    return fallback;
}