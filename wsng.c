/* 
 * File:        wsng.c
 * Author:      Roland L. Galibert/CSCI E-28 staff
 *              Problem Set 6 (wsng)
 *              For Harvard Extension course CSCI E-28 Unix Systems Programming
 * Date:        May 2, 2015
 * 
 * This program implements a basic HTTP server which includes the functionality
 * described in the assignment document. Please see the Outline design document
 * as well as assignment specifications for more details.
 */

#include        <openssl/md5.h>
#include	<dirent.h>
#include        <netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<strings.h>
#include	<string.h>
#include	<errno.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<signal.h>
#include	<sys/wait.h>
#include        "conf.h"
#include        "flexstr.h"
#include        "httplib.h"
#include	"socklib.h"
#include        "urllib.h"

#define	MAX_RQ_LEN	4096
#define	LINELEN		1024
#define	PARAM_LEN	128
#define	VALUE_LEN	512
#define	MAXDATELEN	100

struct sigaction orig_sigchld_handler;

#define	oops(m,x)	{ perror(m); exit(x); }

/*
 * prototypes
 */

/* General program initialization, exit, etc. */
int	startup(int, char *a[], char [], int *);
int     load_config_file(int, char *[]);
int     wsng_init();
int     wsng_exit();
int     fatal_error(char *);
int     init_sigchld();
void    handle_sigchld();
void    fatal(char *, char *);

/* Main loop functions */
void	read_til_crnl(FILE *);
void	process_rq( char *, FILE *);
void    handle_call(int);
int     read_request(FILE *, char *, int);
char    *readline(char *, int, FILE *);
int     proc_cmd_line(int, char *[]);

/* Response functions */
void	bad_request(FILE *);
void	cannot_do(FILE *fp);
void	do_404(char *item, FILE *fp);

/* Processing */
void    process_dir(struct http_request *http_request, FILE *fp);
void	do_cat(char *file, FILE *fpsock, int request_type);
int     compute_MD5(char *file, unsigned char digest[MD5_DIGEST_LENGTH]);
void	do_exec(char *file, FILE *fp, struct url_parm *parms, int request_type);
void	do_ls(char *directory, FILE *fp, int request_type);
void    print_ls_html(char *directory, FILE *fp, struct stat *, int num_entries, struct dirent **file_list);

/* Miscellaneous utility functions */
int	isadir(char *f);
int	not_exist(char *f);
struct  dirent *readdir(DIR *);
char    *fmt_time( time_t timeval , char *fmt );
int     file_exists(char *directory, char *file);
void    format_dir(FLEXSTR *directory);

/*
 * Function:
 *   main
 * 
 * Purpose:
 *   main driver for the wsng server.
 * 
 * Input parameters
 *   argc
 *   argv       Optional argv[1] -c indicates configuration file in argv[2].
 * 
 * Return values:
 *   Corresponding exit value if exit was called from within shell,
 *   otherwise EXIT_SUCCESS on successful completion, EXIT_FAILURE in
 *   case of system error.
 * 
 * Side effects:
 *   None.
 */
int main(int argc, char *argv[])
{
        int socket, fd;
        
	if (proc_cmd_line(argc, argv) == -1) {
		exit(EXIT_FAILURE);
	}
	if (load_config_file(argc, argv) == -1) {
                exit(EXIT_FAILURE);
        }
        if ((socket = wsng_init()) == -1) {
                exit(EXIT_FAILURE);
        }
        fprintf(stdout, "wsng started: Server name = %s, version = %s, port = %s\n",
                get_conf_value(CONF_SERVER_NAME),
                get_conf_value(CONF_SERVER_VERSION),
                get_conf_value(CONF_PORT));
        while(1) {
                fd = accept(socket, NULL, NULL);
                if (fd == -1) {
                        perror("accept");
		} else {
                        handle_call(fd);                /* handle call  */
		}
        }
        return EXIT_SUCCESS;
}

