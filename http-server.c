/*
  A trivial static http webserver using Libevent's evhttp.

  This is not the best code in the world, and it does some fairly stupid stuff
  that you would never want to do in a production webserver. Caveat hackor!

 */

/* Compatibility for possible missing IPv6 declarations */
#include "../util-internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <getopt.h>
#include <io.h>
#include <fcntl.h>
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#else /* !_WIN32 */
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif /* _WIN32 */
#include <signal.h>

#ifdef EVENT__HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef EVENT__HAVE_AFUNIX_H
#include <afunix.h>
#endif

#include <event2/event.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#ifdef _WIN32
#include <event2/thread.h>
#endif /* _WIN32 */

#ifdef EVENT__HAVE_NETINET_IN_H
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#endif

#ifdef _WIN32
#ifndef stat
#define stat _stat
#endif
#ifndef fstat
#define fstat _fstat
#endif
#ifndef open
#define open _open
#endif
#ifndef close
#define close _close
#endif
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif
#endif /* _WIN32 */

char uri_root[512];

static const struct table_entry {
	const char *extension;
	const char *content_type;
} content_type_table[] = {
	{ "txt", "text/plain" },
	{ "c", "text/plain" },
	{ "h", "text/plain" },
	{ "html", "text/html" },
	{ "htm", "text/htm" },
	{ "css", "text/css" },
	{ "gif", "image/gif" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "png", "image/png" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postscript" },
	{ NULL, NULL },
};

struct options
{
	int port;
	int iocp;
	int verbose;

	int unlink;
	const char *unixsock;
};

/* Try to guess a good content-type for 'path' */
static const char *
guess_content_type(const char *path)
{
	const char *last_period, *extension;
	const struct table_entry *ent;
	last_period = strrchr(path, '.');
	if (!last_period || strchr(last_period, '/'))
		goto not_found; /* no exension */
	extension = last_period + 1;
	for (ent = &content_type_table[0]; ent->extension; ++ent) {
		if (!evutil_ascii_strcasecmp(ent->extension, extension))
			return ent->content_type;
	}

not_found:
	return "application/misc";
}

enum echonet_cmd_type {
    ECHO_REQ_NULL  =  0,
	ECHO_REQ_AHTH   = 1 << 0,
	ECHO_REQ_PUT    = 1 << 1,
	ECHO_REQ_GET    = 1 << 2,
	ECHO_REQ_SET    = 1 << 3,
	ECHO_REQ_FILE   = 1 << 4,
	ECHO_REQ_UPLOAD = 1 << 5,
	ECHO_REQ_MEMB   = 1 << 6,
    ECHO_REQ_END    = 1 << 7,
};


void print_output(struct evhttp_request *req)
{
    const char *cmdtype;
	struct evkeyvalq *headers;
	struct evkeyval *header;
	struct evbuffer *buf;
    struct evbuffer *evb = evbuffer_new();
	// enum echonet_cmd_type ret = ECHO_REQ_NULL;
    char cbuf[2048];
    int n;
  
	buf = evhttp_request_get_output_buffer(req);
    n = evbuffer_get_length(buf);
    printf("\n n = %d ",n);
    printf("\n buf = %s ",buf);	
    // while (evbuffer_get_length(buf)) {
	
	// 	n = evbuffer_copyout(buf, cbuf, sizeof(cbuf));
	// 	if (n > 0) {
    //         (void) fwrite(cbuf, 1, n, stdout);
    //     }
	// }

}
enum echonet_cmd_type echo_request_get_command(const struct evhttp_request *req) 
{
    const char *cmdtype;
	struct evkeyvalq *headers;
	struct evkeyval *header;
	struct evbuffer *buf;
    struct evbuffer *evb = evbuffer_new();
	enum echonet_cmd_type ret = ECHO_REQ_NULL;
    char cbuf[2048];
  
	buf = evhttp_request_get_input_buffer(req);
	while (evbuffer_get_length(buf)) {
		int n;

		n = evbuffer_remove(buf, cbuf, sizeof(cbuf));
		if (n > 0) {
            (void) fwrite(cbuf, 1, n, stdout);
            if(NULL != strstr(cbuf,"auth")) {
                ret = ECHO_REQ_AHTH;
                goto END;
            } else if (NULL != strstr(cbuf,"memb")) {
                ret = ECHO_REQ_MEMB;
                goto END;
            } else if (NULL != strstr(cbuf,"put")) {
                ret = ECHO_REQ_PUT;
                goto END;
            } else if (NULL != strstr(cbuf,"get")) {
                ret = ECHO_REQ_GET;
                goto END;
            } else if (NULL != strstr(cbuf,"set")) {
                ret = ECHO_REQ_SET;
                goto END;
            } 
        }
	}
END:
    return ret;
}

//body
void proc_auth(struct evhttp_request *req,struct evbuffer *respayload)
{
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=Shift_JIS");
    evhttp_add_header(evhttp_request_get_output_headers(req),
        "Keep-Alive", "timeout=600, max=68");
    evhttp_add_header(evhttp_request_get_output_headers(req),
    "Set-Cookie", "JSESSIONID=9CF07B056A5A1B788794A6D69BA249B9; Path=/CenterServer/; HttpOnly");

    evbuffer_add_printf(respayload, "%s", "<monitoring><cmd>memb</cmd><data><member>1</member></data></monitoring>");

    evhttp_send_reply(req, HTTP_OK, "OK", respayload);

}

void proc_memb(struct evhttp_request *req,struct evbuffer *respayload)
{
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=Shift_JIS");
    evbuffer_add_printf(respayload, "%s", "<monitoring><cmd>put</cmd><data><put><set_time>19,09,30,14,40,47</set_time><next_comm2>19,09,30,14,41</next_comm2><next_interval>0A</next_interval><retry_interval>0B</retry_interval><repeat_interval>0C</repeat_interval><center_addr>emstest2.eliiypower.co.jp</center_addr></put></data></monitoring>");
    evhttp_send_reply(req, HTTP_OK, "OK", respayload);
}

void proc_put(struct evhttp_request *req,struct evbuffer *respayload)
{
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=Shift_JIS");
    evbuffer_add_printf(respayload, "%s", "<monitoring><cmd>get</cmd><data><get><time>19,09,30,14,40</time></get></data></monitoring>");
    evhttp_send_reply(req, HTTP_OK, "OK", respayload);
}

void proc_get(struct evhttp_request *req,struct evbuffer *respayload)
{
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=Shift_JIS");
    evbuffer_add_printf(respayload, "%s", "<monitoring><cmd>set</cmd><data><set><bat_set><bat_time>010a1f3f</bat_time><st_xda>2</st_xda><st_xf0>3</st_xf0><st_xf4>4</st_xf4><st_xf5>5</st_xf5><st_xf6>6</st_xf6><re_timer>7</re_timer><re_xda>8</re_xda><xda>41</xda><xf0>0</xf0><xf4>1</xf4><xf5>2</xf5><xf6>3</xf6></bat_set><elewh_set><elewh_timer>4</elewh_timer><st_xb0>5</st_xb0><xb0>43</xb0><xb5>07,10</xb5><xc0>42</xc0><xc7>01</xc7><xca>0B</xca></elewh_set><hybwh_set><hybwh_timer>1</hybwh_timer><st_xb0>2</st_xb0><xb0>3</xb0><xb8>4</xb8><xb9>5</xb9></hybwh_set><app_set>6</app_set></set></data></monitoring>");
    evhttp_send_reply(req, HTTP_OK, "OK", respayload);
}
void proc_set(struct evhttp_request *req,struct evbuffer *respayload)
{
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=Shift_JIS");
    evbuffer_add_printf(respayload, "%s", "<monitoring><cmd>file</cmd><data><file><apli>3_1001_BMU.mot</apli><crc_code>AFFF</crc_code></file></data></monitoring>");
    evhttp_send_reply(req, HTTP_OK, "OK", respayload);
}

void proc_file(struct evhttp_request *req,struct evbuffer *respayload)
{
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/octet-stream");
    evbuffer_add_printf(respayload, "%s", "I2luY2x1ZGUJInVucGlwYy5oIgoKc3RydWN0IG1xX2F0dHIJYXR0cjsJLyogbXFfbWF4bXNnIGFuZCBtcV9tc2dzaXplIGJvdGggaW5pdCB0byAwICovCgppbnQKbWFpbihpbnQgYXJnYywgY2hhciAqKmFyZ3YpCnsKCWludAkJYywgZmxhZ3M7CgltcWRfdAltcWQ7CgoJZmxhZ3MgPSBPX1JEV1IgfCBPX0NSRUFUOwoJd2hpbGUgKCAoYyA9IEdldG9wdChhcmdjLCBhcmd2LCAiZW06ejoiKSkgIT0gLTEpIHsKCQlzd2l0Y2ggKGMpIHsKCQljYXNlICdlJzoKCQkJZmxhZ3MgfD0gT19FWENMOwoJCQlicmVhazsKCgkJY2FzZSAnbSc6CgkJCWF0dHIubXFfbWF4bXNnID0gYXRvbChvcHRhcmcpOwoJCQlicmVhazsKCgkJY2FzZSAneic6CgkJCWF0dHIubXFfbXNnc2l6ZSA9IGF0b2wob3B0YXJnKTsKCQkJYnJlYWs7CgkJfQoJfQoJaWYgKG9wdGluZCAhPSBhcmdjIC0gMSkKCQllcnJfcXVpdCgidXNhZ2U6IG1xY3JlYXRlIFsgLWUgXSBbIC1tIG1heG1zZyAteiBtc2dzaXplIF0gPG5hbWU+Iik7CgoJaWYgKChhdHRyLm1xX21heG1zZyAhPSAwICYmIGF0dHIubXFfbXNnc2l6ZSA9PSAwKSB8fAoJCShhdHRyLm1xX21heG1zZyA9PSAwICYmIGF0dHIubXFfbXNnc2l6ZSAhPSAwKSkKCQllcnJfcXVpdCgibXVzdCBzcGVjaWZ5IGJvdGggLW0gbWF4bXNnIGFuZCAteiBtc2dzaXplIik7CgoJcHJpbnRmKCJhYm91dCB0byBzbGVlcCBmb3IgMzAgc2Vjb25kcyBiZWZvcmUgbXFfb3BlblxuIik7CglzbGVlcCgxKTsKCgltcWQgPSBNcV9vcGVuKGFyZ3Zbb3B0aW5kXSwgZmxhZ3MsIEZJTEVfTU9ERSwKCQkJCSAgKGF0dHIubXFfbWF4bXNnICE9IDApID8gJmF0dHIgOiBOVUxMKTsKCglwcmludGYoIm1xX29wZW4gT0ssIGFib3V0IHRvIHNsZWVwIGZvciAzMCBtb3JlIHNlY29uZHNcbiIpOwoJc2xlZXAoMzApOwoKCU1xX2Nsb3NlKG1xZCk7CglleGl0KDApOwp9Cg==");
    evhttp_send_reply(req, HTTP_OK, "OK", respayload);
}
void proc_upload(struct evhttp_request *req,struct evbuffer *respayload)
{
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=Shift_JIS");
    evbuffer_add_printf(respayload, "%s", "<monitoring><cmd>upload</cmd><data><upload><url>https://ems2.elliypower.co.jp/upload</url><file_name>elplog</file_name></upload></data></monitoring>");
    evhttp_send_reply(req, HTTP_OK, "OK", respayload);
}

//body
void proc_post(struct evhttp_request *req,struct evbuffer *evb)
{
    const char *cmdtype;
    switch (echo_request_get_command(req)) {
	case ECHO_REQ_AHTH : proc_auth(req,evb); break;
	case ECHO_REQ_MEMB : proc_memb(req,evb); break;
    case ECHO_REQ_PUT  : proc_put (req,evb); break;
    case ECHO_REQ_GET  : proc_get(req,evb); break;
	case ECHO_REQ_SET  : proc_set(req,evb); break;
    // case ECHO_REQ_FILE : proc_file (req,evb); break;
    case ECHO_REQ_UPLOAD : proc_upload (req,evb); break;
	// case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
	// case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
	// case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
	// case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
	// case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
	// case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
	// case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
	// default: cmdtype = "unknown"; break;
	}
}

//body
void proc_get_type(struct evhttp_request *req,struct evbuffer *evb)
{
    proc_file (req,evb); 
    // const char *cmdtype;
    // switch (echo_request_get_command(req)) {
	// case ECHO_REQ_AHTH : proc_auth(req,evb); break;
	// case ECHO_REQ_MEMB : proc_memb(req,evb); break;
    // case ECHO_REQ_PUT  : proc_put (req,evb); break;
    // case ECHO_REQ_GET  : proc_get(req,evb); break;
	// case ECHO_REQ_SET  : proc_set(req,evb); break;
    // case ECHO_REQ_FILE : proc_file (req,evb); break;
    // case ECHO_REQ_UPLOAD : proc_upload (req,evb); break;
	// case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
	// case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
	// case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
	// case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
	// case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
	// case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
	// case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
	// default: cmdtype = "unknown"; break;
	// }
}

/* Callback used for the /dump URI, and for every non-GET request:
 * dumps all information to stdout and gives back a trivial 200 ok */
static void
dump_request_cb(struct evhttp_request *req, void *arg)
{
	const char *cmdtype;
	struct evkeyvalq *headers;
	struct evkeyval *header;
	struct evbuffer *buf;
    struct evbuffer *respayload = evbuffer_new();
	
    puts("\n Input data: <<<");
	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
	    header = header->next.tqe_next) {
		printf("  %s: %s\n", header->key, header->value);
	}

    switch (evhttp_request_get_command(req)) {
	case EVHTTP_REQ_GET: cmdtype = "GET"; break;
                         proc_get_type(req,respayload);
                         break;
	case EVHTTP_REQ_POST: 
                        cmdtype = "POST"; 
                        proc_post(req,respayload);
                        break;

	case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
	case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
	case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
	case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
	case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
	case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
	case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
	default: cmdtype = "unknown"; break;
	}

	// printf("Received a %s request for %s\nHeaders:\n",
	//     cmdtype, evhttp_request_get_uri(req));

	// headers = evhttp_request_get_input_headers(req);
	// for (header = headers->tqh_first; header;
	//     header = header->next.tqe_next) {
	// 	printf("  %s: %s\n", header->key, header->value);
	// }

	// evhttp_send_reply(req, 200, "OK", respayload);


}

