
#include <qse/http/std.h>
#include <qse/xli/std.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/time.h>
#include <qse/cmn/path.h>

#include <signal.h>
#include <locale.h>

#if defined(_WIN32)
#	include <winsock2.h>
#	include <windows.h>
#	include <tchar.h>
#	include <process.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_ERRORS
#	include <os2.h>
#elif defined(__DOS__)
#	include <dos.h>
#else
#	include <unistd.h>
#	include <errno.h>
#endif

#if defined(HAVE_SSL)
#	include <openssl/ssl.h>
#	include <openssl/err.h>
#	include <openssl/engine.h>
#endif

static void reconf_server (qse_httpd_t* httpd, qse_httpd_server_t* server);

/* --------------------------------------------------------------------- */

static qse_httpd_t* g_httpd = QSE_NULL;

static void sigint (int sig)
{
	if (g_httpd) qse_httpd_stop (g_httpd);
}

static void sighup (int sig)
{
	if (g_httpd) qse_httpd_reconf (g_httpd);
}

static void setup_signal_handlers ()
{
	struct sigaction act;

#if defined(SIGINT)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = sigint;
	sigaction (SIGINT, &act, QSE_NULL);
#endif

#if defined(SIGHUP)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = sighup;
	sigaction (SIGHUP, &act, QSE_NULL);
#endif

#if defined(SIGPIPE)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = SIG_IGN;
	sigaction (SIGPIPE, &act, QSE_NULL);
#endif
}

static void restore_signal_handlers ()
{
	struct sigaction act;

#if defined(SIGINT)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = SIG_DFL;
	sigaction (SIGINT, &act, QSE_NULL);
#endif

#if defined(SIGHUP)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = SIG_DFL;
	sigaction (SIGHUP, &act, QSE_NULL);
#endif

#if defined(SIGPIPE)
	qse_memset (&act, 0, QSE_SIZEOF(act));
	act.sa_handler = SIG_DFL;
	sigaction (SIGPIPE, &act, QSE_NULL);
#endif
}

/* --------------------------------------------------------------------- */

enum
{
	SCFG_NAME,
	SCFG_DOCROOT,
	SCFG_REALM,
	SCFG_AUTH,
	SCFG_DIRCSS,
	SCFG_ERRCSS,

	SCFG_MAX
};

struct cgi_t
{
	enum {
		CGI_SUFFIX,
		CGI_FILE,

		CGI_MAX
	} type;

	qse_mchar_t* spec;
	int nph;
	qse_mchar_t* shebang;

	struct cgi_t* next;
};

struct mime_t
{
	enum {
		MIME_SUFFIX,
		MIME_FILE,

		MIME_MAX
	} type;

	qse_mchar_t* spec;
	qse_mchar_t* value;

	struct mime_t* next;
};

typedef struct server_xtn_t server_xtn_t;
struct server_xtn_t
{
	int tproxy;
	int nodir; /* no directory listing */

	int num;
	qse_nwad_t bind;

	qse_httpd_serverstd_makersrc_t orgmakersrc;
	qse_httpd_serverstd_freersrc_t orgfreersrc;
	qse_httpd_serverstd_query_t orgquery;

	qse_mchar_t* scfg[SCFG_MAX];
	
	struct
	{
		qse_size_t count;
		qse_mchar_t* files;
	} index;

	struct
	{
		struct cgi_t* head;
		struct cgi_t* tail;
	} cgi[CGI_MAX];

	struct
	{
		struct mime_t* head;
		struct mime_t* tail;
	} mime[MIME_MAX];
};

typedef struct httpd_xtn_t httpd_xtn_t;
struct httpd_xtn_t
{
	const  qse_char_t* cfgfile;
	qse_xli_t* xli;
	qse_httpd_ecb_t ecb;
};