/*
 * Function:
 *   proc_cmd_line
 * 
 * Purpose:
 *   Processes and validates wsng command line parameters.
 * 
 * Input parameters
 *   argc
 *   argv       
 * 
 * Return values:
 *   -1 if command line was invalid, 0 otherwise.
 * 
 * Side effects:
 *   None.
 */
int proc_cmd_line(int argc, char *argv[])
{
	if (argc == 1) {
                return 0;
	} else if (argc == 2) {
                if (strcmp(argv[1], "-c") == 0) {
                        fprintf(stderr, "wsng: Missing arg for -c\n");
                } else {
                        fprintf(stderr, "wsng: Invalid option %s\n", argv[1]);
                }
                return -1;
        } else if (argc == 3) {
                if (strcmp(argv[1], "-c") != 0) {
                        fprintf(stderr, "wsng: Invalid option %s\n", argv[1]);
                        return -1;
                } else {
                        return 0;
                }
	} else {
		fprintf(stderr, "wsng: Invalid number of arguments\n");
		return -1;
	}
}

/*
 * Function:
 *   load_config_file
 * 
 * Purpose:
 *   Loads wsng server configuration parameters.
 * 
 * Input parameters
 *   argc
 *   argv       Optional argv[1] -c indicates configuration file in argv[2].
 * 
 * Return values:
 *   None
 * 
 * Side effects:
 *   None.
 */
int load_config_file(int argc, char *argv[])
{
        if (argc == 1) {
                return load_default_configuration();
        } else {
                return load_configuration(argv[2]);
        }
}

/*
 * Function:
 *   wsng_init
 * 
 * Purpose:
 *   Initializes server state, by changing into server directory, creating
 *   socket for input requests and setting SIGCHLD handling.
 * 
 * Input parameters
 *   None.
 * 
 * Return values:
 *   -1 if an error occurred, 0 otherwise.
 * 
 * Side effects:
 *   Directory change, SIGCHLD signal handler implemented.
 */
int wsng_init()
{
        int socket;

        if (chdir(get_conf_value(CONF_SERVER_ROOT)) == -1) {
                perror("wsng: ws_init: error calling chdir to change to server root_dir");
                return -1;
        }
        if ((socket = make_server_socket(atoi(get_conf_value(CONF_PORT)))) == -1) {
                perror("wsng: ws_init: error calling make_server_socket");
                return -1;
        }
        if (init_sigchld() == -1) {
                perror("wsng: ws_init: error calling init_sigchld");
                return -1;
	}
        return socket;
}

/*
 * Function:
 *   wsng_exit
 * 
 * Purpose:
 *   Restores system state to what it was previously by resetting SIGCHLD
 *   handling and freeing malloced space.
 * 
 * Input parameters
 *   None.
 * 
 * Return values:
 *   -1 if an error occurred, 0 otherwise.
 * 
 * Side effects:
 *   System restored to previous state.
 */
int wsng_exit()
{
	if (sigaction(SIGCHLD, &orig_sigchld_handler, NULL) == -1) {
                perror("wsng_exit() - error calling sigaction(SIGCHLD)");
                return -1;
        }
        return 0;
}

/*
 * Function:
 *   init_sigchld()
 * 
 * Purpose:
 *   Set handler for SIGCHLD signal.
 * 
 * Input parameters
 *   None
 * 
 * Return values:
 *   0 on success, -1 on failure
 * 
 * Side effects:
 *   Original handler settings saved in global orig_sigchld_handler.
 */
int init_sigchld()
{
        struct sigaction sigchld_handler;
        
        sigchld_handler.sa_handler = handle_sigchld;
        if (sigaction(SIGCHLD, &sigchld_handler, &orig_sigchld_handler) == -1) {
                perror("init_sigchld() - error calling sigaction(SIGCHLD)");
                return -1;
        }
        return 0;
}

/*
 * Function:
 *   handle_sigchld()
 * 
 * Purpose:
 *   Handles SIGCHLD signals from child processes which just completed.
 *   in order to previous these processes from becoming "zombies".
 * 
 * Input parameters
 *   None.
 * 
 * Return values:
 *   None.
 * 
 * Side effects:
 *   Global done variable is set to 1 in case of error.
 */
