/* 
 * File:        conf.h
 * Author:      Roland L. Galibert/CSCI E-28 staff
 *              Problem Set 6 (wsng)
 *              For Harvard Extension course CSCI E-28 Unix Systems Programming
 * Date:        May 2, 2015
 * 
 * This file contains definitions of macros and structures for maintaining
 * parameters to configure the wsng server.
 */  

/* 
 * Names of parameters in conf.c configuration table conf_table and
 * content type table content_type_table.
 */
#define CONF_FILE_DEFAULT               "wsng.conf"
#define CONF_PORT                       "port"
#define CONF_SERVER_ROOT                "server_root"
#define CONF_SERVER_NAME                "server_name"
#define CONF_SERVER_VERSION             "server_version"
#define CONF_HTTP_VERSION		"http_version"
#define CONF_CONTENT_TYPE               "type"

/* Default values */
#define CONF_CONTENT_TYPE_DEFAULT       "DEFAULT"
#define CONTENT_TYPE_DEFAULT_VALUE      "text/plain"
#define PORT_DEFAULT                    "80"
#define SERVER_ROOT_DEFAULT             "/home/r/g/rgalibert/public_html/wsng"
#define HTTP_VERSION_DEFAULT            "HTTP/1.0"

/* Record for general server configuration parameters */
struct conf_record {
        char *name;
        char *value;
        struct conf_record *next;
};

/* Content type structs */
struct content_type_table {
        char *default_content_type;
        struct content_type_record *first_record;
};

struct content_type_record {
        char *file_type;
        char *content_type;
        struct content_type_record *next;
};

/* conf.c functions */
int load_configuration(char *conf_file_name);
void add_conf_record(char *name, char *value);
struct conf_record *get_conf_record(char *record_name);
void add_new_conf_record(char *record_name, char *record_value);
char *get_conf_value(char *name);
void add_content_type(char *file_type, char *content_type);
struct content_type_record *get_content_type_record(char *file_type);
void add_new_content_type_record(char *file_type, char *content_type);
char *get_content_type(char *file_type);
char *new_string(char *value);
int load_default_configuration();