#define URL_AUTH     "/v1/gateway/auth"
#define URL_REFRESH  "/v1/gateway/refresh"
#define URL_SENSOR   "/sensor"
#define URL_BEACON   "/v1/gateway/beacontag/event"
#define URL_MGSENSOR "/v1/gateway/mgsensor/event"

void auth_ok(void);
void auth_format_invalid1(void);

void first_auth(struct evhttp_request *req,struct evbuffer *respayload)
{

}

void auth_reply_ok(struct evhttp_request *req,struct evbuffer *respayload)
{
    char buf[] = "{\"expiresAt\":\"2020-03-22T04:09:54+00:00\",\"key\":\"dkey_7gJsF9yZQBDF19hxpzbW\",\"refreshToken\":\"refreshToken_7bVXWbKa8lBqFDT5FRRX\",\"setAt\":\"2020-03-21T04:09:54+00:00\"}";
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=Shift_JIS");
    evbuffer_add_printf(respayload, "%s", buf);
    evhttp_send_reply(req, HTTP_OK, "OK", respayload);
}

void sensor_reply_ok(struct evhttp_request *req,struct evbuffer *respayload)
{
    char buf[] = "[{\"applicationName\":\"watch_over\",\"sensorType\":\"MG-SENSOR\",\"serialNumbers\":\"A434F1F165DF\"},{\"applicationName\":\"watch_over\",\"sensorType\":\"MG-SENSOR\",\"serialNumbers\":\"A434F1F18155\"},{\"applicationName\":\"watch_over\",\"sensorType\":\"BEACON-TAG\",\"serialNumbers\":\"A434F1F18121\"},{\"applicationName\":\"watch_over\",\"sensorType\":\"BEACON-TAG\",\"serialNumbers\":\"C1F755B9A7C2\"},{\"applicationName\":\"watch_over\",\"sensorType\":\"BEACON-TAG\",\"serialNumbers\":\"DD29AADA8F2E\"},{\"applicationName\":\"watch_over\",\"sensorType\":\"BEACON-TAG\",\"serialNumbers\":\"DFD7A0ACB4A1\"},{\"applicationName\":\"watch_over\",\"sensorType\":\"BEACON-TAG\",\"serialNumbers\":\"E8B17975C8E6\"},{\"applicationName\":\"watch_over\",\"sensorType\":\"BEACON-TAG\",\"serialNumbers\":\"FA9590CD8343\"}]";
    // char buf[] = "[{\"applicationName\":\"watch_over\",\"sensorType\":\"MG-SENSOR\",\"serialNumbers\":\"A434F1F165DF\"}]";
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=Shift_JIS");
    evbuffer_add_printf(respayload, "%s", buf);
    evhttp_send_reply(req, HTTP_OK, "OK", respayload);
}

