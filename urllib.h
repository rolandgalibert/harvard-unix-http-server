/* 
 * File:        urlib.h
 * Author:      Roland L. Galibert
 *              Problem Set 6 (wsng)
 *              For Harvard Extension course CSCI E-28 Unix Systems Programming
 * Date:        May 2, 2015
 * 
 * This file contains definitions of the structs on which the functionality
 * defined in urllib.c is based. Those functions parse an HTTP request/URL
 * into separate request, URL (directory/file) and query string components.
 */ 

/* struct http_request.type command types */
#define HTTP_REQ_GET    0
#define HTTP_REQ_HEAD   1
#define HTTP_REQ_POST   2
#define HTTP_REQ_PUT    3
#define HTTP_REQ_DELETE 4

struct http_request {
        int type;       /* HTTP_REQ_GET, HTTP_REQ_HEAD or HTTP_REQ_POST */
        char *resource; /* Entire URI */
        char *version;  /* Client HTTP version */
        char *file;     /* File/directory specification */
        struct url_parm *parms; /* Query parms, if any */
};

struct url_parm {
        char *name;
        char *value;
        struct url_parm *next;
};

/* urllib.c function declarations */
int parse_request(char *req_string, struct http_request *http_request);
int get_req_type(char *command);
char *get_file(char *resource);
int get_url_parms(char *resource, struct url_parm **parms);
struct url_parm *new_url_parm(char *name, char *value);
void set_url_parm_name(struct url_parm *url_parm, char *name);
void set_url_parm_value(struct url_parm *url_parm, char *value);
char *get_file_ext(char *file_name);