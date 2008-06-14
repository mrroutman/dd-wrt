/* milli_httpd - pretty small HTTP server
** A combination of
** micro_httpd - really small HTTP server
** and
** mini_httpd - small HTTP server
**
** Copyright  1999,2000 by Jef Poskanzer <jef@acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdarg.h>
#include <syslog.h>
#include <cy_conf.h>
#include "httpd.h"
#include <bcmnvram.h>
#include <code_pattern.h>
#include <utils.h>
#include <shutils.h>
#include <sys/time.h>

#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#endif

#ifdef HAVE_MATRIXSSL
# include <matrixSsl.h>
# include <matrixssl_xface.h>
#endif

#ifdef HAVE_VFS
#include <vfs.h>
#endif

#include <error.h>
#include <sys/signal.h>

#define SERVER_NAME "httpd"
//#define SERVER_PORT 80
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define TIMEOUT	15

/* A multi-family sockaddr. */
typedef union
{
  struct sockaddr sa;
  struct sockaddr_in sa_in;
} usockaddr;

/* Globals. */
#ifdef HAVE_OPENSSL
static SSL *ssl;
#endif

#ifdef FILTER_DEBUG
FILE *debout;
#endif

#if defined(HAVE_OPENSSL) || defined(HAVE_MATRIXSSL)

#define DEFAULT_HTTPS_PORT 443
#define CERT_FILE "/etc/cert.pem"
#define KEY_FILE "/etc/key.pem"
#endif

#ifdef HAVE_MATRIXSSL
extern ssl_t *ssl;
extern sslKeys_t *keys;
#endif

#define DEFAULT_HTTP_PORT 80
#ifdef HAVE_HTTPS
int do_ssl;
#endif
int server_port;
char pid_file[80];
char *server_dir = NULL;

#ifdef SAMBA_SUPPORT
extern int smb_getlock;
extern int httpd_fork;
void
smb_handler ()
{
  printf ("============child exit=====waitting ...====\n");
  smb_getlock = 0;
  wait (NULL);
  printf ("wait finish \n");
}
#endif

//static FILE *conn_fp;
static webs_t conn_fp;		// jimmy, https, 8/4/2003
static char auth_userid[AUTH_MAX];
static char auth_passwd[AUTH_MAX];
char auth_realm[AUTH_MAX];
//#ifdef GET_POST_SUPPORT
int post;
//#endif
int auth_fail = 0;
int httpd_level;

char http_client_ip[20];
extern char *get_mac_from_ip (char *ip);

/* Forwards. */
static int initialize_listen_socket (usockaddr * usaP);
static int auth_check (char *dirname, char *authorization);
static void send_error (int status, char *title, char *extra_header,
			char *text);
static void send_headers (int status, char *title, char *extra_header,
			  char *mime_type);
static int b64_decode (const char *str, unsigned char *space, int size);
static int match (const char *pattern, const char *string);
static int match_one (const char *pattern, int patternlen,
		      const char *string);
static void handle_request (void);

static int
initialize_listen_socket (usockaddr * usaP)
{
  int listen_fd;
  int i;

  memset (usaP, 0, sizeof (usockaddr));
  usaP->sa.sa_family = AF_INET;
  usaP->sa_in.sin_addr.s_addr = htonl (INADDR_ANY);
  usaP->sa_in.sin_port = htons (server_port);

  listen_fd = socket (usaP->sa.sa_family, SOCK_STREAM, 0);
  if (listen_fd < 0)
    {
      perror ("socket");
      return -1;
    }
  (void) fcntl (listen_fd, F_SETFD, 1);
  i = 1;
  if (setsockopt
      (listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &i, sizeof (i)) < 0)
    {
      perror ("setsockopt");
      return -1;
    }
  if (bind (listen_fd, &usaP->sa, sizeof (struct sockaddr_in)) < 0)
    {
      perror ("bind");
      return -1;
    }
  if (listen (listen_fd, 1024) < 0)
    {
      perror ("listen");
      return -1;
    }
  return listen_fd;
}

static int
auth_check (char *dirname, char *authorization)
{
  char authinfo[500];
  char *authpass;
  int l;

  /* Is this directory unprotected? */
  if (!strlen (auth_passwd))
    /* Yes, let the request go through. */
    return 1;

  /* Basic authorization info? */
  if (!authorization || strncmp (authorization, "Basic ", 6) != 0)
    {
      //send_authenticate( dirname );
      ct_syslog (LOG_INFO, httpd_level, "Authentication fail");
      return 0;
    }

  /* Decode it. */
  l = b64_decode (&(authorization[6]), authinfo, sizeof (authinfo));
  authinfo[l] = '\0';
  /* Split into user and password. */
  authpass = strchr (authinfo, ':');
  if (authpass == (char *) 0)
    {
      /* No colon?  Bogus auth info. */
      //send_authenticate( dirname );
      return 0;
    }
  *authpass++ = '\0';

  /* Is this the right user and password? */
//#ifdef DDM_SUPPORT
  char *crypt (const char *, const char *);

  char *enc1 = crypt (authinfo, auth_userid);
  if (strcmp (enc1, auth_userid))
    {
      return 0;
    }

  char *enc2 = crypt (authpass, auth_passwd);
  if (strcmp (enc2, auth_passwd))
    {
      return 0;
    }


  if (strcmp (enc1, auth_userid) == 0 && strcmp (enc2, auth_passwd) == 0)
    {
      return 1;
    }
//#else
//  if (strcmp (auth_passwd, authpass) == 0)
//    return 1;
//#endif
  //send_authenticate( dirname );
  return 1;
}