static int make_resource (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_htre_t* req, qse_httpd_rsrc_t* rsrc)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverstdxtn (httpd, client->server);

	if (server_xtn->tproxy)
	{
		if (qse_nwadequal(&client->orgdst_addr, &client->local_addr)) /* both equal and error */
		{
			/* TODO: implement a better check that the
			 *       destination is not one of the local addresses */

			rsrc->type = QSE_HTTPD_RSRC_ERR;
			rsrc->u.err.code = 500;
		}
		else
		{
			rsrc->type = QSE_HTTPD_RSRC_PROXY;
			rsrc->u.proxy.dst = client->orgdst_addr;
			rsrc->u.proxy.src = client->remote_addr;
	
			if (rsrc->u.proxy.src.type == QSE_NWAD_IN4)
				rsrc->u.proxy.src.u.in4.port = 0; /* reset the port to 0. */
			else if (rsrc->u.proxy.src.type == QSE_NWAD_IN6)
				rsrc->u.proxy.src.u.in6.port = 0; /* reset the port to 0. */
		}

		return 0;
	}
	else
	{
		if (server_xtn->orgmakersrc (httpd, client, req, rsrc) <= -1) return -1;
		if (server_xtn->nodir && rsrc->type == QSE_HTTPD_RSRC_DIR)
		{
			/* prohibit no directory listing */
			if (server_xtn->orgfreersrc)
				server_xtn->orgfreersrc (httpd, client, req, rsrc);
			rsrc->type = QSE_HTTPD_RSRC_ERR;
			rsrc->u.err.code = 403;
		}
		return 0;
	}
}

static void free_resource (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_htre_t* req, qse_httpd_rsrc_t* rsrc)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverstdxtn (httpd, client->server);

	if (server_xtn->tproxy)
	{
		/* nothing to do */
	}
	else
	{
		if (server_xtn->orgfreersrc) 
			server_xtn->orgfreersrc (httpd, client, req, rsrc);
	}
}
/* --------------------------------------------------------------------- */
static void clear_server_config (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	server_xtn_t* server_xtn;
	qse_size_t i;

	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	for (i = 0; i < QSE_COUNTOF(server_xtn->scfg); i++)
	{
		if (server_xtn->scfg[i]) 
		{
			qse_httpd_freemem (httpd, server_xtn->scfg[i]);
			server_xtn->scfg[i] = QSE_NULL;
		}
	}

	if (server_xtn->index.files) 
	{
		qse_httpd_freemem (httpd, server_xtn->index.files);
		server_xtn->index.files = QSE_NULL;
		server_xtn->index.count = 0;
	}

	for (i = 0; i < QSE_COUNTOF(server_xtn->cgi); i++)
	{
		struct cgi_t* cgi = server_xtn->cgi[i].head;
		while (cgi)
		{
			struct cgi_t* x = cgi;
			cgi = x->next;

			if (x->shebang) qse_httpd_freemem (httpd, x->shebang);
			if (x->spec) qse_httpd_freemem (httpd, x->spec);
			if (x) qse_httpd_freemem (httpd, x);
		}

		server_xtn->cgi[i].head = QSE_NULL;
		server_xtn->cgi[i].tail = QSE_NULL;
	}

	for (i = 0; i < QSE_COUNTOF(server_xtn->mime); i++)
	{
		struct mime_t* mime = server_xtn->mime[i].head;
		while (mime)
		{
			struct mime_t* x = mime;
			mime = x->next;

			if (x->spec) qse_httpd_freemem (httpd, x->spec);
			if (x->value) qse_httpd_freemem (httpd, x->value);
			if (x) qse_httpd_freemem (httpd, x);
		}

		server_xtn->mime[i].head = QSE_NULL;
		server_xtn->mime[i].tail = QSE_NULL;
	}
}

static void detach_server (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	clear_server_config (httpd, server);
}

