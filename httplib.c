/* 
 * File:        httplib.c
 * Author:      Roland L. Galibert/CSCI E-28 staff
 *              Problem Set 6 (wsng)
 *              For Harvard Extension course CSCI E-28 Unix Systems Programming
 * Date:        May 2, 2015
 * 
 * This file contains functions to support the transmission of HTTP responses,
 * as well as a constant array http_resp_status_table which contains HTTP
 * response status codes and their corresponding descriptions as defined
 * in RFC 1945.
 */

#include        <netdb.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <time.h>
#include        <unistd.h>
#include        <openssl/md5.h>
#include        <sys/param.h>
#include	"httplib.h"

struct http_resp_status http_resp_status_table[] = {
        {"200", "OK"},
        {"201", "Created"},
        {"202", "Accepted"},
        {"204", "No Content"},
        {"301", "Moved Permanently"},
        {"302", "Moved Temporarily"},
        {"304", "Not Modified"},
        {"400", "Bad Request"},
        {"401", "Unauthorized"},
        {"403", "Forbidden"},
        {"404", "Not Found"},
        {"500", "Internal Server Error"},
        {"501", "Not Implemented"},
        {"502", "Bad Gateway"},
        {"503", "Service Unavailable"},
};

void print_http_resp_status(FILE *fp, char *http_version, int response)
{
        fprintf(fp, "%s %s %s\n", http_version,
                http_resp_status_table[response].code,
                http_resp_status_table[response].desc);
        fprintf(stderr, "%s %s %s\n", http_version,
                http_resp_status_table[response].code,
                http_resp_status_table[response].desc);
	
}

void print_http_header_date(FILE *fp, char *date)
{
        fprintf(fp, "Date: %s\n", date);
}

void print_http_header_date_curr_rfc822(FILE *fp)
{
        print_http_header_date(fp, rfc822_time(time(0L)));
}

/*
 *      function        rfc822_time()
 *      purpose         return a string suitable for web servers
 *      details         Sun, 06 Nov 1994 08:49:37 GMT    
 *      method          use gmtime() to get struct
 *                      then use asctime to translate to English
 *                      Tue Nov  9 15:37:29 1993\n\0
 *                      012345678901234567890123456789
 *                      then rearrange using sprintf
 *      arg             a time_t value 
 *      returns         a pointer to a static buffer
 */
char *rfc822_time(time_t thetime)
{
        struct tm *t ;
        char    *str;
        int     d;
        static  char retval[36];
        
        t = gmtime( &thetime );         /* break into parts     */
        str = asctime( t );             /* create string        */
        d = atoi( str + 8 );
        sprintf(retval,"%.3s, %02d %.3s %.4s %.8s GMT", 
                        str ,   d, str+4, str+20, str+11 );
        return retval;
}

void print_http_header_server(FILE *fp, char *server)
{
        fprintf(fp, "Server: %s\n", server);
}

void print_http_header_server_official(FILE *fp)
{
        print_http_header_server(fp, full_hostname());
}

char *
full_hostname()
/*
 * returns full `official' hostname for current machine
 * NOTE: this returns a ptr to a static buffer that is
 *       overwritten with each call. ( you know what to do.)
 */
{
        struct  hostent         *hp;
        char    hname[MAXHOSTNAMELEN];
        static  char fullname[MAXHOSTNAMELEN];

        if ( gethostname(hname,MAXHOSTNAMELEN) == -1 )  /* get rel name */
        {
                perror("gethostname");
                exit(1);
        }
        hp = gethostbyname( hname );            /* get info about host  */
        if ( hp == NULL )                       /*   or die             */
                return NULL;
        strcpy( fullname, hp->h_name );         /* store foo.bar.com    */
        return fullname;                        /* and return it        */
}

void print_http_header_content_length(FILE *fp, long length)
{
        fprintf(fp, "Content-Length: %ld\n", length);
}

void print_http_header_content_language(FILE *fp, char *lang)
{
        fprintf(fp, "Content-Language: %s\n", lang);
}

void print_http_header_content_language_en(FILE *fp)
{
        print_http_header_content_language(fp, "en");
}

void print_http_header_content_MD5(FILE *fp, unsigned char digest[MD5_DIGEST_LENGTH])
{
        int i;
        
        fprintf(fp, "Content-MD5: ");
        for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
                fprintf(fp, "%02x", digest[i]);
        }
        fprintf(fp, "\n");
}

void print_http_header_content_type(FILE *fp, char *type)
{
        fprintf(fp, "Content-Type: %s\n", type);
	fprintf(stderr, "Content-Type: %s\n", type);
}

void print_http_header_upgrade(FILE *fp, char *version)
{
        fprintf(fp, "Upgrade: %s\n", version);
}
      