void data_reply_ok(struct evhttp_request *req,struct evbuffer *respayload)
{
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=Shift_JIS");
    evbuffer_add_printf(respayload, "%s", "{}");
    evhttp_send_reply(req, HTTP_OK, "OK", respayload);
}

void data_no_reply(struct evhttp_request *req,struct evbuffer *respayload)
{

}
/* { "error": "BadRequest","description": "Request is unexpected.","code": "400-06"} */
void data_invalid_creden(struct evhttp_request *req,struct evbuffer *respayload)
{
    char buf[] = "{ \"error\": \"nvalidCredential\",\"description\": \"Request is unexpected.\",\"code\": \"401-00\"}";
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=Shift_JIS");
    evbuffer_add_printf(respayload, "%s", buf);
    evhttp_send_reply(req, 401, "OK", respayload);
}

// {"refreshToken": "refreshToken_AsUp4rSTr0ngcR4denTia1","serialNumber": "01234567890"}
void refresh_reply_ok(struct evhttp_request *req,struct evbuffer *respayload)
{
    char buf[] = "{\"expiresAt\":\"2020-03-22T04:09:54+00:00\",\"key\":\"dkey_7gJsF9yZQBDF19hxpzbW\",\"refreshToken\":\"refreshToken_7bVXWbKa8lBqFDT5FRRX\",\"setAt\":\"2020-03-21T04:09:54+00:00\"}";
    evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "text/html; charset=Shift_JIS");
    evbuffer_add_printf(respayload, "%s", buf);
    evhttp_send_reply(req, HTTP_OK, "OK", respayload);
}