void
send_authenticate (char *realm)
{
  char header[10000];

  (void) snprintf (header, sizeof (header),
		   "WWW-Authenticate: Basic realm=\"%s\"", realm);
  send_error (401, "Unauthorized", header,
	      "Authorization required. please note that the default username is \"root\" in all newer releases");
}


static void
send_error (int status, char *title, char *extra_header, char *text)
{

  // jimmy, https, 8/4/2003, fprintf -> wfprintf, fflush -> wfflush
  send_headers (status, title, extra_header, "text/html");
  (void) wfprintf (conn_fp,
		   "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\n<BODY BGCOLOR=\"#cc9999\"><H4>%d %s</H4>\n",
		   status, title, status, title);
  (void) wfprintf (conn_fp, "%s\n", text);
  (void) wfprintf (conn_fp, "</BODY></HTML>\n");
  (void) wfflush (conn_fp);
}

static void
send_headers (int status, char *title, char *extra_header, char *mime_type)
{
  time_t now;
  char timebuf[100];

  // jimmy, https, 8/4/2003, fprintf -> wfprintf
  wfprintf (conn_fp, "%s %d %s\r\n", PROTOCOL, status, title);
  wfprintf (conn_fp, "Server: %s\r\n", SERVER_NAME);
  now = time ((time_t *) 0);
  strftime (timebuf, sizeof (timebuf), RFC1123FMT, gmtime (&now));
  wfprintf (conn_fp, "Date: %s\r\n", timebuf);
  if (extra_header != (char *) 0 && *extra_header)
    wfprintf (conn_fp, "%s\r\n", extra_header);
  if (mime_type != (char *) 0)
    wfprintf (conn_fp, "Content-Type: %s\r\n", mime_type);
  wfprintf (conn_fp, "Connection: close\r\n");
  wfprintf (conn_fp, "\r\n");
}


/* Base-64 decoding.  This represents binary data as printable ASCII
** characters.  Three 8-bit binary bytes are turned into four 6-bit
** values, like so:
**
**   [11111111]  [22222222]  [33333333]
**
**   [111111] [112222] [222233] [333333]
**
** Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
*/

static int b64_decode_table[256] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* 00-0F */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* 10-1F */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,	/* 20-2F */
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,	/* 30-3F */
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,	/* 40-4F */
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,	/* 50-5F */
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,	/* 60-6F */
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,	/* 70-7F */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* 80-8F */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* 90-9F */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* A0-AF */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* B0-BF */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* C0-CF */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* D0-DF */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,	/* E0-EF */
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1	/* F0-FF */
};

/* Do base-64 decoding on a string.  Ignore any non-base64 bytes.
** Return the actual number of bytes generated.  The decoded size will
** be at most 3/4 the size of the encoded, and may be smaller if there
** are padding characters (blanks, newlines).
*/
static int
b64_decode (const char *str, unsigned char *space, int size)
{
  const char *cp;
  int space_idx, phase;
  int d, prev_d = 0;
  unsigned char c;

  space_idx = 0;
  phase = 0;
  for (cp = str; *cp != '\0'; ++cp)
    {
      d = b64_decode_table[(int) *cp];
      if (d != -1)
	{
	  switch (phase)
	    {
	    case 0:
	      ++phase;
	      break;
	    case 1:
	      c = ((prev_d << 2) | ((d & 0x30) >> 4));
	      if (space_idx < size)
		space[space_idx++] = c;
	      ++phase;
	      break;
	    case 2:
	      c = (((prev_d & 0xf) << 4) | ((d & 0x3c) >> 2));
	      if (space_idx < size)
		space[space_idx++] = c;
	      ++phase;
	      break;
	    case 3:
	      c = (((prev_d & 0x03) << 6) | d);
	      if (space_idx < size)
		space[space_idx++] = c;
	      phase = 0;
	      break;
	    }
	  prev_d = d;
	}
    }
  return space_idx;
}


/* Simple shell-style filename matcher.  Only does ? * and **, and multiple
** patterns separated by |.  Returns 1 or 0.
*/
int
match (const char *pattern, const char *string)
{
  const char *or;
  for (;;)
    {
      or = strchr (pattern, '|');
      if (or == (char *) 0)
	return match_one (pattern, strlen (pattern), string);
      if (match_one (pattern, or - pattern, string))
	return 1;
      pattern = or + 1;
    }
}


static int
match_one (const char *pattern, int patternlen, const char *string)
{
  const char *p;

  for (p = pattern; p - pattern < patternlen; ++p, ++string)
    {
      if (*p == '?' && *string != '\0')
	continue;
      if (*p == '*')
	{
	  int i, pl;
	  ++p;
	  if (*p == '*')
	    {
	      /* Double-wildcard matches anything. */
	      ++p;
	      i = strlen (string);
	    }
	  else
	    /* Single-wildcard matches anything but slash. */
	    i = strcspn (string, "/");
	  pl = patternlen - (p - pattern);
	  for (; i >= 0; --i)
	    if (match_one (p, pl, &(string[i])))
	      return 1;
	  return 0;
	}
      if (*p != *string)
	return 0;
    }
  if (*string == '\0')
    return 1;
  return 0;
}