void handle_sigchld(int s)
{
	int old_errno = errno;
	while(waitpid(-1,NULL,WNOHANG) > 0)
		;
	errno = old_errno;
}

/*
 * handle_call(fd) - serve the request arriving on fd
 * summary: fork, then get request, then process request
 *    rets: child exits with 1 for error, 0 for ok
 *    note: closes fd in parent
 */
void handle_call(int fd)
{
	int	pid = fork();
	FILE	*fpin, *fpout;
	char	request[MAX_RQ_LEN];

	if ( pid == -1 ){
		perror("fork");
		return;
	}

	/* child: buffer socket and talk with client */
	if ( pid == 0 )
	{
		fpin  = fdopen(fd, "r");
		fpout = fdopen(fd, "w");
		if ( fpin == NULL || fpout == NULL )
			exit(1);

		if ( read_request(fpin, request, MAX_RQ_LEN) == -1 )
			exit(1);
		printf("Got a call: request = %s", request);

		process_rq(request, fpout);
		fflush(fpout);		/* send data to client	*/
		exit(0);		/* child is done	*/
					/* exit closes files	*/
	}
	/* parent: close fd and return to take next call	*/
	close(fd);
}

/*
 * read the http request into rq not to exceed rqlen
 * return -1 for error, 0 for success
 */
int read_request(FILE *fp, char rq[], int rqlen)
{
	/* null means EOF or error. Either way there is no request */
	if ( readline(rq, rqlen, fp) == NULL )
		return -1;
	read_til_crnl(fp);
	return 0;
}

void read_til_crnl(FILE *fp)
{
        char    buf[MAX_RQ_LEN];
        while( readline(buf,MAX_RQ_LEN,fp) != NULL 
			&& strcmp(buf,"\r\n") != 0 )
                ;
}

/*
 * readline -- read in a line from fp, stop at \n 
 *    args: buf - place to store line
 *          len - size of buffer
 *          fp  - input stream
 *    rets: NULL at EOF else the buffer
 *    note: will not overflow buffer, but will read until \n or EOF
 *          thus will lose data if line exceeds len-2 chars
 *    note: like fgets but will always read until \n even if it loses data
 */
char *readline(char *buf, int len, FILE *fp)
{
        int     space = len - 2;
        char    *cp = buf;
        int     c;

        while( ( c = getc(fp) ) != '\n' && c != EOF ){
                if ( space-- > 0 )
                        *cp++ = c;
        }
        if ( c == '\n' )
                *cp++ = c;
        *cp = '\0';
        return ( c == EOF && cp == buf ? NULL : buf );
}

/*
 * read_param:
 *   purpose -- read next parameter setting line from fp
 *   details -- a param-setting line looks like  name value
 *		for example:  port 4444
 *     extra -- skip over lines that start with # and those
 *		that do not contain two strings
 *   returns -- EOF at eof and 1 on good data
 *
 */
int read_param(FILE *fp, char *name, int nlen, char* value, int vlen)
{
	char	line[LINELEN];
	int	c;
	char	fmt[100] ;

	sprintf(fmt, "%%%ds%%%ds", nlen, vlen);

	/* read in next line and if the line is too long, read until \n */
	while( fgets(line, LINELEN, fp) != NULL )
	{
		if ( line[strlen(line)-1] != '\n' )
			while( (c = getc(fp)) != '\n' && c != EOF )
				;
		if ( sscanf(line, fmt, name, value ) == 2 && *name != '#' )
			return 1;
	}
	return EOF;
}
	

/*
 * Function:
 *   process_rq (Revised from original code)
 * 
 * Purpose:
 *   Takes in the HTTP request made by a client and:
 *     1) Checks whether if was a valid request, and if not, calls
 *        bad_request() to send back a 400 response
 *     2) If the request was valid, checks whether the request was either a
 *        of the command types supported by the server (GET or HEAD) and
 *        whether the URI file requested does indeed exist, and if not,
 *        calls do_404 to send back a 404 response.
 *     3) If the request and URI were valid:
 *          - Calls process_dir if the URI is a directory
 *          - Otherwise calls do_exec if the URI is a .cgi file
 *          - Otherwise calls do_cat if the URI is another file type
 *     4) If the request was not GET or HEAD, calls cannot_do()
 *         to send back a 501 response.
 * 
 * Input parameters
 *   rq         Entire HTTP request
 *   fp         FILE * pointer for client
 * 
 * Return values:
 *   None.
 * 
 * Side effects:
 *   None.
 */