static int query_server (
	qse_httpd_t* httpd, qse_httpd_server_t* server, 
	qse_htre_t* req, const qse_mchar_t* xpath,
	qse_httpd_serverstd_query_code_t code, void* result)
{
	server_xtn_t* server_xtn;

	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	switch (code)
	{
		case QSE_HTTPD_SERVERSTD_NAME:
			*(const qse_mchar_t**)result = server_xtn->scfg[SCFG_NAME];
			return 0;

		case QSE_HTTPD_SERVERSTD_DOCROOT:
			*(const qse_mchar_t**)result = server_xtn->scfg[SCFG_DOCROOT];
			return 0;

		case QSE_HTTPD_SERVERSTD_REALM:
			*(const qse_mchar_t**)result = server_xtn->scfg[SCFG_REALM];
			return 0;

		case QSE_HTTPD_SERVERSTD_AUTH:
			*(const qse_mchar_t**)result = server_xtn->scfg[SCFG_AUTH];
			return 0;

		case QSE_HTTPD_SERVERSTD_DIRCSS:
			*(const qse_mchar_t**)result = server_xtn->scfg[SCFG_DIRCSS];
			return 0;

		case QSE_HTTPD_SERVERSTD_ERRCSS:
			*(const qse_mchar_t**)result = server_xtn->scfg[SCFG_ERRCSS];
			return 0;

		case QSE_HTTPD_SERVERSTD_INDEX:
			((qse_httpd_serverstd_index_t*)result)->count = server_xtn->index.count;
			((qse_httpd_serverstd_index_t*)result)->files = server_xtn->index.files;
			return 0;

		case QSE_HTTPD_SERVERSTD_CGI:
		{
			qse_size_t i;
			qse_httpd_serverstd_cgi_t* scgi;
			const qse_mchar_t* xpath_base;

			xpath_base = qse_mbsbasename (xpath);

			scgi = (qse_httpd_serverstd_cgi_t*)result;
			qse_memset (scgi, 0, QSE_SIZEOF(*scgi));

			for (i = 0; i < QSE_COUNTOF(server_xtn->cgi); i++)
			{
				struct cgi_t* cgi;
				for (cgi = server_xtn->cgi[i].head; cgi; cgi = cgi->next)
				{
					if ((cgi->type == MIME_SUFFIX && qse_mbsend (xpath_base, cgi->spec)) ||
					    (cgi->type == MIME_FILE && qse_mbscmp (xpath_base, cgi->spec) == 0))
					{
						scgi->cgi = 1;
						scgi->nph = cgi->nph;		
						scgi->shebang = cgi->shebang;
						return 0;
					}
				}
			}

			return 0;
		}

		case QSE_HTTPD_SERVERSTD_MIME:
		{
			qse_size_t i;
			qse_mchar_t* xpath_base;

			xpath_base = qse_mbsbasename (xpath);

			*(const qse_mchar_t**)result = QSE_NULL;
			for (i = 0; i < QSE_COUNTOF(server_xtn->mime); i++)
			{
				struct mime_t* mime;
				for (mime = server_xtn->mime[i].head; mime; mime = mime->next)
				{
					if ((mime->type == MIME_SUFFIX && qse_mbsend (xpath_base, mime->spec)) ||
					    (mime->type == MIME_FILE && qse_mbscmp (xpath_base, mime->spec) == 0))
					{
						*(const qse_mchar_t**)result = mime->value;
						return 0;
					}
				}
			}
			return 0;
		}
	}

	return server_xtn->orgquery (httpd, server, req, xpath, code, result);
}

/* --------------------------------------------------------------------- */

static struct 
{
	const qse_char_t* x;
	const qse_char_t* y;
} scfg_items[] =
{
	{ QSE_T("host['*'].location['/'].name"),      QSE_T("default.name") },
	{ QSE_T("host['*'].location['/'].docroot"),   QSE_T("default.docroot") },
	{ QSE_T("host['*'].location['/'].realm"),     QSE_T("default.realm") },
	{ QSE_T("host['*'].location['/'].auth"),      QSE_T("default.auth") },
	{ QSE_T("host['*'].location['/'].dir-css"),   QSE_T("default.dir-css") },
	{ QSE_T("host['*'].location['/'].error-css"), QSE_T("default.error-css") }
};