void
//do_file(char *path, FILE *stream)
do_file (char *path, webs_t stream, char *query)	//jimmy, https, 8/4/2003
{

#ifdef HAVE_VFS
  int c;
  entry *e;
  e = vfsopen (path, "r");
  if (e == NULL)
    {
      FILE *fp;
      if (!(fp = fopen (path, "rb")))
	return;
      while ((c = getc (fp)) != EOF)
	//fputc(c, stream);
	wfputc (c, stream);	// jimmy, https, 8/4/2003
      fclose (fp);
    }
  else
    {

      while ((c = vfsgetc (e)) != EOF)
	//fputc(c, stream);
	wfputc (c, stream);	// jimmy, https, 8/4/2003
      vfsclose (e);
    }
#else
  FILE *web = getWebsFile (path);
  if (web == NULL)
    {
      FILE *fp;
      int c;

      if (!(fp = fopen (path, "rb")))
	return;
      while ((c = getc (fp)) != EOF)
	wfputc (c, stream);	// jimmy, https, 8/4/2003
      fclose (fp);
    }
  else
    {
      int i;
      int len = getWebsFileLen (path);
      for (i = 0; i < len; i++)
	{
	  wfputc (getc (web), stream);
	}
      fclose (web);
    }
#endif
}

#ifdef HSIAB_SUPPORT
static char *			// add by jimmy 2003-5-13
get_aaa_url (int inout_mode, char *client_ip)
{
  static char line[MAX_BUF_LEN];
  char cmd[MAX_BUF_LEN];

  strcpy (line, "");
  if (inout_mode == 0)
    snprintf (cmd, sizeof (cmd), "GET aaa_login_url %s", client_ip);
  else
    snprintf (cmd, sizeof (cmd), "GET aaa_logout_url %s", client_ip);

  send_command (cmd, line);

  return line;
}

char *				// add by honor 2003-04-16, modify by jimmy 2003-05-13
get_client_ip (int conn_fp)
{
  struct sockaddr_in sa;
  int len = sizeof (struct sockaddr_in);
  static char ip[20];

  getpeername (conn_fp, (struct sockaddr *) &sa, &len);
  strcpy (ip, inet_ntoa (sa.sin_addr));
  return (ip);
}
#endif

static int
check_connect_type (void)
{
  struct wl_assoc_mac *wlmac = NULL;
  int count_wl = 0;
  int i;

  if (nvram_invmatch ("web_wl_filter", "1"))
    return 0;

  wlmac = get_wl_assoc_mac (&count_wl);

  for (i = 0; i < count_wl; i++)
    {
      if (!strcmp (wlmac[i].mac, nvram_safe_get ("http_client_mac")))
	{
	  cprintf ("Can't accept wireless access\n");
	  free (wlmac);
	  return -1;
	}
    }
  free (wlmac);

  return 0;
}

/*static void addEnv(const char *name_before_underline,
			const char *name_after_underline, const char *value)
{
  char *s = NULL;
  const char *underline;

  if (!value)
	value = "";
  underline = *name_after_underline ? "_" : "";
  asprintf(&s, "%s%s%s=%s", name_before_underline, underline,
					name_after_underline, value);
  if(s) {
    putenv(s);
  }
}*/
int
containsstring (char *source, char *cmp)
{
  if (cmp == NULL || source == NULL)
    return 0;
  int slen = strlen (source);
  int clen = strlen (cmp);
  if (slen < clen)
    return 0;
  int i;

  for (i = 0; i < slen - clen; i++)
    {
      int a;
      int r = 0;
      for (a = 0; a < clen; a++)
	if (source[i + a] != cmp[a])
	  r++;
      if (!r)
	return 1;
    }
  return 0;
}