//todo : url check\payload check
typedef struct
{
    void           (*action)(struct evhttp_request *,struct evbuffer *); 
    const char     expected[128];      
}subset_t;
 
/* normal test */
subset_t subset1[] = {
    {first_auth      , URL_AUTH    },
    {auth_reply_ok   , URL_SENSOR  },
    {sensor_reply_ok , URL_MGSENSOR},
    {data_reply_ok   , URL_MGSENSOR},
    {data_reply_ok   , URL_MGSENSOR},
    {NULL            , ""          },
};

subset_t subset2[] = {
    {data_no_reply   , URL_AUTH    },
    {NULL            , ""          },
};

subset_t subset3[] = {
    {refresh_reply_ok , URL_REFRESH },
    {NULL             , ""          },
};

subset_t subset4[] = {
    {data_invalid_creden , URL_REFRESH  },
    {refresh_reply_ok    , URL_MGSENSOR },
    {data_reply_ok       , URL_MGSENSOR },
    {NULL             , ""          },
};

subset_t subset5[] = {
    {data_invalid_creden , URL_REFRESH },
    {NULL             , ""          },
};
subset_t * totalset[] = {
    subset1,
    // subset2,
    // subset3,
    subset4,
    NULL,
}; 

/* This callback gets invoked when we get any http request that doesn't match*/