static int load_server_config (
	qse_httpd_t* httpd, qse_httpd_server_t* server, qse_xli_list_t* list)
{
	qse_size_t i;
	httpd_xtn_t* httpd_xtn;
	server_xtn_t* server_xtn;
	qse_xli_pair_t* pair;
	qse_xli_atom_t* atom;

	httpd_xtn = qse_httpd_getxtnstd (httpd);
	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	for (i = 0; i < QSE_COUNTOF(scfg_items); i++)
	{
		pair = qse_xli_findpairbyname (httpd_xtn->xli, list, scfg_items[i].x);
		if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, scfg_items[i].y);
		if (pair && pair->val->type == QSE_XLI_STR)
		{
			server_xtn->scfg[i] = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
			if (server_xtn->scfg[i] == QSE_NULL) 
			{
				/*qse_printf (QSE_T("ERROR in copying - %s\n"), qse_httpd_geterrmsg (httpd));*/
				qse_printf (QSE_T("ERROR in copying\n"));
				return -1;
			}
		}
	}

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("host['*'].location['/'].index"));
	if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, QSE_T("default.index"));
	if (pair && pair->val->type == QSE_XLI_STR)
	{
		const qse_char_t* tmpptr, * tmpend;
		qse_size_t count;

		tmpptr = ((qse_xli_str_t*)pair->val)->ptr;
		tmpend = tmpptr + ((qse_xli_str_t*)pair->val)->len;
	
		for (count = 0; tmpptr < tmpend; count++) tmpptr += qse_strlen (tmpptr) + 1;

		server_xtn->index.count = count;
		server_xtn->index.files = qse_httpd_strntombsdup (
			httpd, ((qse_xli_str_t*)pair->val)->ptr, ((qse_xli_str_t*)pair->val)->len);
		if (server_xtn->index.files == QSE_NULL) 
		{
			qse_printf (QSE_T("ERROR: in copying index\n"));
			return -1;
		}
	}

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("host['*'].location['/'].cgi"));
	if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, QSE_T("default.cgi"));
	if (pair && pair->val->type == QSE_XLI_LIST)
	{
		/* TODO: more sanity check... this can be done with xli schema... if supported */
		qse_xli_list_t* cgilist = (qse_xli_list_t*)pair->val;
		for (atom = cgilist->head; atom; atom = atom->next)
		{
			if (atom->type != QSE_XLI_PAIR) continue;

			pair = (qse_xli_pair_t*)atom;
			if (pair->key && pair->name && 
			    (pair->val->type == QSE_XLI_NIL || pair->val->type == QSE_XLI_STR))
			{
				struct cgi_t* cgi;
				int type;

				if (qse_strcmp (pair->key, QSE_T("suffix")) == 0)
				{
					type = CGI_SUFFIX;
				}
				else if (qse_strcmp (pair->key, QSE_T("file")) == 0)
				{
					type = CGI_FILE;
				}
				else continue;

				cgi = qse_httpd_callocmem (httpd, QSE_SIZEOF(*cgi));
				if (cgi == QSE_NULL)
				{
					qse_printf (QSE_T("ERROR: memory failure in copying cgi\n"));
					return -1;
				}

				cgi->type = type;
				cgi->spec = qse_httpd_strtombsdup (httpd, pair->name);
				if (!cgi->spec)
				{
					qse_httpd_freemem (httpd, cgi);
					qse_printf (QSE_T("ERROR: memory failure in copying cgi\n"));
					return -1;
				}
				if (pair->val->type == QSE_XLI_STR) 
				{
					const qse_char_t* tmpptr, * tmpend;
					qse_size_t count;

					tmpptr = ((qse_xli_str_t*)pair->val)->ptr;
					tmpend = tmpptr + ((qse_xli_str_t*)pair->val)->len;
	
					for (count = 0; tmpptr < tmpend; count++) 
					{
						if (count == 0)
						{
							if (qse_strcmp (tmpptr, QSE_T("nph")) == 0) cgi->nph = 1;
						}
						else if (count == 1)
						{
							cgi->shebang = qse_httpd_strtombsdup (httpd, tmpptr);
							if (!cgi->shebang)
							{
								qse_httpd_freemem (httpd, cgi->spec);
								qse_httpd_freemem (httpd, cgi);
								qse_printf (QSE_T("ERROR: memory failure in copying cgi\n"));
								return -1;
							}
						}

						tmpptr += qse_strlen (tmpptr) + 1;

						/* TODO: more sanity check */
					}

				}
				if (server_xtn->cgi[type].tail)
					server_xtn->cgi[type].tail->next = cgi;
				else
					server_xtn->cgi[type].head = cgi;
				server_xtn->cgi[type].tail = cgi;
			}
		}	
	}

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("host['*'].location['/'].mime"));
	if (!pair) pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, QSE_T("default.mime"));
	if (pair && pair->val->type == QSE_XLI_LIST)
	{
		qse_xli_list_t* mimelist = (qse_xli_list_t*)pair->val;
		for (atom = mimelist->head; atom; atom = atom->next)
		{
			if (atom->type != QSE_XLI_PAIR) continue;

			pair = (qse_xli_pair_t*)atom;
			if (pair->key && pair->name && pair->val->type == QSE_XLI_STR)
			{
				struct mime_t* mime;
				int type;

				if (qse_strcmp (pair->key, QSE_T("suffix")) == 0) type = MIME_SUFFIX;
				else if (qse_strcmp (pair->key, QSE_T("file")) == 0) type = MIME_FILE;
				else continue;

				mime = qse_httpd_callocmem (httpd, QSE_SIZEOF(*mime));
				if (mime == QSE_NULL)
				{
					qse_printf (QSE_T("ERROR: memory failure in copying mime\n"));
					return -1;
				}

				mime->type = type;
				mime->spec = qse_httpd_strtombsdup (httpd, pair->name);
				if (!mime->spec)
				{
					qse_httpd_freemem (httpd, mime);
					qse_printf (QSE_T("ERROR: memory failure in copying mime\n"));
					return -1;
				}
				if (pair->val->type == QSE_XLI_STR) 
				{
					mime->value = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
					if (!mime->value)
					{
						qse_httpd_freemem (httpd, mime->spec);
						qse_httpd_freemem (httpd, mime);
						qse_printf (QSE_T("ERROR: memory failure in copying mime\n"));
						return -1;
					}

					/* TODO: more sanity check */

				}
				if (server_xtn->mime[type].tail)
					server_xtn->mime[type].tail->next = mime;
				else
					server_xtn->mime[type].head = mime;
				server_xtn->mime[type].tail = mime;
			}
		}	
	}

	/* perform more sanity check */
	if (qse_mbschr (server_xtn->scfg[SCFG_AUTH], QSE_MT(':')) == QSE_NULL)
	{
		qse_printf (QSE_T("WARNING: no colon in the auth string - [%hs]\n"), server_xtn->scfg[SCFG_AUTH]);
	}

	return 0;
}