void process_rq(char *rq, FILE *fp)
{
	struct http_request *http_request = malloc(sizeof(struct http_request));
	if (parse_request(rq, http_request) == -1) {
		bad_request(fp);
		return;
	}
	if ((http_request->type == HTTP_REQ_GET) || (http_request->type == HTTP_REQ_HEAD)) {
		if (not_exist(http_request->file)) {
			do_404(http_request->file, fp);
		} else if (isadir(http_request->file)) {
			process_dir(http_request, fp);
		} else if (strcmp(get_file_ext(http_request->file), "cgi") == 0) {
			do_exec(http_request->file, fp, http_request->parms, http_request->type);
		} else {
			do_cat(http_request->file, fp, http_request->type);
		}
	} else {
		cannot_do(fp);
	}
}

/*
 * Function:
 *   process_dir
 * 
 * Purpose:
 *   Takes http_request, known to be either a GET or HEAD for a directory,
 *   and calls the appropriate function, based on the index.html/index.cgi
 *   algorithm defined in the assignment specifications.
 * 
 * Input parameters
 *   http_request       struct *http_request with all request parameters
 *   fp                 FILE * pointer for client
 * 
 * Return values:
 *   None.
 * 
 * Side effects:
 *   None.
 */
void process_dir(struct http_request *http_request, FILE *fp)
{
        FLEXSTR *path = malloc(sizeof(FLEXSTR));
        fs_init(path, sizeof(http_request->file) + 11);
        fs_addstr(path, http_request->file);
        format_dir(path);
        
        if (file_exists(fs_getstr(path), "index.html")) {
                fs_addstr(path, "index.html");
                do_cat(fs_getstr(path), fp, http_request->type);
        } else if (file_exists(fs_getstr(path), "index.cgi")) {
                fs_addstr(path, "index.cgi");
                do_exec(fs_getstr(path), fp, http_request->parms, http_request->type);
        } else {
                do_ls(fs_getstr(path), fp, http_request->type);
        }
        fs_free(path);
}

/*
 * Function:
 *   file_exists
 * 
 * Purpose:
 *   Returns 1 if the given file exists in the given directory. The directory
 *   string is known to have a final /
 * 
 * Input parameters
 *   directory  char * directory string
 *   file       char * file string
 * 
 * Return values:
 *   1 if the given file exists in the given directory, 0 otherwise.
 * 
 * Side effects:
 *   None.
 */
int file_exists(char *directory, char *file)
{
        FLEXSTR *path = malloc(sizeof(FLEXSTR));
        struct stat buffer;

        fs_init(path, sizeof(directory) + sizeof(file));
        fs_addstr(path, directory);
        fs_addstr(path, file);
        if (lstat(fs_getstr(path), &buffer) == 0) {
                fs_free(path);
                return 1;
        } else {
                fs_free(path);
                return 0;
        }
}

/*
 * Function:
 *   format_dir
 * 
 * Purpose:
 *   Appends a / to the FLEXSTR directory string if one does not already
 *   exist.
 * 
 * Input parameters
 *   directory  FLEXSTR * directory string
 * 
 * Return values:
 *   None.
 * 
 * Side effects:
 *   None.
 */
void format_dir(FLEXSTR *directory)
{
        char *dir_string = fs_getstr(directory);
        if (dir_string[strlen(dir_string) - 1] != '/') {
                fs_addch(directory, '/');
        }
}

/* ------------------------------------------------------ *
   simple functions first:
	bad_request(fp)     bad request syntax
        cannot_do(fp)       unimplemented HTTP command
    and do_404(item,fp)     no such object
   ------------------------------------------------------ */