static void
test_cb(struct evhttp_request *req, void *arg)
{
    struct evkeyvalq *headers;
    struct evkeyval  *header;   
    struct evbuffer  *respayload = evbuffer_new();
    static int first_flag = 1;
    int id = 0;
    static subset_t  **ptotal =  NULL;
    static subset_t  *pset = NULL;
    char * url = NULL;
    
    if(1 == first_flag) {
        first_flag = 0;
        ptotal = &totalset[0];
        pset = *ptotal;
    }

    puts("\nInput data: <<<");
    
    url = evhttp_request_get_uri(req);
    printf("url = %s\n",url);

/* 	
headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
	    header = header->next.tqe_next) {
		printf("  %s: %s\n", header->key, header->value);
	} 
*/

    if((NULL == pset) || (NULL == pset->action)) {
        printf("subset is null,finish test...\n");
        exit(1);
        return;
    }

    /* judge */
    if(NULL != strstr(url,pset->expected)) {
        printf("----PASS----\n");
     } else {
        printf("-----NO-----\n");
        printf("expected = %s, recv = %s\n",pset->expected,url);
        exit(1);
     }

     pset++;

    if((NULL == pset) || (NULL == pset->action)) {
        ptotal++;
        if(NULL == ptotal) {
            printf("total is null,finish test\n");
            exit(1);
        } else {
            pset = *ptotal;
            if((NULL == pset) || (NULL == pset->action)) {
                printf("subset is null,finish test\n");
                exit(1);    
            }
        }
    }

    pset->action(req, respayload);
     
}
/* This callback gets invoked when we get any http request that doesn't match
 * any other callback.  Like any evhttp server callback, it has a simple job:
 * it must eventually call evhttp_send_error() or evhttp_send_reply().
 */