static char *last_log_ip = NULL;
static int registered = -1;
#define LINE_LEN 10000
static void
handle_request (void)
{
  char *query;
  char *cur;
  char *method, *path, *protocol, *authorization, *boundary;
  char *cp;
  char *file;
  FILE *exec;
  int len;
  struct mime_handler *handler;
  int cl = 0, count, flags;
  char line[LINE_LEN];
  //   line =(char*)malloc(LINE_LEN);
  /* Initialize the request variables. */
  authorization = boundary = NULL;
  bzero (line, sizeof line);

  memset (line, 0, LINE_LEN);

  /* Parse the first line of the request. */
  if (wfgets (line, LINE_LEN, conn_fp) == (char *) 0)
    {				//jimmy,https,8/4/2003
      send_error (400, "Bad Request", (char *) 0, "No request found.");
//          free(line);
      return;
    }

  /* To prevent http receive https packets, cause http crash (by honor 2003/09/02) */
  if (strncasecmp (line, "GET", 3) && strncasecmp (line, "POST", 4))
    {
//      free(line);
      return;
    }

  method = path = line;
  strsep (&path, " ");
  if (!path)
    {				// Avoid http server crash, added by honor 2003-12-08
      send_error (400, "Bad Request", (char *) 0, "Can't parse request.");
//      free(line);
      return;
    }
  while (*path == ' ')
    path++;
  protocol = path;
  strsep (&protocol, " ");
  if (!protocol)
    {				// Avoid http server crash, added by honor 2003-12-08
      send_error (400, "Bad Request", (char *) 0, "Can't parse request.");
//      free(line);
      return;
    }
  while (*protocol == ' ')
    protocol++;
  cp = protocol;
  strsep (&cp, " ");
  cur = protocol + strlen (protocol) + 1;

  /* Parse the rest of the request headers. */
  //while ( fgets( cur, line + sizeof(line) - cur, conn_fp ) != (char*) 0 )
  //exec=fopen("/tmp/logweb.tmp","wb");

  while (wfgets (cur, line + LINE_LEN - cur, conn_fp) != 0)	//jimmy,https,8/4/2003
    {
      //    fwrite(cur,1,line + LINE_LEN - cur,exec);

      if (strcmp (cur, "\n") == 0 || strcmp (cur, "\r\n") == 0)
	{
	  break;
	}
      else if (strncasecmp (cur, "Authorization:", 14) == 0)
	{
	  cp = &cur[14];
	  cp += strspn (cp, " \t");
	  authorization = cp;
	  cur = cp + strlen (cp) + 1;
	}
      else if (strncasecmp (cur, "Content-Length:", 15) == 0)
	{
	  cp = &cur[15];
	  cp += strspn (cp, " \t");
	  cl = strtoul (cp, NULL, 0);
	  /*      if (cl<0 || cl>1024*1024*10)
	     {
	     send_error( 400, "Bad Request", (char*) 0, "bad try!!!" );
	     free(line);
	     return;
	     } */

	}
      else if ((cp = strstr (cur, "boundary=")))
	{
	  boundary = &cp[9];
	  for (cp = cp + 9; *cp && *cp != '\r' && *cp != '\n'; cp++);
	  *cp = '\0';
	  cur = ++cp;
	}

    }
  //fclose(exec);



  if (strcasecmp (method, "get") != 0 && strcasecmp (method, "post") != 0)
    {
      send_error (501, "Not Implemented", (char *) 0,
		  "That method is not implemented.");
//      free(line);
      return;
    }
  if (path[0] != '/')
    {
      send_error (400, "Bad Request", (char *) 0, "Bad filename.");
//      free(line);
      return;
    }
  file = &(path[1]);
  len = strlen (file);
  if (file[0] == '/' || strcmp (file, "..") == 0
      || strncmp (file, "../", 3) == 0 || strstr (file, "/../") != (char *) 0
      || strcmp (&(file[len - 3]), "/..") == 0)
    {
      send_error (400, "Bad Request", (char *) 0, "Illegal filename.");
//      free(line);
      return;
    }
  // seg change for status site
#ifdef HAVE_REGISTER
  if (registered == -1)
    registered = isregistered ();
#endif


#ifdef HAVE_SKYTRON
  if (file[0] == '\0' || file[len - 1] == '/')
    {
      file = "setupindex.asp";
    }
#else
  if (file[0] == '\0' || file[len - 1] == '/')
    {

      {
	if (server_dir != NULL && strcmp (server_dir, "/www"))	// to allow to use router as a WEB server
	  {
	    file = "index.htm";
	  }
	else
	  {
	    if (nvram_invmatch ("status_auth", "0"))
	      file = "Info.htm";
	    else
	      file = "index.asp";
	  }
      }
    }
  else
    {
      {
	if (nvram_invmatch ("status_auth", "1"))
	  if (strcmp (file, "Info.htm") == 0)
	    file = "index.asp";
      }
    }
#endif
  int changepassword = 0;

#ifdef HAVE_REGISTER
  if (!registered)
    {
      if (endswith (file, ".asp"))
	file = "register.asp";
      else if (endswith (file, ".htm"))
	file = "register.asp";
      else if (endswith (file, ".html"))
	file = "register.asp";
    }
  else
#endif
    {
      if ((nvram_match ("http_username", DEFAULT_USER)
	   && nvram_match ("http_passwd", DEFAULT_PASS))
	  || nvram_match ("http_username", "")
	  || nvram_match ("http_passwd", "admin"))
	{
	  changepassword = 1;
	  if (endswith (file, ".asp"))
	    file = "changepass.asp";
	  else if (endswith (file, ".htm"))
	    file = "changepass.asp";
	  else if (endswith (file, ".html"))
	    file = "changepass.asp";
	}
    }

  if (containsstring (file, "cgi-bin"))
    {

      do_auth (auth_userid, auth_passwd, auth_realm);
      auth_fail = 0;
      if (!auth_check (auth_realm, authorization))
	auth_fail = 1;
      query = NULL;
      if (strcasecmp (method, "post") == 0)
	{
	  query = malloc (10000);
	  if ((count = wfread (query, 1, cl, conn_fp)))
	    {
	      query[count] = '\0';;
	      cl -= strlen (query);
	    }
	  else
	    {
	      free (query);
	      query = NULL;
	    }
#if defined(linux)
#ifdef HAVE_HTTPS
	  if (!do_ssl && (flags = fcntl (fileno (conn_fp), F_GETFL)) != -1 &&
	      fcntl (fileno (conn_fp), F_SETFL, flags | O_NONBLOCK) != -1)
	    {
	      /* Read up to two more characters */
	      if (fgetc (conn_fp) != EOF)
		(void) fgetc (conn_fp);
	      fcntl (fileno (conn_fp), F_SETFL, flags);
	    }
#else
	  if ((flags = fcntl (fileno (conn_fp), F_GETFL)) != -1 &&
	      fcntl (fileno (conn_fp), F_SETFL, flags | O_NONBLOCK) != -1)
	    {
	      /* Read up to two more characters */
	      if (fgetc (conn_fp) != EOF)
		(void) fgetc (conn_fp);
	      fcntl (fileno (conn_fp), F_SETFL, flags);
	    }
#endif
#endif

	}
      exec = fopen ("/tmp/exec.tmp", "wb");
      fprintf (exec, "export REQUEST_METHOD=\"%s\"\n", method);
      if (query)
	fprintf (exec, "/bin/sh %s/%s</tmp/exec.query\n",
		 server_dir != NULL ? server_dir : "/www", file);
      else
	fprintf (exec, "/%s/%s\n", server_dir != NULL ? server_dir : "/www",
		 file);
      fclose (exec);

      if (query)
	{
	  exec = fopen ("/tmp/exec.query", "wb");
	  fprintf (exec, "%s\n", query);
	  free (query);
	  fclose (exec);
	}

      system2 ("chmod 700 /tmp/exec.tmp");
      system2 ("/tmp/exec.tmp>/tmp/shellout.asp");
      if (check_connect_type () < 0)
	{
	  send_error (401, "Bad Request", (char *) 0,
		      "Can't use wireless interface to access web.");
//          free(line);
	  return;
	}
      if (auth_fail == 1)
	{
	  send_authenticate (auth_realm);
	  auth_fail = 0;
//          free(line);
	  return;
	}

      do_ej ("/tmp/shellout.asp", conn_fp, "");
      unlink ("/tmp/shellout.asp");
      unlink ("/tmp/exec.tmp");
      unlink ("/tmp/exec.query");

    }
  else
    {
      /* extract url args if present */
      query = strchr (file, '?');
      if (query)
	{
	  *query++ = 0;
	}
      for (handler = &mime_handlers[0]; handler->pattern; handler++)
	{
	  if (match (handler->pattern, file))
	    {
#ifdef HAVE_REGISTER
	      if (registered)
#endif
		if (!changepassword && handler->auth)
		  {
		    int result =
		      handler->auth (auth_userid, auth_passwd, auth_realm);
		    if (result == 0)
		      {
			auth_fail = 0;
			if (!auth_check (auth_realm, authorization))
			  {
			    send_authenticate (auth_realm);
//                        syslog(LOG_INFO,"%s fails web authentication\n",nvram_safe_get("http_client_ip"));
//                        lcdmessaged(nvram_safe_get("http_client_ip"),"fails authentication!!!");
			    return;
			    //auth_fail = 1;
			  }
//                      if (last_log_ip==NULL || strcmp(nvram_safe_get("http_client_ip"),last_log_ip))
//                        {
//                        last_log_ip=nvram_safe_get("http_client_ip");
//                        lcdmessaged(nvram_safe_get("http_client_ip"),"logged in!!!");
//                        syslog(LOG_INFO,"%s successfully authenticated\n",nvram_safe_get("http_client_ip"));
//                        }
		      }
		  }
	      post = 0;
	      if (strcasecmp (method, "post") == 0)
		{
		  post = 1;
		}
	      if (handler->input)
		handler->input (file, conn_fp, cl, boundary);
#if defined(linux)
#ifdef HAVE_HTTPS
	      if (!do_ssl && (flags = fcntl (fileno (conn_fp), F_GETFL)) != -1
		  && fcntl (fileno (conn_fp), F_SETFL,
			    flags | O_NONBLOCK) != -1)
		{
		  /* Read up to two more characters */
		  if (fgetc (conn_fp) != EOF)
		    (void) fgetc (conn_fp);
		  fcntl (fileno (conn_fp), F_SETFL, flags);
		}
#else
	      if ((flags = fcntl (fileno (conn_fp), F_GETFL)) != -1 &&
		  fcntl (fileno (conn_fp), F_SETFL, flags | O_NONBLOCK) != -1)
		{
		  /* Read up to two more characters */
		  if (fgetc (conn_fp) != EOF)
		    (void) fgetc (conn_fp);
		  fcntl (fileno (conn_fp), F_SETFL, flags);
		}
#endif
#endif
	      if (check_connect_type () < 0)
		{
		  send_error (401, "Bad Request", (char *) 0,
			      "Can't use wireless interface to access web.");
//                          free(line);
		  return;
		}

	      if (auth_fail == 1)
		{
		  send_authenticate (auth_realm);
		  auth_fail = 0;
//                          free(line);
		  return;
		}
	      else
		{
		  send_headers (200, "Ok", handler->extra_header,
				handler->mime_type);
		}
	      if (handler->output)
		{
		  handler->output (file, conn_fp, query);
		}
	      break;
	    }

	  if (!handler->pattern)
	    send_error (404, "Not Found", (char *) 0, "File not found.");
	}

    }
//    free(line);
}