void
bad_request(FILE *fp)
{
	print_http_resp_status(fp,
			       get_conf_value(CONF_HTTP_VERSION),
			       HTTP_RESP_STATUS_400);
	fprintf(fp, "\r\nI cannot understand your request\r\n");
}

void
cannot_do(FILE *fp)
{
	print_http_resp_status(fp,
			       get_conf_value(CONF_HTTP_VERSION),
			       HTTP_RESP_STATUS_501);
	fprintf(fp, "\r\n");
	fprintf(fp, "That command is not yet implemented\r\n");
}

void
do_404(char *item, FILE *fp)
{
	print_http_resp_status(fp,
			       get_conf_value(CONF_HTTP_VERSION),
			       HTTP_RESP_STATUS_404);
	print_http_header_content_type(fp,
				       get_content_type(CONF_CONTENT_TYPE_DEFAULT));
	fprintf(fp, "\r\n");

	fprintf(fp, "The item you requested: %s\r\nis not found\r\n", 
			item);
}

/* ------------------------------------------------------ *
   the directory listing section
   isadir() uses stat, not_exist() uses stat
   do_ls runs ls. It should not
   ------------------------------------------------------ */

int
isadir(char *f)
{
	struct stat info;
	return ( stat(f, &info) != -1 && S_ISDIR(info.st_mode) );
}

int
not_exist(char *f)
{
	struct stat info;

	return( stat(f,&info) == -1 && errno == ENOENT );
}

/*
 * Function:
 *   do_ls (modified from original)
 * 
 * Purpose:
 *   To handle a GET or HEAD request for a directory.
 * 
 * Input parameters
 *   directory          char * directory string
 *   fp                 FILE * pointer to client
 *   request_type       HTTP_REQ_GET or HTTP_REQ_HEAD
 * 
 * Return values:
 *   None.
 * 
 * Side effects:
 *   None.
 */
void
do_ls(char *directory, FILE *fp, int request_type)
{
	DIR *current_dir;
        struct stat *file_status;
	int num_entries;
        struct dirent **file_list;
        
        /* 
         * Try to open directory and send appropriate response to client
         * if this cannot be done.
         */
	if ((current_dir = opendir(directory)) == (DIR *) NULL) {
		fprintf(stderr, "wsng: do_ls: `%s': Permission denied\n", directory);
                print_http_resp_status(fp,
                                       get_conf_value(CONF_HTTP_VERSION),
                                       HTTP_RESP_STATUS_401);
                fprintf(fp,"\r\n");
                fflush(fp);
        } else {
                
               /* 
                * Retrieve directory's status, sending appropriate response to client
                * if this cannot be done.
                */
                if ((file_status = malloc(sizeof(struct stat))) == (struct stat *) NULL) {
                        fprintf(stderr, "wsng: do_ls: error calling malloc\n");
                        print_http_resp_status(fp,
                                        get_conf_value(CONF_HTTP_VERSION),
                                        HTTP_RESP_STATUS_500);
                        fprintf(fp,"\r\n");
                        fflush(fp);
                        return;
                }
                
               /* 
                * Scan the directory to retrieve its contents, sending appropriate
                * response to client if this cannot be done.
                */
                num_entries = scandir(directory, &file_list, NULL, alphasort);
                if (num_entries < 0) {
                        perror("scandir");
                        print_http_resp_status(fp,
                                        get_conf_value(CONF_HTTP_VERSION),
                                        HTTP_RESP_STATUS_401);
                        fprintf(fp,"\r\n");
                        fflush(fp);
                        fprintf(fp,"401 Unauthorized\r\n");
                } else {
                        
                       /* 
                        * Directory was successfully scanned, send 200
                        * response to client.
                        */
                        print_http_resp_status(fp,
                                        get_conf_value(CONF_HTTP_VERSION),
                                        HTTP_RESP_STATUS_200);
                        print_http_header_content_type(fp,
                                                get_content_type("html"));
                        fprintf(fp,"\r\n");
                        fflush(fp);
                        
                        /* Print contents if request is a GET */
                        if (request_type == HTTP_REQ_GET) {
                                print_ls_html(directory, fp, file_status, num_entries, file_list);
                        }
                }
        }
}