static void
send_document_cb(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = NULL;
    struct evbuffer *evb_new = NULL;
	const char *docroot = arg;
	const char *uri = evhttp_request_get_uri(req);
	struct evhttp_uri *decoded = NULL;
	const char *path;
	char *decoded_path;
	char *whole_path = NULL;
	size_t len;
	int fd = -1;
	struct stat st;
    struct evkeyvalq *headers;
    struct evkeyval *header;

	if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
		dump_request_cb(req, arg);
		return;
	}

	printf("Got a GET request for <%s>\n",  uri);

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
	    header = header->next.tqe_next) {
		printf("  %s: %s\n", header->key, header->value);
	}

	/* This holds the content we're sending. */
	evb_new = evbuffer_new();
    proc_file (req,evb_new); 
    return ;
	
    /* Decode the URI */
	decoded = evhttp_uri_parse(uri);
	if (!decoded) {
		printf("It's not a good URI. Sending BADREQUEST\n");
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	/* Let's see what path the user asked for. */
	path = evhttp_uri_get_path(decoded);
    printf("path=%s",path);
	if (!path) path = "/";
    
	/* We need to decode it, to see what path the user really wanted. */
	decoded_path = evhttp_uridecode(path, 0, NULL);
	if (decoded_path == NULL)
		goto err;
	/* Don't allow any ".."s in the path, to avoid exposing stuff outside
	 * of the docroot.  This test is both overzealous and underzealous:
	 * it forbids aceptable paths like "/this/one..here", but it doesn't
	 * do anything to prevent symlink following." */
	if (strstr(decoded_path, ".."))
		goto err;

	len = strlen(decoded_path)+strlen(docroot)+2;
	if (!(whole_path = malloc(len))) {
		perror("malloc");
		goto err;
	}
	evutil_snprintf(whole_path, len, "%s/%s", docroot, decoded_path);

	if (stat(whole_path, &st)<0) {
		goto err;
	}

	/* This holds the content we're sending. */
	evb = evbuffer_new();

	if (S_ISDIR(st.st_mode)) {
		/* If it's a directory, read the comments and make a little
		 * index page */
#ifdef _WIN32
		HANDLE d;
		WIN32_FIND_DATAA ent;
		char *pattern;
		size_t dirlen;
#else
		DIR *d;
		struct dirent *ent;
#endif
		const char *trailing_slash = "";

		if (!strlen(path) || path[strlen(path)-1] != '/')
			trailing_slash = "/";

#ifdef _WIN32
		dirlen = strlen(whole_path);
		pattern = malloc(dirlen+3);
		memcpy(pattern, whole_path, dirlen);
		pattern[dirlen] = '\\';
		pattern[dirlen+1] = '*';
		pattern[dirlen+2] = '\0';
		d = FindFirstFileA(pattern, &ent);
		free(pattern);
		if (d == INVALID_HANDLE_VALUE)
			goto err;
#else
		if (!(d = opendir(whole_path)))
			goto err;
#endif

		evbuffer_add_printf(evb,
                    "<!DOCTYPE html>\n"
                    "<html>\n <head>\n"
                    "  <meta charset='utf-8'>\n"
		    "  <title>%s</title>\n"
		    "  <base href='%s%s'>\n"
		    " </head>\n"
		    " <body>\n"
		    "  <h1>%s</h1>\n"
		    "  <ul>\n",
		    decoded_path, /* XXX html-escape this. */
		    path, /* XXX html-escape this? */
		    trailing_slash,
		    decoded_path /* XXX html-escape this */);
#ifdef _WIN32
		do {
			const char *name = ent.cFileName;
#else
		while ((ent = readdir(d))) {
			const char *name = ent->d_name;
#endif
			evbuffer_add_printf(evb,
			    "    <li><a href=\"%s\">%s</a>\n",
			    name, name);/* XXX escape this */
#ifdef _WIN32
		} while (FindNextFileA(d, &ent));
#else
		}
#endif
		evbuffer_add_printf(evb, "</ul></body></html>\n");
#ifdef _WIN32
		FindClose(d);
#else
		closedir(d);
#endif  
// Set-Cookie: JSESSIONID=9CF07B056A5A1B788794A6D69BA249B9; Path=/CenterServer/; HttpOnly
        evhttp_add_header(evhttp_request_get_output_headers(req),
		    "Content-Type", "text/html"); 
	} else {
		/* Otherwise it's a file; add it to the buffer to get
		 * sent via sendfile */
		const char *type = guess_content_type(decoded_path);
		if ((fd = open(whole_path, O_RDONLY)) < 0) {
			perror("open");
			goto err;
		}

		if (fstat(fd, &st)<0) {
			/* Make sure the length still matches, now that we
			 * opened the file :/ */
			perror("fstat");
			goto err;
		}
		evhttp_add_header(evhttp_request_get_output_headers(req),
		    "Content-Type", type);
		evbuffer_add_file(evb, fd, 0, st.st_size);
	}

	evhttp_send_reply(req, 200, "OK", evb);
	goto done;
err:

         printf("\n -----test------\n");  

	evhttp_send_error(req, 404, "Document was not found");
	if (fd>=0)
		close(fd);
done:
	if (decoded)
		evhttp_uri_free(decoded);
	if (decoded_path)
		free(decoded_path);
	if (whole_path)
		free(whole_path);
	if (evb)
		evbuffer_free(evb);
}