static qse_httpd_server_t* attach_server (qse_httpd_t* httpd, int num, qse_xli_list_t* list)
{
	qse_httpd_server_dope_t dope; 
	qse_httpd_server_t* xserver;
	httpd_xtn_t* httpd_xtn;
	server_xtn_t* server_xtn;
	qse_xli_pair_t* pair;

	httpd_xtn = qse_httpd_getxtnstd (httpd);

	pair = qse_xli_findpairbyname (httpd_xtn->xli, list, QSE_T("bind"));
	if (pair == QSE_NULL || pair->val->type != QSE_XLI_STR)
	{
		/* TOOD: logging */
		qse_printf (QSE_T("WARNING: no value or invalid value specified for bind\n"));
		return QSE_NULL;
	}

	qse_memset (&dope, 0, QSE_SIZEOF(dope));
	if (qse_strtonwad (((qse_xli_str_t*)pair->val)->ptr, &dope.nwad) <= -1)
	{
		/*  TOOD: logging */
		qse_printf (QSE_T("WARNING: invalid value for bind - %s\n"), ((qse_xli_str_t*)pair->val)->ptr);
		return QSE_NULL;
	}

	dope.detach = detach_server;
	dope.reconf = reconf_server;
	xserver = qse_httpd_attachserverstd (httpd, &dope, QSE_SIZEOF(server_xtn_t));
	if (xserver == QSE_NULL) 
	{
		/* TODO: logging */
		qse_printf (QSE_T("WARNING: failed to attach server\n"));
		return QSE_NULL;
	}

	server_xtn = qse_httpd_getserverstdxtn (httpd, xserver);

	/* remember original callbacks  */
	qse_httpd_getserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_QUERY, &server_xtn->orgquery);
	qse_httpd_getserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_MAKERSRC, &server_xtn->orgmakersrc);
	qse_httpd_getserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_FREERSRC, &server_xtn->orgfreersrc);

	/* set changeable callbacks  */
	qse_httpd_setserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_QUERY, query_server);
	qse_httpd_setserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_MAKERSRC, make_resource);
	qse_httpd_setserverstdopt (httpd, xserver, QSE_HTTPD_SERVERSTD_FREERSRC, free_resource);

	/* remember the binding address used */
	server_xtn->num = num;
	server_xtn->bind = dope.nwad;

	return xserver;
}