void				// add by honor 2003-04-16
get_client_ip_mac (int conn_fp)
{
  struct sockaddr_in sa;
  int len = sizeof (struct sockaddr_in);
  char *m;

  getpeername (conn_fp, (struct sockaddr *) &sa, &len);
  nvram_safe_set ("http_client_ip", inet_ntoa (sa.sin_addr));
  //if(strcmp(nvram_safe_get("http_client_ip"), inet_ntoa(sa.sin_addr)))
  //      nvram_set("http_client_ip", inet_ntoa(sa.sin_addr));
  //fprintf(stderr, "Client ip=[%s]\n", inet_ntoa(sa.sin_addr));
  m = get_mac_from_ip (inet_ntoa (sa.sin_addr));
  nvram_safe_set ("http_client_mac", m);
  //if(m){
  //      if(strcmp(nvram_safe_get("http_client_mac"), m))
  //              nvram_set("http_client_mac", m);
  //}
}

static void
handle_server_sig_int (int sig)
{
#ifdef HAVE_HTTPS
  ct_syslog (LOG_INFO, httpd_level, "httpd server %sshutdown",
	     do_ssl ? "(ssl support) " : "");
#else
  ct_syslog (LOG_INFO, httpd_level, "httpd server shutdown");
#endif
  exit (0);
}

void
settimeouts (int sock, int secs)
{
  struct timeval tv;

  tv.tv_sec = secs;
  tv.tv_usec = 0;
  if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof (tv)) < 0)
    perror ("setsockopt(SO_SNDTIMEO)");
  if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv)) < 0)
    perror ("setsockopt(SO_RCVTIMEO)");
}

/****************************************************************************
 *
 > $Function: decodeString()
 *
 * $Description: Given a URL encoded string, convert it to plain ascii.
 *   Since decoding always makes strings smaller, the decode is done in-place.
 *   Thus, callers should strdup() the argument if they do not want the
 *   argument modified.  The return is the original pointer, allowing this
 *   function to be easily used as arguments to other functions.
 *
 * $Parameters:
 *      (char *) string . . . The first string to decode.
 *      (int)    flag   . . . 1 if require decode '+' as ' ' for CGI
 *
 * $Return: (char *)  . . . . A pointer to the decoded string (same as input).
 *
 * $Errors: None
 *
 ****************************************************************************/