static void
print_usage(FILE *out, const char *prog, int exit_code)
{
	fprintf(out, "Syntax: [ OPTS ] %s <docroot>\n", prog);
	fprintf(out, " -p      - port\n");
	fprintf(out, " -U      - bind to unix socket\n");
	fprintf(out, " -u      - unlink unix socket before bind\n");
	fprintf(out, " -I      - IOCP\n");
	fprintf(out, " -v      - verbosity, enables libevent debug logging too\n");
	exit(exit_code);
}
static struct options
parse_opts(int argc, char **argv)
{
	struct options o;
	int opt;

	memset(&o, 0, sizeof(o));

	while ((opt = getopt(argc, argv, "hp:U:uIv")) != -1) {
		switch (opt) {
			case 'p': o.port = atoi(optarg); break;
			case 'U': o.unixsock = optarg; break;
			case 'u': o.unlink = 1; break;
			case 'I': o.iocp = 1; break;
			case 'v': ++o.verbose; break;
			case 'h': print_usage(stdout, argv[0], 0); break;
			default : fprintf(stderr, "Unknown option %c\n", opt); break;
		}
	}

	if (optind >= argc || (argc-optind) > 1) {
		print_usage(stdout, argv[0], 1);
	}

	return o;
}

static void
do_term(int sig, short events, void *arg)
{
	struct event_base *base = arg;
	event_base_loopbreak(base);
	fprintf(stderr, "Got %i, Terminating\n", sig);
}

static int
display_listen_sock(struct evhttp_bound_socket *handle)
{
	struct sockaddr_storage ss;
	evutil_socket_t fd;
	ev_socklen_t socklen = sizeof(ss);
	char addrbuf[128];
	void *inaddr;
	const char *addr;
	int got_port = -1;

	fd = evhttp_bound_socket_get_fd(handle);
	memset(&ss, 0, sizeof(ss));
	if (getsockname(fd, (struct sockaddr *)&ss, &socklen)) {
		perror("getsockname() failed");
		return 1;
	}

	if (ss.ss_family == AF_INET) {
		got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
		inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
	} else if (ss.ss_family == AF_INET6) {
		got_port = ntohs(((struct sockaddr_in6*)&ss)->sin6_port);
		inaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
	}
#ifdef EVENT__HAVE_STRUCT_SOCKADDR_UN
	else if (ss.ss_family == AF_UNIX) {
		printf("Listening on <%s>\n", ((struct sockaddr_un*)&ss)->sun_path);
		return 0;
	}
#endif
	else {
		fprintf(stderr, "Weird address family %d\n",
		    ss.ss_family);
		return 1;
	}

	addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf,
	    sizeof(addrbuf));
	if (addr) {
		printf("Listening on %s:%d\n", addr, got_port);
		evutil_snprintf(uri_root, sizeof(uri_root),
		    "http://%s:%d",addr,got_port);
	} else {
		fprintf(stderr, "evutil_inet_ntop failed\n");
		return 1;
	}

	return 0;
}