/*
 * Function:
 *   print_ls_html
 * 
 * Purpose:
 *   To output the contents of a directory, in the form of HTML,
 *   to the client fp file pointer.
 *   (Algorithm based on scandir/alphasort example in man scandir).
 * 
 * Input parameters
 *   directory          char * directory string
 *   fp                 FILE * pointer to client
 *   file_status        struct stat with key directory values
 *   num_entries        Number of directory entries as determined by
 *                      scandir run in do_ls.
 * 
 * Return values:
 *   None.
 * 
 * Side effects:
 *   None.
 */
void print_ls_html(char *directory, FILE *fp, struct stat *file_status, int num_entries, struct dirent **file_list)
{
        fprintf(fp,"<html><head><title>Directory Listing</title></head><body>");
        fprintf(fp,"<table>");
        fprintf(fp,"<tr>");
        fprintf(fp,"<th align = \"left\" width=\"150\">Name</th>");
        fprintf(fp,"<th align = \"left\" width=\"150\">Last Modified</th>");
        fprintf(fp,"<th align = \"left\" width=\"100\">Size</th>");
        fprintf(fp,"</tr>");
        int i = 0;
        while (i < num_entries) {

                /* Skip . present directory */
                if (strcmp(file_list[i]->d_name, ".") == 0) {
                        i++;
                        continue;
                }
                if (lstat(file_list[i]->d_name, file_status) == -1) {
                        fprintf(stderr, "wsng: do_ls: error calling lstat\n");
                } 
                fprintf(fp,"<tr>");
                
                /* File name */
                fprintf(fp, "<td><a href=\"%s%s\">%s</a></td>", 
                        directory, file_list[i]->d_name, file_list[i]->d_name);
                
                /* Last modified */
                fprintf(fp,"<td>%s</td>", fmt_time(file_status->st_mtime, "%b %e %H:%M"));

                /* Size */
                fprintf(fp,"<td>%ld</td>", (long) file_status->st_size);
                fprintf(fp,"</tr>");
                free(file_list[i]);
                i++;
        }
        free(file_list);
        fprintf(fp,"</table></body></html>");
}

char *
fmt_time( time_t timeval , char *fmt )
/*
 * formats time for human consumption.
 * Uses localtime to convert the timeval into a struct of elements
 * (see localtime(3)) and uses strftime to format the data
 */
{
	static char	result[MAXDATELEN];

	struct tm *tp = localtime(&timeval);		/* convert time	*/
	strftime(result, MAXDATELEN, fmt, tp);		/* format it	*/
	return result;
}

/*
 * Function:
 *   do_exec (revised from original)
 * 
 * Purpose:
 *   To run the given cgi file, using the URL query parameters specified,
 *   and output the results to the client.
 * 
 * Input parameters
 *   file               char * string of file to run
 *   fp                 FILE * pointer to client
 *   parms              url query parms
 *   request_type       HTTP_REQ_GET or HTTP_REQ_HEAD
 * 
 * Return values:
 *   None.
 * 
 * Side effects:
 *   None.
 */
void
do_exec(char *file, FILE *fp, struct url_parm *parms, int request_type)
{
	int	fd = fileno(fp);

	print_http_resp_status(fp,
			       get_conf_value(CONF_HTTP_VERSION),
			       HTTP_RESP_STATUS_200);
        fflush(fp);
        if (request_type == HTTP_REQ_GET) {
                
                dup2(fd, 1);
                dup2(fd, 2);
                if (parms != NULL) {
                        setenv("QUERY_STRING", parms->value, 1);
                }
                execl(file, file, NULL);
                perror(file);
        }
}

/*
 * Function:
 *   do_cat (revised from original)
 * 
 * Purpose:
 *   To handle a GET or HEAD request for a file.
 *   to the client fp file pointer.
 *   (MD5 sum code based on sample code at
 *   http://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c)
 * 
 * Input parameters
 *   file               char * string of file to run
 *   fp                 FILE * pointer to client
 *   parms              url query parms
 *   request_type       HTTP_REQ_GET or HTTP_REQ_HEAD
 * 
 * Return values:
 *   None.
 * 
 * Side effects:
 *   None.
 */
