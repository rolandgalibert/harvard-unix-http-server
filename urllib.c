/* 
 * File:        urlib.c
 * Author:      Roland L. Galibert
 *              Problem Set 6 (wsng)
 *              For Harvard Extension course CSCI E-28 Unix Systems Programming
 * Date:        May 2, 2015
 * 
 * This file contains functions that parse an HTTP request/URL into separate
 * request, URL (directory/file) and query string components which can be
 * stored and accessed using the http_request and url_parm structs defined
 * in urllib.h.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "urllib.h"

#define MAX_RQ_LEN      4096

int parse_request(char *req_string, struct http_request *http_request)
{
        int req_type;
        char command[MAX_RQ_LEN];
        char resource[MAX_RQ_LEN];
        char version[MAX_RQ_LEN];       /* request HTTP version */

        if ( sscanf(req_string, "%s%s%s", command, resource, version) != 3 ){
                return -1;
        }
        if ((req_type = get_req_type(command)) != -1) {
                http_request->type = req_type;
        } else {
                return -1;
        }
        http_request->resource = malloc(strlen(resource) + 1);
        strcpy(http_request->resource, resource);
        http_request->version = malloc(strlen(version) + 1);
        strcpy(http_request->version, version);
        http_request->file = get_file(resource);

        if (get_url_parms(resource, &(http_request->parms)) == -1) {
                return -1;
        }
        return 0;
}

int get_req_type(char *command)
{
        if (strcmp(command, "GET") == 0) {
                return HTTP_REQ_GET;
        } else if (strcmp(command, "HEAD") == 0) {
                return HTTP_REQ_HEAD;
        } else if (strcmp(command, "POST") == 0) {
                return HTTP_REQ_POST;
        } else {
                return -1;
        }
}

char *get_file(char *resource)
{
        char *file = malloc(MAX_RQ_LEN);
        char *file_copy;
        char *resource_copy;
        
        char copy[MAX_RQ_LEN];
        char *nexttoken;
        
        /* Grab directory/file portion of URL (w/o request parms) */
        resource_copy = malloc(sizeof(resource) + 1);
        strcpy(resource_copy, resource);
        strcpy(file, strtok(resource_copy, "?"));
        
        /* Eliminate .. */
        *copy = '\0';
        file_copy = malloc(sizeof(file) + 1);
        strcpy(file_copy, file);
        nexttoken = strtok(file_copy, "/");
        while( nexttoken != NULL )
        {
                if ( strcmp(nexttoken,"..") != 0 )
                {
                        if ( *copy ) {
                                strcat(copy, "/");
                        }
                        strcat(copy, nexttoken);
                }
                nexttoken = strtok(NULL, "/");
        }
        strcpy(file, copy);
        free(file_copy);
        free(resource_copy);

        /* Special case */
        if ( strcmp(file,"") == 0 ) {
                strcpy(file, ".");
        }
        return file;
}

int get_url_parms(char *resource, struct url_parm **parms)
{
        char *resource_parms, *resource_copy;
        char *next_parm, *parm_name, *parm_value;
        struct url_parm *first_url_parm = NULL, *next_url_parm = NULL;
        char *name_copy;
	
        resource_copy = strdup(resource);
        resource_parms = strtok(resource_copy, "?");
        resource_parms = strtok(NULL, "?");     /* parms follow URL ? and are second token */
        next_parm = strtok(resource_parms, "&");
        
        while (next_parm != NULL) {
                if (first_url_parm == (struct url_parm *) NULL) {
                        first_url_parm = next_url_parm = new_url_parm(next_parm, NULL);
                } else {
                        next_url_parm->next = new_url_parm(next_parm, NULL);
                        next_url_parm = next_url_parm->next;
                }
                next_parm = strtok(NULL, "&");
        }
        next_url_parm = first_url_parm;
        
        while (next_url_parm != (struct url_parm *) NULL) {
		name_copy = strdup(next_url_parm->name);
                parm_name = strtok(name_copy, "=");
                parm_value = strtok(NULL, "=");
                if (parm_value != (char *) NULL) {
                        set_url_parm_name(next_url_parm, parm_name);
                        set_url_parm_value(next_url_parm, parm_value);
                } else {
                        set_url_parm_name(next_url_parm, NULL);
                        set_url_parm_value(next_url_parm, parm_name);
                }
                next_url_parm = next_url_parm->next;
        }
        free(resource_copy);
	*parms = first_url_parm;
        return 0;
}

struct url_parm *new_url_parm(char *name, char *value)
{
        struct url_parm *new_url_parm = malloc(sizeof(struct url_parm));
        set_url_parm_name(new_url_parm, name);
        set_url_parm_value(new_url_parm, value);
        new_url_parm->next = (struct url_parm *) NULL;
        return new_url_parm;
}

void set_url_parm_name(struct url_parm *url_parm, char *name)
{
        if (name != (char *) NULL) {
                url_parm->name = strdup(name);
        } else {
                url_parm->name = (char *) NULL;
        }
}

void set_url_parm_value(struct url_parm *url_parm, char *value)
{
        if (value != (char *) NULL) {
                url_parm->value = strdup(value);
        } else {
                url_parm->value = (char *) NULL;
        }
}

char *get_file_ext(char *file_name)
{
        char *file_ext = (char *) NULL;
        int start, end;
        
        end = start = strlen(file_name) - 1;
        while ((start >= 0) && (*(file_name + start) != '.')) {
                start--;
        }
        if ((start != end) && (start >= 0)) {
                file_ext = malloc(end - start + 1);
                strcpy(file_ext, (file_name + start + 1));
        }
        return file_ext;
}