int
main(int argc, char **argv)
{
	struct event_config *cfg = NULL;
	struct event_base *base = NULL;
	struct evhttp *http = NULL;
	struct evhttp_bound_socket *handle = NULL;
	struct evconnlistener *lev = NULL;
	struct event *term = NULL;
	struct options o = parse_opts(argc, argv);
	int ret = 0;

#ifdef _WIN32
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD(2, 2);
		WSAStartup(wVersionRequested, &wsaData);
	}
#else
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		ret = 1;
		goto err;
	}
#endif

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	/** Read env like in regress */
	if (o.verbose || getenv("EVENT_DEBUG_LOGGING_ALL"))
		event_enable_debug_logging(EVENT_DBG_ALL);

	cfg = event_config_new();
#ifdef _WIN32
	if (o.iocp) {
#ifdef EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED
		evthread_use_windows_threads();
		event_config_set_num_cpus_hint(cfg, 8);
#endif
		event_config_set_flag(cfg, EVENT_BASE_FLAG_STARTUP_IOCP);
	}
#endif

	base = event_base_new_with_config(cfg);
	if (!base) {
		fprintf(stderr, "Couldn't create an event_base: exiting\n");
		ret = 1;
	}
	event_config_free(cfg);
	cfg = NULL;

	/* Create a new evhttp object to handle requests. */
	http = evhttp_new(base);
	if (!http) {
		fprintf(stderr, "couldn't create evhttp. Exiting.\n");
		ret = 1;
	}

	/* The /dump URI will dump all requests to stdout and say 200 ok. */
	evhttp_set_cb(http, "/dump", dump_request_cb, NULL);

	/* We want to accept arbitrary requests, so we need to set a "generic"
	 * cb.  We can also add callbacks for specific paths. */
	if(0)
        evhttp_set_gencb(http, send_document_cb, argv[1]);
    else
        evhttp_set_gencb(http, test_cb, argv[1]);
    
	if (o.unixsock) {
#ifdef EVENT__HAVE_STRUCT_SOCKADDR_UN
		struct sockaddr_un addr;

		if (o.unlink && (unlink(o.unixsock) && errno != ENOENT)) {
			perror(o.unixsock);
			ret = 1;
			goto err;
		}

		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, o.unixsock);

		lev = evconnlistener_new_bind(base, NULL, NULL,
			LEV_OPT_CLOSE_ON_FREE, -1,
			(struct sockaddr *)&addr, sizeof(addr));
		if (!lev) {
			perror("Cannot create listener");
			ret = 1;
			goto err;
		}

		handle = evhttp_bind_listener(http, lev);
		if (!handle) {
			fprintf(stderr, "couldn't bind to %s. Exiting.\n", o.unixsock);
			ret = 1;
			goto err;
		}
#else /* !EVENT__HAVE_STRUCT_SOCKADDR_UN */
		fprintf(stderr, "-U is not supported on this platform. Exiting.\n");
		ret = 1;
		goto err;
#endif /* EVENT__HAVE_STRUCT_SOCKADDR_UN */
	}
	else {
		handle = evhttp_bind_socket_with_handle(http, "127.0.0.1", o.port);
        // handle = evhttp_bind_socket_with_handle(http, "192.168.0.2", o.port);
		if (!handle) {
			fprintf(stderr, "couldn't bind to port %d. Exiting.\n", o.port);
			ret = 1;
			goto err;
		}
	}

	if (display_listen_sock(handle)) {
		ret = 1;
		goto err;
	}

	term = evsignal_new(base, SIGINT, do_term, base);
	if (!term)
		goto err;
	if (event_add(term, NULL))
		goto err;

	event_base_dispatch(base);

#ifdef _WIN32
	WSACleanup();
#endif

err:
	if (cfg)
		event_config_free(cfg);
	if (http)
		evhttp_free(http);
	if (term)
		event_free(term);
	if (base)
		event_base_free(base);

	return ret;
}