static char *
decodeString (char *orig, int flag_plus_to_space)
{
  /* note that decoded string is always shorter than original */
  char *string = orig;
  char *ptr = string;

  while (*ptr)
    {
      if (*ptr == '+' && flag_plus_to_space)
	{
	  *string++ = ' ';
	  ptr++;
	}
      else if (*ptr != '%')
	*string++ = *ptr++;
      else
	{
	  unsigned int value;
	  sscanf (ptr + 1, "%2X", &value);
	  *string++ = value;
	  ptr += 3;
	}
    }
  *string = '\0';
  return orig;
}

/****************************************************************************
 *
 > $Function: encodeString()
 *
 * $Description: Given a string, html encode special characters.
 *   This is used for the -e command line option to provide an easy way
 *   for scripts to encode result data without confusing browsers.  The
 *   returned string pointer is memory allocated by malloc().
 *
 * $Parameters:
 *      (const char *) string . . The first string to encode.
 *
 * $Return: (char *) . . . .. . . A pointer to the encoded string.
 *
 * $Errors: Returns a null string ("") if memory is not available.
 *
 ****************************************************************************/
static char *
encodeString (const char *string)
{
  /* take the simple route and encode everything */
  /* could possibly scan once to get length.     */
  int len = strlen (string);
  char *out = malloc (len * 5 + 1);
  char *p = out;
  char ch;

  if (!out)
    return "";
  while ((ch = *string++))
    {
      // very simple check for what to encode

      if (ch > '0' - 1 && ch < '9' + 1)
	*p++ = ch;
      else if (ch > 'a' - 1 && ch < 'z' + 1)
	*p++ = ch;
      else if (ch > 'A' - 1 && ch < 'Z' + 1)
	*p++ = ch;
      else
	p += sprintf (p, "&#%d;", (unsigned char) ch);
    }
  *p = 0;
  return out;
}

