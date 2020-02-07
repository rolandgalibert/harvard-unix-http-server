/* 
 * File:        httplib.h
 * Author:      Roland L. Galibert/CSCI E-28 staff
 *              Problem Set 6 (wsng)
 *              For Harvard Extension course CSCI E-28 Unix Systems Programming
 * Date:        May 2, 2015
 * 
 * This file contains definitions for macros and structs for the HTTP response
 * transmission functionality provided in httplib.c.
 */
#include        <stdio.h>
#include        <time.h>
#include        <openssl/md5.h>

/* Array elements in httplib.c constant array http_resp_status_table[] */
#define HTTP_RESP_STATUS_200 0
#define HTTP_RESP_STATUS_201 1
#define HTTP_RESP_STATUS_202 2
#define HTTP_RESP_STATUS_204 3
#define HTTP_RESP_STATUS_301 4
#define HTTP_RESP_STATUS_302 5
#define HTTP_RESP_STATUS_304 6
#define HTTP_RESP_STATUS_400 7
#define HTTP_RESP_STATUS_401 8
#define HTTP_RESP_STATUS_403 9
#define HTTP_RESP_STATUS_404 10
#define HTTP_RESP_STATUS_500 11
#define HTTP_RESP_STATUS_501 12
#define HTTP_RESP_STATUS_502 13
#define HTTP_RESP_STATUS_503 14

/* Element in httplib.c constant array http_resp_status_table[] */
struct http_resp_status {
        char *code;
        char *desc;
};

/* httplib.c function declarations */
void print_http_resp_status(FILE *fp, char *http_version, int response);
void print_http_header_date(FILE *fp, char *date);
void print_http_header_date_curr_rfc822(FILE *fp);
char *rfc822_time(time_t thetime);
void print_http_header_server(FILE *fp, char *server);
void print_http_header_server_official(FILE *fp);
char *full_hostname();
void print_http_header_content_length(FILE *fp, long length);
void print_http_header_content_language(FILE *fp, char *lang);
void print_http_header_content_language_en(FILE *fp);
void print_http_header_content_MD5(FILE *fp, unsigned char digest[MD5_DIGEST_LENGTH]);
void print_http_header_content_type(FILE *fp, char *type);
void print_http_header_upgrade(FILE *fp, char *version);