void
do_cat(char *file, FILE *fpsock, int request_type)
{
	char	*extension = get_file_ext(file);
	FILE	*fpfile;
        unsigned char digest[MD5_DIGEST_LENGTH];
        int bytes;
        unsigned char data[1024];
        struct stat *file_status;
        
        if ((file_status = malloc(sizeof(struct stat))) == (struct stat *) NULL) {
                print_http_resp_status(fpsock,
                                       get_conf_value(CONF_HTTP_VERSION),
                                       HTTP_RESP_STATUS_500);
                fprintf(fpsock, "\r\n");
                fprintf(stderr, "wsng: do_cat: error calling malloc\n");
                return;
        }
        
        /* Get file's status for its size for Content-Length header */
        if (lstat(file, file_status) == -1) {
                print_http_resp_status(fpsock,
                                       get_conf_value(CONF_HTTP_VERSION),
                                       HTTP_RESP_STATUS_403);
                fprintf(fpsock, "\r\n");
                fprintf(stderr, "wsng: do_cat: error calling lstat\n");
                return;
        }
        
        /* Compute file's MD5 sum for Content-MD5 header */
        if (compute_MD5(file, digest) == -1) {
                print_http_resp_status(fpsock,
                                       get_conf_value(CONF_HTTP_VERSION),
                                       HTTP_RESP_STATUS_403);
                fprintf(fpsock, "\r\n");
                fprintf(stderr, "wsng: do_cat: error computing md5 sum\n");
                return;
        }
        
	fpfile = fopen(file, "r");
	if ( fpfile == NULL ) {
                print_http_resp_status(fpsock,
                                       get_conf_value(CONF_HTTP_VERSION),
                                       HTTP_RESP_STATUS_403);
        } else {
                
                /*
                 * At this point we know the request was either a GET or HEAD.
                 * 
                 * Print response headers.
                 */
		print_http_resp_status(fpsock,
				       get_conf_value(CONF_HTTP_VERSION),
				       HTTP_RESP_STATUS_200);
		print_http_header_content_type(fpsock,
					       get_content_type(extension));
                print_http_header_content_length(fpsock, (long) file_status->st_size);
                print_http_header_content_MD5(fpsock, digest);
                fprintf(fpsock, "\r\n");
                
                /* Output content if this was a GET request */
                if (request_type == HTTP_REQ_GET) {
                        while ((bytes = fread (data, 1, 1024, fpfile)) != 0) {
                                fwrite(data, 1, bytes, fpsock);
                        }
                }
		fclose(fpfile);
	}
}

/*
 * Function:
 *   compute_MD5
 * 
 * Purpose:
 *   To compute the MD5 sum for the specified file.
 *   (Code based on sample code at
 *   http://stackoverflow.com/questions/10324611/how-to-calculate-the-md5-hash-of-a-large-file-in-c)
 * 
 * Input parameters
 *   file               char * string of file to run
 *   fp                 FILE * pointer to client
 *   parms              url query parms
 *   request_type       HTTP_REQ_GET or HTTP_REQ_HEAD
 * 
 * Return values:
 *   None.
 * 
 * Side effects:
 *   None.
 */
int compute_MD5(char *file, unsigned char digest[MD5_DIGEST_LENGTH])
{
        int bytes;
        unsigned char data[1024];
        MD5_CTX mdContext;
        FILE *fp;
        
        if ((fp = fopen(file, "r")) == NULL) {
                return -1;
        } else {
                MD5_Init (&mdContext);
                while ((bytes = fread(data, 1, 1024, fp)) != 0) {
                        MD5_Update (&mdContext, data, bytes);
                }
                MD5_Final (digest, &mdContext);
        }
        fclose(fp);
        return 0;
}


void fatal(char *fmt, char *str)
{
	fprintf(stderr, fmt, str);
	exit(1);
}