int
main (int argc, char **argv)
{
  usockaddr usa;
  int listen_fd;
  int conn_fd;
  socklen_t sz = sizeof (usa);
  int c;
  int timeout = TIMEOUT;
  struct stat stat_dir;
//#ifdef HAVE_MATRIXSSL
//  if (strstr(argv[0],"httpsclient"))
//    return https_main(argc,argv);
//#endif
#ifdef FILTER_DEBUG
  debout = fopen ("/tmp/filterdebug.log", "wb");
#endif
  cprintf ("init nvram tab\n");
/* SEG addition */
  Initnvramtab ();
  cprintf ("done()\n");
#ifdef HAVE_OPENSSL
  BIO *sbio;
  SSL_CTX *ctx = NULL;
  int r;
  BIO *ssl_bio;
#endif

  strcpy (pid_file, "/var/run/httpd.pid");
  server_port = DEFAULT_HTTP_PORT;

  while ((c = getopt (argc, argv, "Sih:p:d:t:s:g:e:")) != -1)
    switch (c)
      {
#ifdef HAVE_HTTPS
      case 'S':
#if defined(HAVE_OPENSSL) || defined(HAVE_MATRIXSSL)
	do_ssl = 1;
	server_port = DEFAULT_HTTPS_PORT;
	strcpy (pid_file, "/var/run/httpsd.pid");
#else
	fprintf (stderr, "No SSL support available\n");
	exit (0);
#endif
	break;
#endif
      case 'h':
	server_dir = optarg;
	break;
      case 'p':
	server_port = atoi (optarg);
	break;
      case 't':
	timeout = atoi (optarg);
	break;
#ifdef DEBUG_CIPHER
      case 's':
	set_ciphers = optarg;
	break;
      case 'g':
	get_ciphers = 1;
	break;
#endif
      case 'e':
	printf ("%s", encodeString (optarg));
	exit (0);
	break;
      case 'd':
	printf ("%s", decodeString (optarg, 1));
	exit (0);
	break;
      case 'i':
	fprintf (stderr, "Usage: %s [-S] [-p port]\n"
#ifdef HAVE_HTTPS
		 "	-S : Support https (port 443)\n"
#endif
		 "	-p port : Which port to listen?\n"
		 "	-t secs : How many seconds to wait before timing out?\n"
		 "	-s ciphers: set cipher lists\n"
		 "	-g: get cipher lists\n"
		 "	-h: home directory: use directory\n"
		 "	-d: decode string\n"
		 "	-e: encode string\n", argv[0]);
	exit (0);
	break;
      default:
	break;
      }


  httpd_level =
    ct_openlog ("httpd", LOG_PID | LOG_NDELAY, LOG_DAEMON, "LOG_HTTPD");
#ifdef HAVE_HTTPS
  ct_syslog (LOG_INFO, httpd_level, "httpd server %sstarted at port %d\n",
	     do_ssl ? "(ssl support) " : "", server_port);
#else
  ct_syslog (LOG_INFO, httpd_level, "httpd server started at port %d\n",
	     server_port);
#endif
  /* Ignore broken pipes */
  signal (SIGPIPE, SIG_IGN);
  signal (SIGTERM, handle_server_sig_int);	// kill

  if (server_dir && stat (server_dir, &stat_dir) == 0)
    chdir (server_dir);

  /* Build our SSL context */
#ifdef HAVE_HTTPS
  if (do_ssl)
    {
#ifdef HAVE_OPENSSL
      SSLeay_add_ssl_algorithms ();
      SSL_load_error_strings ();
      ctx = SSL_CTX_new (SSLv23_server_method ());
      if (SSL_CTX_use_certificate_file (ctx, CERT_FILE, SSL_FILETYPE_PEM) ==
	  0)
	{
	  cprintf ("Can't read %s\n", CERT_FILE);
	  ERR_print_errors_fp (stderr);
	  exit (1);

	}
      if (SSL_CTX_use_PrivateKey_file (ctx, KEY_FILE, SSL_FILETYPE_PEM) == 0)
	{
	  cprintf ("Can't read %s\n", KEY_FILE);
	  ERR_print_errors_fp (stderr);
	  exit (1);

	}
      if (SSL_CTX_check_private_key (ctx) == 0)
	{
	  cprintf ("Check private key fail\n");
	  ERR_print_errors_fp (stderr);
	  exit (1);
	}
#endif

#ifdef HAVE_MATRIXSSL
      matrixssl_init ();
      if (0 != matrixSslReadKeys (&keys, CERT_FILE, KEY_FILE, NULL, NULL))
	{
	  fprintf (stderr, "Error reading or parsing %s.\n", KEY_FILE);
	  exit (0);
	}
#endif
    }
#endif

  /* Initialize listen socket */
  if ((listen_fd = initialize_listen_socket (&usa)) < 0)
    {
      ct_syslog (LOG_ERR, httpd_level, "Can't bind to any address");
      exit (errno);
    }

#if !defined(DEBUG)
  {
    FILE *pid_fp;
    /* Daemonize and log PID */
    if (daemon (1, 1) == -1)
      {
	perror ("daemon");
	exit (errno);
      }
    if (!(pid_fp = fopen (pid_file, "w")))
      {
	perror (pid_file);
	return errno;
      }
    fprintf (pid_fp, "%d", getpid ());
    fclose (pid_fp);
  }
#endif

  /* Loop forever handling requests */
  for (;;)
    {
      if ((conn_fd = accept (listen_fd, &usa.sa, &sz)) < 0)
	{
	  perror ("accept");
	  return errno;
	}

      /* Make sure we don't linger a long time if the other end disappears */
      settimeouts (conn_fd, timeout);

      if (check_action () == ACT_TFTP_UPGRADE ||	// We don't want user to use web during tftp upgrade.
	  check_action () == ACT_SW_RESTORE ||
	  check_action () == ACT_HW_RESTORE)
	{
	  fprintf (stderr, "http(s)d: nothing to do...\n");
	  return -1;
	}
#ifdef HAVE_HTTPS
      if (do_ssl)
	{
	  if (check_action () == ACT_WEB_UPGRADE)
	    {			// We don't want user to use web (https) during web (http) upgrade.
	      fprintf (stderr, "httpsd: nothing to do...\n");
	      return -1;
	    }
#ifdef HAVE_OPENSSL
	  sbio = BIO_new_socket (conn_fd, BIO_NOCLOSE);
	  ssl = SSL_new (ctx);

//#ifdef DEBUG_CIPHER
//              check_cipher();
//#endif
	  SSL_set_bio (ssl, sbio, sbio);

	  if ((r = SSL_accept (ssl) <= 0))
	    {
	      //berr_exit("SSL accept error");
	      ct_syslog (LOG_ERR, httpd_level, "SSL accept error");
	      close (conn_fd);
	      continue;
	    }

	  conn_fp = (webs_t) BIO_new (BIO_f_buffer ());
	  ssl_bio = BIO_new (BIO_f_ssl ());
	  BIO_set_ssl (ssl_bio, ssl, BIO_CLOSE);
	  BIO_push ((BIO *) conn_fp, ssl_bio);
#elif defined(HAVE_MATRIXSSL)
	  matrixssl_new_session (conn_fd);
	  conn_fp = (FILE *) conn_fd;
#endif
	}
      else
#endif
	{
#ifdef HAVE_HTTPS
	  if (check_action () == ACT_WEBS_UPGRADE)
	    {			// We don't want user to use web (http) during web (https) upgrade.
	      fprintf (stderr, "httpd: nothing to do...\n");
	      return -1;
	    }
#endif
	  if (!(conn_fp = fdopen (conn_fd, "r+")))
	    {
	      perror ("fdopen");
	      return errno;
	    }
	}
      get_client_ip_mac (conn_fd);
      handle_request ();
      wfflush (conn_fp);	// jimmy, https, 8/4/2003
      wfclose (conn_fp);	// jimmy, https, 8/4/2003
      close (conn_fd);
    }

  shutdown (listen_fd, 2);
  close (listen_fd);

  return 0;
}

char *
wfgets (char *buf, int len, FILE * fp)
{
#ifdef HAVE_HTTPS
#ifdef HAVE_OPENSSL
  if (do_ssl)
    return (char *) BIO_gets ((BIO *) fp, buf, len);
  else
#elif defined(HAVE_MATRIXSSL)
  if (do_ssl)
    return (char *) matrixssl_gets (fp, buf, len);
  else
#endif
#endif
    return fgets (buf, len, fp);
}

int
wfputc (char c, FILE * fp)
{
#ifdef HAVE_HTTPS
#ifdef HAVE_OPENSSL
  if (do_ssl)
    return BIO_printf ((BIO *) fp, "%c", c);
  else
#elif defined(HAVE_MATRIXSSL)
  if (do_ssl)
    return matrixssl_putc (fp, c);
  else
#endif
#endif
    return fputc (c, fp);
}

int
wfputs (char *buf, FILE * fp)
{
#ifdef HAVE_HTTPS
#ifdef HAVE_OPENSSL
  if (do_ssl)
    return BIO_puts ((BIO *) fp, buf);
  else
#elif defined(HAVE_MATRIXSSL)
  if (do_ssl)
    return matrixssl_puts (fp, buf);
  else
#endif
#endif
    return fputs (buf, fp);
}