static int open_config_file (qse_httpd_t* httpd)
{
	httpd_xtn_t* httpd_xtn;
	qse_xli_iostd_t xli_in;
	int trait;

	httpd_xtn = (httpd_xtn_t*) qse_httpd_getxtnstd (httpd);
	QSE_ASSERT (httpd_xtn->xli == QSE_NULL);

	httpd_xtn->xli = qse_xli_openstd (0);
	if (	httpd_xtn->xli == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open xli\n"));
		return -1;
	}
 
	qse_xli_getopt (httpd_xtn->xli, QSE_XLI_TRAIT, &trait);
	trait |= QSE_XLI_KEYNAME;
	qse_xli_setopt (httpd_xtn->xli, QSE_XLI_TRAIT, &trait);


	xli_in.type = QSE_XLI_IOSTD_FILE;
	xli_in.u.file.path = httpd_xtn->cfgfile;
	xli_in.u.file.cmgr = QSE_NULL;
 
	if (qse_xli_readstd (httpd_xtn->xli, &xli_in) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot load %s - %s\n"), xli_in.u.file.path, qse_xli_geterrmsg(httpd_xtn->xli));
		qse_xli_close (httpd_xtn->xli);
		httpd_xtn->xli = QSE_NULL;
		return -1;
	}
 
	return 0;
}

static int close_config_file (qse_httpd_t* httpd)
{
	httpd_xtn_t* httpd_xtn;

	httpd_xtn = (httpd_xtn_t*) qse_httpd_getxtnstd (httpd);
	if (httpd_xtn->xli)
	{
		qse_xli_close (httpd_xtn->xli);
		httpd_xtn->xli = QSE_NULL;
	}

	return 0;
}

static int load_config (qse_httpd_t* httpd)
{
	qse_xli_iostd_t xli_in;
	qse_xli_pair_t* pair;
	httpd_xtn_t* httpd_xtn;
	int i;

	httpd_xtn = (httpd_xtn_t*)qse_httpd_getxtnstd (httpd);

	if (open_config_file (httpd) <= -1) goto oops;

	for (i = 0; ; i++)
	{
		qse_char_t buf[32];
		qse_sprintf (buf, QSE_COUNTOF(buf), QSE_T("server[%d]"), i);
		pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, buf);
		if (pair == QSE_NULL) break;

		if (pair->val->type != QSE_XLI_LIST)
		{
			qse_fprintf (QSE_STDERR, QSE_T("WARNING: non-list value for server\n"));
		}
		else
		{
			qse_httpd_server_t* server;
	
			server = attach_server (httpd, i, (qse_xli_list_t*)pair->val);
			if (server)
			{
				load_server_config (httpd, server, (qse_xli_list_t*)pair->val);
				/* TODO: error check */
			}
		}
	}

	if (i == 0)
	{
		qse_fprintf (QSE_STDERR, QSE_T("No valid server specified in %s\n"), xli_in.u.file.path);
		goto oops;
	}

	pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, QSE_T("default.name"));
	if (pair && pair->val->type == QSE_XLI_STR)
	{
		qse_mchar_t* name = qse_httpd_strtombsdup (httpd, ((qse_xli_str_t*)pair->val)->ptr);
		if (name)
		{
			qse_httpd_setname (httpd, name);
			qse_httpd_freemem (httpd, name);
		}
		else
		{
			/* TODO: warning */
		}
	}

	close_config_file (httpd);
	return 0;

