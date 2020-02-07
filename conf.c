/* 
 * File:        conf.c
 * Author:      Roland L. Galibert
 *              Problem Set 6 (wsng)
 *              For Harvard Extension course CSCI E-28 Unix Systems Programming
 * Date:        May 2, 2015
 * 
 * This file contains for initializing and accessing wsng server configuration
 * parameters, maintained in the tables conf_table (general server parameters)
 * and content_type_table (content types supported by wsng). The values for
 * these parameters are generally read in from a configuration file, though
 * default values are provided in conf.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "conf.h"

#define BUFFER_SIZE 4096
#define PARM_SIZE 256

struct conf_record *conf_table;
struct content_type_table *content_type_table;
        
int load_configuration(char *conf_file_name)
{
        FILE *conf_file;
        char line[BUFFER_SIZE];
        char command[PARM_SIZE], parm1[PARM_SIZE], parm2[PARM_SIZE];
	int parms_read;
        
        conf_table = (struct conf_record *) NULL;
        content_type_table = malloc(sizeof(struct content_type_table));
        content_type_table->default_content_type = CONTENT_TYPE_DEFAULT_VALUE;
        content_type_table->first_record = (struct content_type_record *) NULL;
        
        if ((conf_file = (FILE *) fopen(conf_file_name, "r")) == NULL) {
                fprintf(stderr, "load_configuration: could not open configuration file %s\n", conf_file_name);
                return -1;
        }
        while (fgets(line, BUFFER_SIZE, conf_file) != NULL) {
		parms_read = sscanf(line, "%s %s %s", command, parm1, parm2);
		if (parms_read > 0) {
			if (command[0] == '#') {
				continue;
			}
                        if (strcmp(command, CONF_CONTENT_TYPE) == 0) {
                                add_content_type(parm1, parm2);
                        } else {
                                add_conf_record(command, parm1);
                        }
                }
        }
        fclose(conf_file);
	
	/* Add default values if these were not set for corresponding configuration parms */
        if (get_conf_value(CONF_PORT) == NULL) {
                add_new_conf_record(CONF_PORT, PORT_DEFAULT);
        }
        if (get_conf_value(CONF_SERVER_ROOT) == NULL) {
                add_new_conf_record(CONF_SERVER_ROOT, SERVER_ROOT_DEFAULT);
        }
        if (get_conf_value(CONF_HTTP_VERSION) == NULL) {
                add_new_conf_record(CONF_HTTP_VERSION, HTTP_VERSION_DEFAULT);
        }
        return 0;
}

void add_conf_record(char *name, char *value)
{
        struct conf_record *conf_record;
	if ((name != NULL) && (value != NULL)) {
                if ((conf_record = get_conf_record(name)) != (struct conf_record *) NULL) {
			free(conf_record->value);
                        conf_record->value = strdup(value);
                } else {
                        add_new_conf_record(name, value);
                }
        }
}

struct conf_record *get_conf_record(char *record_name)
{
        struct conf_record *conf_record = conf_table;
        while (conf_record != (struct conf_record *) NULL) {
                if (strcmp(conf_record->name, record_name) == 0) {
                        return conf_record;
                } else {
                        conf_record = conf_record->next;
                }
        }
        return (struct conf_record *) NULL;
}

void add_new_conf_record(char *record_name, char *record_value)
{
        struct conf_record *new_conf_record;
        new_conf_record = malloc(sizeof(struct conf_record));
        new_conf_record->name = strdup(record_name);
        new_conf_record->value = strdup(record_value);
        if (conf_table == (struct conf_record *) NULL) {
                new_conf_record->next = (struct conf_record *) NULL;
        } else {
                new_conf_record->next = conf_table;
        }
        conf_table = new_conf_record;
}

char *get_conf_value(char *name)
{
       struct conf_record *conf_record = conf_table;
       while (conf_record != (struct conf_record *) NULL) {
               if (strcmp(conf_record->name, name) == 0) {
                       return conf_record->value;
               } else {
                       conf_record = conf_record->next;
               }
       }
       return (char *) NULL;
}

void add_content_type(char *file_type, char *content_type)
{
        struct content_type_record *content_type_record;
        
        if ((file_type != (char *) NULL) && (content_type != (char *) NULL)) {
                if (strcmp(file_type, CONF_CONTENT_TYPE_DEFAULT) == 0) {
                        content_type_table->default_content_type = strdup(content_type);
                } else {
                        if ((content_type_record = get_content_type_record(file_type)) != NULL) {
                                free(content_type_record->content_type);
                                content_type_record->content_type = strdup(content_type);
                        } else {
                                add_new_content_type_record(file_type, content_type);
                        }
                }
        }
}

struct content_type_record *get_content_type_record(char *file_type)
{
        struct content_type_record *content_type_record = content_type_table->first_record;
        
        while (content_type_record != (struct content_type_record *) NULL) {
                if (strcmp(content_type_record->file_type, file_type) == 0) {
                        return content_type_record;
                } else {
                        content_type_record = content_type_record->next;
                }
        }
        return (struct content_type_record *) NULL;
}

void add_new_content_type_record(char *file_type, char *content_type)
{
        struct content_type_record *content_type_record = malloc(sizeof(struct content_type_record));
        content_type_record->file_type = strdup(file_type);
        content_type_record->content_type = strdup(content_type);
        content_type_record->next = content_type_table->first_record;
        content_type_table->first_record = content_type_record;
}

char *get_content_type(char *file_type)
{
        struct content_type_record *content_type_record = content_type_table->first_record;
        
        while (content_type_record != (struct content_type_record *) NULL) {
                if (strcmp(content_type_record->file_type, file_type) == 0) {
                        return content_type_record->content_type;
                } else {
                        content_type_record = content_type_record->next;
                }
        }
        return content_type_table->default_content_type;
}

int load_default_configuration()
{
        return load_configuration(CONF_FILE_DEFAULT);
}