int
wfprintf (FILE * fp, char *fmt, ...)
{
  va_list args;
  char buf[1024];
  int ret;
  //buf = (char*)malloc(1024);
  va_start (args, fmt);
  vsnprintf (buf, sizeof (buf), fmt, args);
#ifdef HAVE_HTTPS
#ifdef HAVE_OPENSSL
  if (do_ssl)
    ret = BIO_printf ((BIO *) fp, "%s", buf);
  else
#elif defined(HAVE_MATRIXSSL)
  if (do_ssl)
    ret = matrixssl_printf (fp, "%s", buf);
  else
#endif
#endif
    ret = fprintf (fp, "%s", buf);
  va_end (args);
  //free(buf);
  return ret;
}

int
websWrite (webs_t wp, char *fmt, ...)
{
  va_list args;
  char buf[1024];
  int ret;
  FILE *fp = wp;
  if (!wp || !fmt)
    return -1;
  //buf = (char*)malloc(1024);
  va_start (args, fmt);
  vsnprintf (buf, sizeof (buf), fmt, args);
#ifdef HAVE_HTTPS
#ifdef HAVE_OPENSSL
  if (do_ssl)
    ret = BIO_printf ((BIO *) fp, "%s", buf);
  else
#elif defined(HAVE_MATRIXSSL)
  if (do_ssl)
    ret = matrixssl_printf (fp, "%s", buf);
  else
#endif
#endif
    ret = fprintf (fp, "%s", buf);
  va_end (args);
  wfflush (wp);
  return ret;
}

size_t
wfwrite (char *buf, int size, int n, FILE * fp)
{
#ifdef HAVE_HTTPS
#ifdef HAVE_OPENSSL
  if (do_ssl)
    return BIO_write ((BIO *) fp, buf, n * size);
  else
#elif defined(HAVE_MATRIXSSL)
  if (do_ssl)
    return matrixssl_write (fp, buf, n * size);
  else
#endif
#endif
    return fwrite (buf, size, n, fp);
}

size_t
wfread (char *buf, int size, int n, FILE * fp)
{
#ifdef HAVE_HTTPS
#ifdef HAVE_OPENSSL
  if (do_ssl)
    return BIO_read ((BIO *) fp, buf, n * size);
  else
#elif defined(HAVE_MATRIXSSL)
  if (do_ssl)
    {
      //do it in chains
      int cnt = (size * n) / 0x4000;
      int i;
      int len = 0;
      for (i = 0; i < cnt; i++)
	{
	  len += matrixssl_read (fp, buf, 0x4000);
	  *buf += 0x4000;
	}
      len += matrixssl_read (fp, buf, (size * n) % 0x4000);

      return len;
    }
  else
#endif
#endif
    return fread (buf, size, n, fp);
}

int
wfflush (FILE * fp)
{
#ifdef HAVE_HTTPS
#ifdef HAVE_OPENSSL
  if (do_ssl)
    {
      BIO_flush ((BIO *) fp);
      return 1;
    }
  else
#elif defined(HAVE_MATRIXSSL)
  if (do_ssl)
    return matrixssl_flush (fp);
  else
#endif
#endif
    return fflush (fp);
}

int
wfclose (FILE * fp)
{
#ifdef HAVE_HTTPS
#ifdef HAVE_OPENSSL
  if (do_ssl)
    {
      BIO_free_all ((BIO *) fp);
      return 1;
    }
  else
#elif defined(HAVE_MATRIXSSL)
  if (do_ssl)
    return matrixssl_free_session (fp);
  else
#endif
#endif
    return fclose (fp);
}

#ifdef DEBUG_CIPHER
void
check_cipher (void)
{
  STACK_OF (SSL_CIPHER) * sk;
  char buf[512];
  BIO *STDout = NULL;
  int i;
  static BIO *bio_stdout = NULL;
  X509 *peer;
  static BIO *bio_s_out = NULL;
  SSL_CIPHER *ciph;

  if (set_ciphers)
    {
      /* Set supported cipher lists */
      SSL_set_cipher_list (ssl, set_ciphers);
    }
  if (get_ciphers)
    {
      /* Show supported cipher lists */
      sk = SSL_get_ciphers (ssl);

      for (i = 0; i < sk_SSL_CIPHER_num (sk); i++)
	{
	  BIO_puts (STDout,
		    SSL_CIPHER_description (sk_SSL_CIPHER_value (sk, i), buf,
					    512));
	  printf ("%d: %s", i, buf);
	}
      if (STDout != NULL)
	BIO_free_all (STDout);
    }
  peer = SSL_get_peer_certificate (ssl);
  if (peer != NULL)
    {
      BIO_printf (bio_s_out, "Client certificate\n");
      PEM_write_bio_X509 (bio_s_out, peer);
      X509_NAME_oneline (X509_get_subject_name (peer), buf, sizeof buf);
      BIO_printf (bio_s_out, "subject=%s\n", buf);
      X509_NAME_oneline (X509_get_issuer_name (peer), buf, sizeof buf);
      BIO_printf (bio_s_out, "issuer=%s\n", buf);
      X509_free (peer);
    }

  if (SSL_get_shared_ciphers (ssl, buf, sizeof buf) != NULL)
    BIO_printf (bio_s_out, "Shared ciphers:%s\n", buf);

  bio_stdout = BIO_new_fp (stdout, BIO_NOCLOSE);
  ciph = SSL_get_current_cipher (ssl);
  BIO_printf (bio_stdout, "%s%s, cipher %s %s\n",
	      "",
	      SSL_get_version (ssl),
	      SSL_CIPHER_get_version (ciph), SSL_CIPHER_get_name (ciph));
}
#endif



char *
websGetVar (webs_t wp, char *var, char *d)
{
  char *ret = get_cgi (var) ? : d;
  return ret;
}