oops:
	close_config_file (httpd);
	return -1;
}

static void reconf_httpd (qse_httpd_t* httpd, qse_httpd_ecb_reconf_type_t type)
{
	switch (type)
	{
		case QSE_HTTPD_ECB_RECONF_PRE:
			open_config_file (httpd);
			break;

		case QSE_HTTPD_ECB_RECONF_POST:
			close_config_file (httpd);
			break;
	}
}

static void reconf_server (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	httpd_xtn_t* httpd_xtn;
	server_xtn_t* server_xtn;
	qse_xli_pair_t* pair;

	httpd_xtn = qse_httpd_getxtnstd (httpd);
	server_xtn = qse_httpd_getserverstdxtn (httpd, server);

	if (httpd_xtn->xli)
	{
		qse_char_t buf[32];
		qse_sprintf (buf, QSE_COUNTOF(buf), QSE_T("server[%d]"), server_xtn->num);
		pair = qse_xli_findpairbyname (httpd_xtn->xli, QSE_NULL, buf);

		if (pair && pair->val->type == QSE_XLI_LIST) 
		{
			clear_server_config (httpd, server);
			load_server_config (httpd, server, (qse_xli_list_t*)pair->val);
		}
	}
}

/* --------------------------------------------------------------------- */
static int httpd_main (int argc, qse_char_t* argv[])
{
	qse_httpd_t* httpd = QSE_NULL;
	httpd_xtn_t* httpd_xtn;
	qse_ntime_t tmout;
	int trait, ret = -1;

	if (argc != 2)
	{
		/* TODO: proper check... */
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s -f config-file\n"), argv[0]);
		goto oops;
	}

	httpd = qse_httpd_openstd (QSE_SIZEOF(httpd_xtn_t));
	if (httpd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open httpd\n"));
		goto oops;
	}

	httpd_xtn = qse_httpd_getxtnstd (httpd);
	httpd_xtn->cfgfile = argv[1];

	httpd_xtn->ecb.reconf = reconf_httpd;
	qse_httpd_pushecb (httpd, &httpd_xtn->ecb);

	if (load_config (httpd) <= -1) goto oops;

	g_httpd = httpd;
	setup_signal_handlers ();

	qse_httpd_getopt (httpd, QSE_HTTPD_TRAIT, &trait);
	trait |= QSE_HTTPD_CGIERRTONUL;
	qse_httpd_setopt (httpd, QSE_HTTPD_TRAIT, &trait);

	tmout.sec = 10;
	tmout.nsec = 0;
	ret = qse_httpd_loopstd (httpd, &tmout);

	restore_signal_handlers ();
	g_httpd = QSE_NULL;

	if (ret <= -1) qse_fprintf (QSE_STDERR, QSE_T("Httpd error - %d\n"), qse_httpd_geterrnum (httpd));

oops:
	if (httpd) qse_httpd_close (httpd);
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int ret;

#if defined(_WIN32)
	char locale[100];
	UINT codepage;
	WSADATA wsadata;

	codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
		sprintf (locale, ".%u", (unsigned int)codepage);
		setlocale (LC_ALL, locale);
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}

	if (WSAStartup (MAKEWORD(2,0), &wsadata) != 0)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Failed to start up winsock\n"));
		return -1;
	}

#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

#if defined(HAVE_SSL)    
	SSL_load_error_strings ();
	SSL_library_init ();
#endif

	ret = qse_runmain (argc, argv, httpd_main);

#if defined(HAVE_SSL)
	/*ERR_remove_state ();*/
	ENGINE_cleanup ();
	ERR_free_strings ();
	EVP_cleanup ();
	CRYPTO_cleanup_all_ex_data ();
#endif

#if defined(_WIN32)
	WSACleanup ();	
#endif

	return ret;
}

