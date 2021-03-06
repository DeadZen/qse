/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * This file holds dns support code and is included by httpd-std.c
 */

#define DNS_MAX_DN_LEN 255 /* full domain name length in binary form (i.e. 3xyz2eu0) */
#define DNS_MAX_MSG_LEN 512  /* basic dns only. no EDNS0. so 512 at most */

#define DNS_MIN_TTL 10
#define DNS_MAX_TTL 120 /* TODO: make these configurable... */

#define DNS_SEQ_RANGE_SIZE ((QSE_TYPE_MAX(qse_uint16_t) / 2) - 2)

typedef struct dns_ctx_t dns_ctx_t;
typedef struct dns_req_t dns_req_t;
typedef struct dns_ans_t dns_ans_t;
typedef struct dns_hdr_t dns_hdr_t;
typedef struct dns_qdtrail_t dns_qdtrail_t;
typedef struct dns_antrail_t dns_antrail_t;

enum
{
	DNS_OPCODE_QUERY = 0,
	DNS_OPCODE_IQUERY = 1,
	DNS_OPCODE_STATUS = 2,
	DNS_OPCODE_NOTIFY = 4,
	DNS_OPCODE_UPDATE = 5,

	DNS_RCODE_NOERROR = 0,
	DNS_RCODE_FORMERR = 1,
	DNS_RCODE_SERVFAIL = 2,
	DNS_RCODE_NXDOMAIN = 3,
	DNS_RCODE_NOTIMPL = 4,
	DNS_RCODE_REFUSED = 5,

	DNS_QTYPE_A = 1,
	DNS_QTYPE_NS = 2,
	DNS_QTYPE_CNAME = 5,
	DNS_QTYPE_SOA = 6,
	DNS_QTYPE_PTR = 12,
	DNS_QTYPE_MX = 15,
	DNS_QTYPE_TXT = 16,
	DNS_QTYPE_AAAA = 28,
	DNS_QTYPE_OPT = 41,
	DNS_QTYPE_ANY = 255,

	DNS_QCLASS_IN = 1, /* internet */
	DNS_QCLASS_CH = 3, /* chaos */
	DNS_QCLASS_HS = 4, /* hesiod */
	DNS_QCLASS_NONE = 254,
	DNS_QCLASS_ANY = 255
};

#include <qse/pack1.h>
struct dns_hdr_t
{
	qse_uint16_t id;

#if defined(QSE_ENDIAN_BIG)
	qse_uint16_t qr: 1; /* question or response  */
	qse_uint16_t opcode: 4; 
	qse_uint16_t aa: 1; /* authoritative answer */
	qse_uint16_t tc: 1; /* truncated message */
	qse_uint16_t rd: 1; /* recursion desired */

	qse_uint16_t ra: 1; /* recursion available */
	qse_uint16_t z: 1; 
	qse_uint16_t ad: 1;
	qse_uint16_t cd: 1;
	qse_uint16_t rcode: 4;
#else
	qse_uint16_t rd: 1;
	qse_uint16_t tc: 1;
	qse_uint16_t aa: 1;
	qse_uint16_t opcode: 4;
	qse_uint16_t qr: 1;

	qse_uint16_t rcode: 4;
	qse_uint16_t cd: 1;
	qse_uint16_t ad: 1;
	qse_uint16_t z: 1; 
	qse_uint16_t ra: 1;
#endif

	qse_uint16_t qdcount; /* questions */
	qse_uint16_t ancount; /* answers */
	qse_uint16_t nscount; /* name servers */
	qse_uint16_t arcount; /* additional resource */
};

struct dns_qdtrail_t
{
	qse_uint16_t qtype;
	qse_uint16_t qclass;
};

struct dns_antrail_t
{
	qse_uint16_t qtype;
	qse_uint16_t qclass;
	qse_uint32_t ttl;
	qse_uint16_t dlen; /* data length */
};
#include <qse/unpack.h>

struct dns_ctx_t
{
	qse_httpd_t* httpd;
	qse_httpd_dns_t* dns;

	qse_skad_t skad;
	int skadlen;
	int dns_socket;

	qse_uint16_t seq;
	dns_req_t* reqs[2048]; /* TOOD: choose the right size or make it configurable. must be < DNS_SEQ_RANGE_SIZE */
	dns_ans_t* anss[2048];
	qse_uint16_t req_count; /* the number of pending requests */

	qse_uint16_t n_qcin; /* DNS_QCLASS_IN in network byte order */
	qse_uint16_t n_qta; /* DNS_QTYPE_A in network byte order */
	qse_uint16_t n_qtaaaa; /* DNS_QTYPE_AAAA in network byte order */

#if defined(AF_UNIX)
	struct sockaddr_un unix_bind_addr;
#endif
};

struct dns_req_t
{
	qse_mchar_t* name;

#define DNS_REQ_A_NX (1 << 0)
#define DNS_REQ_AAAA_NX (1 << 1)
	int flags;
	qse_uint16_t seqa, seqaaaa;

	qse_uint8_t* dn;
	qse_size_t dnlen;

	qse_httpd_resolve_t resol;
	void* ctx;

	qse_uint8_t qa[DNS_MAX_DN_LEN + QSE_SIZEOF(dns_hdr_t) + QSE_SIZEOF(dns_qdtrail_t)];
	qse_uint8_t qaaaa[DNS_MAX_DN_LEN  + QSE_SIZEOF(dns_hdr_t) + QSE_SIZEOF(dns_qdtrail_t)];
	int qalen;
	int qaaaalen;

	dns_ctx_t* dc;
	qse_skad_t dns_skad;
	int dns_skadlen;
	int dns_socket;
	int dns_retries;
	qse_ntime_t dns_tmout;

	qse_tmr_index_t tmr_tmout;

	dns_req_t* next;
	dns_req_t* prev;
};

struct dns_ans_t
{
	/* the name part must be the same as dns_req_t */
	qse_mchar_t* name; 

	/* the total size of data fields below must not be greater than
	 * the total size of data fields of dns_req_t excluding name. 
	 * this condition is required for reusing the dns_req_t chunk
	 * when caching an answer without allocating another chunk. */
	qse_nwad_t nwad;
	qse_long_t age;
	qse_uint32_t ttl;
	dns_ans_t* next;
};

#define DN_AT_END(ptr) (ptr[0] == QSE_MT('\0') || (ptr[0] == QSE_MT('.') && ptr[1] == QSE_MT('\0')))

static qse_size_t to_dn (const qse_mchar_t* str, qse_uint8_t* buf, qse_size_t bufsz)
{
	qse_uint8_t* bp = buf, * be = buf + bufsz;

	QSE_ASSERT (QSE_SIZEOF(qse_uint8_t) == QSE_SIZEOF(qse_mchar_t));

	if (!DN_AT_END(str))
	{
		qse_uint8_t* lp;
		qse_size_t len;
		const qse_mchar_t* seg;
		const qse_mchar_t* cur = str - 1;

		do
		{
			if (bp < be) lp = bp++;
			else lp = QSE_NULL;

			seg = ++cur;
			while (*cur != QSE_MT('\0') && *cur != QSE_MT('.'))
			{
				if (bp < be) *bp++ = *cur; 
				cur++;
			}
			len = cur - seg;
			if (len <= 0 || len > 63) return 0;

			if (lp) *lp = (qse_uint8_t)len;
		}
		while (!DN_AT_END(cur));
	}

	if (bp < be) *bp++ = 0;
	return bp - buf;
}

static qse_size_t dn_length (qse_uint8_t* ptr, qse_size_t len)
{
	qse_uint8_t* curptr;
	qse_size_t curlen, seglen;

	curptr = ptr;
	curlen = len;

	do
	{
		if (curlen <= 0) return 0;

		seglen = *curptr++;
		curlen = curlen - 1;
		if (seglen == 0) break;
		else if (seglen > curlen || seglen > 63) return 0;

		curptr += seglen;
		curlen -= seglen;
	}
	while (1);

	return curptr - ptr;
}

int init_dns_query (qse_uint8_t* buf, qse_size_t len, const qse_mchar_t* name, int qtype, qse_uint16_t seq)
{
	dns_hdr_t* hdr;
	dns_qdtrail_t* qdtrail;
	qse_size_t x;

	if (len < QSE_SIZEOF(*hdr)) return -1;

	QSE_MEMSET (buf, 0, len);
	hdr = (dns_hdr_t*)buf;
	hdr->id = qse_hton16(seq);
	hdr->opcode = DNS_OPCODE_QUERY; 
	hdr->rd = 1;  /* recursion desired*/
	hdr->qdcount = qse_hton16(1); /* 1 question */

	len -= QSE_SIZEOF(*hdr);
	
	x = to_dn (name, (qse_uint8_t*)(hdr + 1), len);
	if (x <= 0) return -1;
	len -= x;

	if (len < QSE_SIZEOF(*qdtrail)) return -1;
	qdtrail = (dns_qdtrail_t*)((qse_uint8_t*)(hdr + 1) + x);

	qdtrail->qtype = qse_hton16(qtype);
	qdtrail->qclass = qse_hton16(DNS_QCLASS_IN);
	return QSE_SIZEOF(*hdr) + x + QSE_SIZEOF(*qdtrail);
}

static int dns_open (qse_httpd_t* httpd, qse_httpd_dns_t* dns)
{
	qse_nwad_t nwad;
	dns_ctx_t* dc;
	httpd_xtn_t* httpd_xtn;

	httpd_xtn = qse_httpd_getxtn (httpd);

	dns->handle[0] = QSE_INVALID_SCKHND;
	dns->handle[1] = QSE_INVALID_SCKHND;
	dns->handle[2] = QSE_INVALID_SCKHND;

	dc = (dns_ctx_t*) qse_httpd_callocmem (httpd, QSE_SIZEOF(dns_ctx_t));
	if (dc == NULL) goto oops;

	dc->httpd = httpd;
	dc->dns = dns;
	dc->n_qcin = qse_hton16(DNS_QCLASS_IN);
	dc->n_qta = qse_hton16(DNS_QTYPE_A);
	dc->n_qtaaaa = qse_hton16(DNS_QTYPE_AAAA);

/* TODO: add static cache entries from /etc/hosts */

	nwad = httpd_xtn->dns.nwad;
	if (nwad.type == QSE_NWAD_NX)
	{
		/* read the system dns configuration */

		qse_sio_t* sio;

	#if defined(_WIN32)
		/* TODO: windns.h dnsapi.lib DnsQueryConfig... */
	#elif defined(__OS2__)
		/* TODO: */
	#else
		/* TODO: read /etc/resolv.conf???? */
	#endif

		sio = qse_sio_open (qse_httpd_getmmgr(httpd), 0, QSE_T("/etc/resolv.conf"), QSE_SIO_READ);
		if (sio)
		{
			qse_mchar_t buf[128];
			qse_ssize_t len;
			qse_mcstr_t tok;
			qse_mchar_t* ptr;
			qse_mchar_t* end;

			while (1)
			{
				len = qse_sio_getmbsn (sio, buf, QSE_COUNTOF(buf));
				if (len <= 0) break;

				end = buf + len;
				ptr = buf;

				ptr = qse_mbsxtok (ptr, end - ptr, QSE_MT(" \t"), &tok);
				if (ptr && qse_mbsxcmp (tok.ptr, tok.len, QSE_MT("nameserver")) == 0)
				{
					ptr = qse_mbsxtok (ptr, end - ptr, QSE_MT(" \t"), &tok);
					if (qse_mbsntonwad (tok.ptr, tok.len, &nwad) >= 0) break;
				}
			}
			qse_sio_close (sio);
		}
	}

	if (nwad.type != QSE_NWAD_NX && qse_getnwadport(&nwad) == 0) 
		qse_setnwadport (&nwad, qse_hton16(QSE_HTTPD_DNSSTD_DEFAULT_PORT));

#if defined(QSE_HTTPD_DEBUG)
	{
		qse_mchar_t tmp[128];
		qse_nwadtombs (&nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
		HTTPD_DBGOUT1 ("Default DNS server set to [%hs]\n", tmp);
	}
#endif

	dns->handle[0] = open_client_socket (httpd, AF_INET, SOCK_DGRAM, 0);
#if defined(AF_INET6)
	dns->handle[1] = open_client_socket (httpd, AF_INET6, SOCK_DGRAM, 0);
#endif
#if defined(AF_UNIX)
	dns->handle[2] = open_client_socket (httpd, AF_UNIX, SOCK_DGRAM, 0);
#endif

	if (qse_isvalidsckhnd(dns->handle[2]))
	{
	#if defined(AF_UNIX)
		qse_ntime_t now;

		qse_gettime (&now);

		QSE_MEMSET (&dc->unix_bind_addr, 0, QSE_SIZEOF(dc->unix_bind_addr));
		dc->unix_bind_addr.sun_family = AF_UNIX;

		/* TODO: make the location(/tmp) or the prefix(.dns-) of the socket file configurable??? */
		qse_mbsxfmt (
			dc->unix_bind_addr.sun_path, 
			QSE_COUNTOF(dc->unix_bind_addr.sun_path),
			QSE_MT("/tmp/.dns-%x-%lu"), (int)QSE_GETPID(), (unsigned long int)dns->handle[2]);
		QSE_UNLINK (dc->unix_bind_addr.sun_path);
		if (bind (dns->handle[2], (struct sockaddr*)&dc->unix_bind_addr, QSE_SIZEOF(dc->unix_bind_addr)) <= -1)
		{
			qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
			qse_closesckhnd (dns->handle[2]);
			dns->handle[2] = QSE_INVALID_SCKHND;
		}
	#endif
	}

	if (!qse_isvalidsckhnd(dns->handle[0]) &&
	    !qse_isvalidsckhnd(dns->handle[1]) &&
	    !qse_isvalidsckhnd(dns->handle[2]))
	{
		/* don't set the error number here.
		 * open_client_socket() should set it */
		goto oops;
	}

	/* carry on regardless of success or failure */
	dc->skadlen = qse_nwadtoskad (&nwad, &dc->skad);

	/* determine which socket to use when sending a request to the default server */
	if (dc->skadlen >=  0)
	{
		switch (nwad.type)
		{
			case QSE_NWAD_IN4:
				dc->dns_socket = dns->handle[0];
				break;

			case QSE_NWAD_IN6:
				dc->dns_socket = dns->handle[1];
				break;

			case QSE_NWAD_LOCAL:
				dc->dns_socket = dns->handle[2];
				break;

			default:
				/* unsupported address type for the default server */
				dc->dns_socket = QSE_INVALID_SCKHND;
				break;
		}
	}
	else
	{
		dc->dns_socket = QSE_INVALID_SCKHND;
	}

	dns->handle_count = 3;
	if (qse_isvalidsckhnd(dns->handle[0])) dns->handle_mask |= (1 << 0);
	if (qse_isvalidsckhnd(dns->handle[1])) dns->handle_mask |= (1 << 1);
	if (qse_isvalidsckhnd(dns->handle[2])) dns->handle_mask |= (1 << 2);

	dns->ctx = dc;

	return 0;

oops:
	if (qse_isvalidsckhnd(dns->handle[0])) qse_closesckhnd (dns->handle[0]);
	if (qse_isvalidsckhnd(dns->handle[1])) qse_closesckhnd (dns->handle[1]);
	if (qse_isvalidsckhnd(dns->handle[2]))
	{
		qse_closesckhnd (dns->handle[2]);
	#if defined(AF_UNIX)
		QSE_UNLINK (dc->unix_bind_addr.sun_path);
	#endif
	}
	if (dc) qse_httpd_freemem (httpd, dc);
	return -1;
}

static void dns_remove_tmr_tmout (qse_httpd_t* httpd, dns_req_t* req)
{
	if (req->tmr_tmout != QSE_TMR_INVALID_INDEX)
	{
		qse_httpd_remove_timer_event (httpd, req->tmr_tmout);
		req->tmr_tmout = QSE_TMR_INVALID_INDEX;
	}
}

static void dns_close (qse_httpd_t* httpd, qse_httpd_dns_t* dns)
{
	dns_ctx_t* dc = (dns_ctx_t*)dns->ctx;
	qse_size_t i;

	for (i = 0; i < QSE_COUNTOF(dc->reqs); i++)
	{
		dns_req_t* next_req;
		while (dc->reqs[i])
		{
			next_req = dc->reqs[i]->next;

			dns_remove_tmr_tmout (httpd, dc->reqs[i]);
			qse_httpd_freemem (httpd, dc->reqs[i]);

			dc->reqs[i] = next_req;
			dc->req_count--;
		}
	}

	QSE_ASSERT (dc->req_count == 0);

	for (i = 0; i < QSE_COUNTOF(dc->anss); i++)
	{
		dns_ans_t* next_ans;
		while (dc->anss[i])
		{
			next_ans = dc->anss[i]->next;
			qse_httpd_freemem (httpd, dc->anss[i]);
			dc->anss[i] = next_ans;
		}
	}

	if (qse_isvalidsckhnd(dns->handle[0])) qse_closesckhnd (dns->handle[0]);
	if (qse_isvalidsckhnd(dns->handle[1])) qse_closesckhnd (dns->handle[1]);
	if (qse_isvalidsckhnd(dns->handle[2]))
	{
		qse_closesckhnd (dns->handle[2]);
	#if defined(AF_UNIX)
		QSE_UNLINK (dc->unix_bind_addr.sun_path);
	#endif
	}
	qse_httpd_freemem (httpd, dns->ctx);
}

static void dns_cache_answer (dns_ctx_t* dc, dns_req_t* req, const qse_nwad_t* nwad, qse_uint32_t ttl)
{
	dns_ans_t* ans, * prv, * cur;
	qse_size_t hid;
	qse_ntime_t now;

/* TODO: implement the maximum number of entries in cache... */

	/* i use the given request as a space to hold an answer.
	 * the following assertion must be met for this to work */
	QSE_ASSERT (QSE_SIZEOF(dns_req_t) >= QSE_SIZEOF(dns_ans_t));

	qse_gettime (&now);

	ans = (dns_ans_t*)req; /* shadow the request with an answer */

	/* reuse the data fields of the request except the name field.
	 * from here downwards, the data fields of the request are invalid. */

	if (nwad) ans->nwad = *nwad; /* positive */
	else ans->nwad.type = QSE_NWAD_NX; /* negative */
	ans->age = now.sec; /* the granularity of a second should be good enough */

	if (ttl < DNS_MIN_TTL) ttl = DNS_MIN_TTL; /* TODO: use configured value */
	else if (ttl > DNS_MAX_TTL) ttl = DNS_MAX_TTL;

	ans->ttl = ttl;
	hid = hash_string (req->name) % QSE_COUNTOF(dc->anss);

	prv = QSE_NULL;
	cur = dc->anss[hid];
	while (cur)
	{
		if (qse_mbscasecmp(cur->name, ans->name) == 0)
		{
			ans->next = cur->next;
			if (prv) prv->next = ans;
			else dc->anss[hid] = ans;
			qse_httpd_freemem (dc->httpd, cur);
			return;
		}

		prv = cur;
		cur = cur->next;
	}
	ans->next = dc->anss[hid];
	dc->anss[hid] = ans;
}

static dns_ans_t* dns_get_answer_from_cache (dns_ctx_t* dc, const qse_mchar_t* name)
{
	dns_ans_t* prv, * cur;
	qse_size_t hid;
	qse_ntime_t now;

	hid = hash_string(name) % QSE_COUNTOF(dc->anss);

	qse_gettime (&now);

	prv = QSE_NULL;
	cur = dc->anss[hid];
	while (cur)
	{
		if (qse_mbscasecmp(cur->name, name) == 0)
		{
			if (cur->age + cur->ttl < now.sec)
			{
				/* entry expired. evict the entry from the cache */
				if (prv) prv->next = cur->next;
				else dc->anss[hid] = cur->next;
				qse_httpd_freemem (dc->httpd, cur);
				break;
			}

			return cur;
		}

		prv = cur;
		cur = cur->next;
	}

	return QSE_NULL;
}

static int dns_recv (qse_httpd_t* httpd, qse_httpd_dns_t* dns, qse_httpd_hnd_t handle)
{
	dns_ctx_t* dc = (dns_ctx_t*)dns->ctx;
	httpd_xtn_t* httpd_xtn;

	qse_skad_t fromaddr;
	qse_sck_len_t fromlen;

	qse_uint8_t buf[DNS_MAX_MSG_LEN];
	qse_ssize_t len;
	dns_hdr_t* hdr;

	qse_uint16_t id, qdcount, ancount, i;
	qse_uint8_t* plptr;
	qse_size_t pllen;
	dns_qdtrail_t* qdtrail;
	dns_antrail_t* antrail;
	qse_uint16_t anlen;

	dns_req_t* req = QSE_NULL;
	qse_uint16_t xid = QSE_COUNTOF(dc->reqs);
	qse_nwad_t nwad;
	qse_nwad_t* resolved_nwad = QSE_NULL;
	int cache_ttl = 0;

	httpd_xtn = qse_httpd_getxtn (httpd);

	fromlen = QSE_SIZEOF(fromaddr);
	len = recvfrom (handle, buf, QSE_SIZEOF(buf), 0, (struct sockaddr*)&fromaddr, &fromlen);

#if defined(QSE_HTTPD_DEBUG)
	{
		qse_nwad_t tmpnwad;
		qse_mchar_t tmp[128];
		qse_skadtonwad (&fromaddr, &tmpnwad);

		qse_nwadtombs (&tmpnwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
		HTTPD_DBGOUT2 ("Received DNS response of length %d from %s\n", (int)len, tmp);
	}
#endif
	if (len < QSE_SIZEOF(*hdr)) goto done; /* packet too small */

	hdr = (dns_hdr_t*)buf;
	qdcount = qse_ntoh16(hdr->qdcount);

	if (!hdr->qr || hdr->opcode != DNS_OPCODE_QUERY || qdcount <= 0) 
	{
		/* not a response to a query */
		goto done;
	}

	ancount = qse_ntoh16(hdr->ancount);
	id = qse_ntoh16(hdr->id);
	xid = (id >= DNS_SEQ_RANGE_SIZE)? (id - DNS_SEQ_RANGE_SIZE): id;
	xid = xid % QSE_COUNTOF(dc->reqs);

	plptr = (qse_uint8_t*)(hdr + 1);
	pllen = len - QSE_SIZEOF(*hdr);

	/* inspect the question section */
	for (i = 0; i < qdcount; i++)
	{
		qse_size_t reclen;
		qse_size_t dnlen;

		dnlen = dn_length (plptr, pllen);
		if (dnlen <= 0) goto done; /* invalid dn name */

		reclen = dnlen + QSE_SIZEOF(dns_qdtrail_t);
		if (pllen < reclen) goto done; /* weird packet */

		if (!req)
		{
			qdtrail = (dns_qdtrail_t*)(plptr + dnlen);

			if (qdtrail->qclass == dc->n_qcin &&
			    (qdtrail->qtype == dc->n_qta || qdtrail->qtype == dc->n_qtaaaa))
			{
				for (req = dc->reqs[xid]; req; req = req->next)
				{
					if ((id == req->seqa || id == req->seqaaaa) &&
					    req->dnlen == dnlen && QSE_MEMCMP (req->dn, plptr, req->dnlen) == 0) 
					{
/* TODO: check if fromadd/fromlen matches req->dns_skad/req->dns_skadlen */
						/* found a match. note that the test here is a bit loose
						 * in that it doesn't really check if the original question 
						 * was A or AAAA. it is possible that it can process an AAAA answer
						 * for an A question and vice versa. i don't care if someone
						 * exploits this and sends a fake response*/
						break;
					}
				}
			}
		}

		plptr += reclen; 
		pllen -= reclen;
	}

	if (!req) goto done; /* no matching request for the question */
	if (hdr->rcode != DNS_RCODE_NOERROR || ancount <= 0) goto done; /* no good answers */

	/* inspect the answer section */
	for (i = 0; i < ancount; i++)
	{
		qse_size_t reclen;
		qse_size_t dnlen;

		if (pllen < 1) goto done; /* weird length */

		if (*plptr > 63) 
		{
/* TODO TODO TODO TODO */

			/* RFC1035
			 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
			 * | 1  1|                OFFSET                   |
			 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
			 */

			/* this is not really right. the second segment or the 
			 * subsequent segments can be pointing to somewhere else.
			 * TODO: fix the problem. dn_legnth() needs
			 *       to use the original request packet also */
			dnlen = 2;
		}
		else
		{
			dnlen = dn_length (plptr, pllen);
			if (dnlen <= 0) return 0; /* invalid dn name */
		}

		reclen = dnlen + QSE_SIZEOF(dns_antrail_t);
		if (pllen < reclen) goto done;

		antrail = (dns_antrail_t*)(plptr + dnlen);
		reclen += qse_ntoh16(antrail->dlen);
		if (pllen < reclen) goto done;

		anlen = qse_ntoh16(antrail->dlen);

		if (antrail->qclass == dc->n_qcin)
		{
			nwad.type = QSE_NWAD_NX;

			if (antrail->qtype == dc->n_qta && anlen == 4)
			{
				QSE_MEMSET (&nwad, 0, QSE_SIZEOF(nwad));
				nwad.type = QSE_NWAD_IN4;
				QSE_MEMCPY (&nwad.u.in4.addr, antrail + 1, 4);
			}
			else if (antrail->qtype == dc->n_qtaaaa && anlen == 16)
			{
				QSE_MEMSET (&nwad, 0, QSE_SIZEOF(nwad));
				nwad.type = QSE_NWAD_IN6;
				QSE_MEMCPY (&nwad.u.in6.addr, antrail + 1, 16);
			}

			if (nwad.type != QSE_NWAD_NX)
			{
				cache_ttl = httpd_xtn->dns.cache_ttl;
				if (cache_ttl > qse_ntoh32(antrail->ttl)) cache_ttl = qse_ntoh32(antrail->ttl);
				if (cache_ttl < httpd_xtn->dns.cache_minttl) cache_ttl = httpd_xtn->dns.cache_minttl;

				resolved_nwad = &nwad;
				goto resolved;
			}
		}

		plptr += reclen;
		pllen -= reclen;
	}

	/* no good answer have been found */
	if (id == req->seqa) req->flags |= DNS_REQ_A_NX;
	else if (id == req->seqaaaa) req->flags |= DNS_REQ_AAAA_NX;

	if ((req->flags & (DNS_REQ_A_NX | DNS_REQ_AAAA_NX)) == (DNS_REQ_A_NX | DNS_REQ_AAAA_NX))
	{
		/* both ipv4 and ipv6 address are unresolvable */
		cache_ttl = httpd_xtn->dns.cache_negttl;
		resolved_nwad = QSE_NULL;
		goto resolved;
	}

done:
	/* is there anything to do here? */
	return 0;

resolved:
	QSE_ASSERT (req != QSE_NULL);
	QSE_ASSERT (xid >= 0 && xid < QSE_COUNTOF(dc->reqs));

	dns_remove_tmr_tmout (httpd, req);
	req->resol (httpd, req->name, resolved_nwad, req->ctx);

	/* detach the request off dc->reqs */
	if (req == dc->reqs[xid]) dc->reqs[xid] = req->next;
	else req->prev->next = req->next;
	if (req->next) req->next->prev = req->prev;

	/* cache the answer instead of destroying it */
	dns_cache_answer (dc, req, resolved_nwad, cache_ttl);
	dc->req_count--;

	return 0;
}

static void tmr_dns_tmout_update (qse_tmr_t* tmr, qse_tmr_index_t old_index, qse_tmr_index_t new_index, qse_tmr_event_t* evt)
{
	dns_req_t* req = (dns_req_t*)evt->ctx;
	QSE_ASSERT (req->tmr_tmout == old_index);
	req->tmr_tmout = new_index;
}

static void tmr_dns_tmout_handle (qse_tmr_t* tmr, const qse_ntime_t* now, qse_tmr_event_t* evt)
{
	/* destory the unanswered request if timed out */

	dns_req_t* req = (dns_req_t*)evt->ctx;
	dns_ctx_t* dc = req->dc;
	qse_uint16_t xid;

	HTTPD_DBGOUT1 ("DNS timed out [%s]\n", req->name);

	/* when this handler is called, the event must be removed from the timer */
	QSE_ASSERT (req->tmr_tmout == QSE_TMR_INVALID_INDEX);

	/* ---------------------------------------------------------------
	 * resend 
	 *---------------------------------------------------------------- */
	if (req->dns_retries > 0)
	{
		/*httpd_xtn_t* httpd_xtn;*/
		qse_tmr_event_t tmout_event;

		/*httpd_xtn = qse_httpd_getxtn (dc->httpd);*/

		QSE_MEMSET (&tmout_event, 0, QSE_SIZEOF(tmout_event));
		qse_gettime (&tmout_event.when);
		qse_addtime (&tmout_event.when, &req->dns_tmout, &tmout_event.when);
		tmout_event.ctx = req;
		tmout_event.handler = tmr_dns_tmout_handle;
		tmout_event.updater = tmr_dns_tmout_update;

		if ((!(req->flags & DNS_REQ_A_NX) && req->qalen > 0 && sendto (req->dns_socket, req->qa, req->qalen, 0, (struct sockaddr*)&req->dns_skad, req->dns_skadlen) != req->qalen) ||
			(!(req->flags & DNS_REQ_AAAA_NX) && req->qaaaalen > 0 && sendto (req->dns_socket, req->qaaaa, req->qaaaalen, 0, (struct sockaddr*)&req->dns_skad, req->dns_skadlen) != req->qaaaalen))
		{
			/* resend failed. fall thru and destroy the request*/

			/* Unix datagram socket seems to fail with EAGAIN often 
			 * even with increased SO_SNDBUF size. */
			if (dc->httpd->errnum == QSE_HTTPD_EAGAIN && req->dns_retries > 1)
			{
				/* TODO: check writability of req->urs_socket instead of just retrying... */
				goto send_ok;
			}
		}
		else
		{
		send_ok:
			QSE_ASSERT (tmr == dc->httpd->tmr);
			if (qse_httpd_insert_timer_event (dc->httpd, &tmout_event, &req->tmr_tmout) >= 0)
			{
				req->dns_retries--;
				return; /* resend ok */
			}
		}
	}

	/* ---------------------------------------------------------------
	 * dns timed out + no resend 
	 *---------------------------------------------------------------- */

	/* it's safe to use req->seqa to find the hash index
	 * because seqa is always set regardless of A or AAAA */
	xid = req->seqa % QSE_COUNTOF(dc->reqs);

	/* detach the request off dc->reqs */
	if (req == dc->reqs[xid]) dc->reqs[xid] = req->next;
	else req->prev->next = req->next;
	if (req->next) req->next->prev = req->prev;

	/* dns timed out. report that name resolution failed */
	req->resol (dc->httpd, req->name, QSE_NULL, req->ctx);

	/* i don't cache the items that have timed out */
	qse_httpd_freemem (dc->httpd, req);

	/* decrement the number of pending requests */
	dc->req_count--;
}

static int dns_send (qse_httpd_t* httpd, qse_httpd_dns_t* dns, const qse_mchar_t* name, qse_httpd_resolve_t resol, const qse_httpd_dns_server_t* dns_server, void* ctx)
{
	dns_ctx_t* dc = (dns_ctx_t*)dns->ctx;
	httpd_xtn_t* httpd_xtn;

	qse_uint32_t seq;
	qse_uint16_t xid;
	dns_req_t* req = QSE_NULL;
	qse_size_t name_len;
	dns_ans_t* ans;
	qse_tmr_event_t tmout_event;
	int dns_flags;

	httpd_xtn = qse_httpd_getxtn (httpd);

	ans = dns_get_answer_from_cache (dc, name);
	if (ans)
	{
		resol (httpd, name, ((ans->nwad.type == QSE_NWAD_NX)? QSE_NULL: &ans->nwad), ctx);
		return 0;
	}

	if (dc->req_count >= QSE_COUNTOF(dc->reqs))
	{
		/* too many pending requests */
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOBUF);
		goto oops;
	}

	seq = ((qse_uint32_t)dc->seq + 1) % DNS_SEQ_RANGE_SIZE;
	dc->seq = seq;

	xid = seq % QSE_COUNTOF(dc->reqs); 

	name_len = qse_mbslen(name);

	/* dn is at most as long as the source length + 2.
	 *  a.bb.ccc => 1a2bb3ccc0  => +2
	 *  a.bb.ccc. => 1a2bb3ccc0  => +1 */
	req = qse_httpd_callocmem (httpd, QSE_SIZEOF(*req) + (name_len + 1) + (name_len + 2));
	if (req == QSE_NULL) goto oops;

	req->tmr_tmout = QSE_TMR_INVALID_INDEX;

	/* seqa is between 0 and DNS_SEQ_RANGE_SIZE - 1 inclusive.
	 * seqaaaa is between DNS_SEQ_RANGE_SIZE and DNS_SEQ_RANGE_SIZE * 2 - 1 inclusive. */
	req->seqa = seq;
	req->seqaaaa = seq + DNS_SEQ_RANGE_SIZE; /* this must not go beyond QSE_TYPE_MAX(qse_uint16_t) */
	req->name = (qse_mchar_t*)(req + 1);
	req->dn = (qse_uint8_t*)(req->name + name_len + 1);

	qse_mbscpy (req->name, name);
	req->dnlen = to_dn (name, req->dn, name_len + 2);
	if (req->dnlen <= 0)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);
		goto oops;
	}
	req->resol = resol;
	req->ctx = ctx;

	req->dns_retries = httpd_xtn->dns.retries;
	req->dns_tmout = httpd_xtn->dns.tmout;
	dns_flags = QSE_HTTPD_DNS_SERVER_A | QSE_HTTPD_DNS_SERVER_AAAA;

	if (dns_server)
	{
		if (dns_server->retries >= 0) req->dns_retries = dns_server->retries;
		if (dns_server->tmout.sec >= 0) req->dns_tmout = dns_server->tmout;

		req->dns_skadlen = qse_nwadtoskad (&dns_server->nwad, &req->dns_skad);
		if (req->dns_skadlen <= -1) goto default_dns_server;

		switch (dns_server->nwad.type)
		{
			case QSE_NWAD_IN4:
				req->dns_socket = dns->handle[0];
				break;
			case QSE_NWAD_IN6:
				req->dns_socket = dns->handle[1];
				break;
			case QSE_NWAD_LOCAL:
				req->dns_socket = dns->handle[2];
				break;
			default:
				/* TODO: should it return failure with QSE_HTTPD_EINVAL? */
				goto default_dns_server;
		}

		dns_flags = dns_server->flags;
	}
	else
	{
	default_dns_server:
		req->dns_skad = dc->skad;
		req->dns_skadlen = dc->skadlen;
		req->dns_socket = dc->dns_socket;
	}

	if (dns_flags & QSE_HTTPD_DNS_SERVER_A)
		req->qalen = init_dns_query (req->qa, QSE_SIZEOF(req->qa), name, DNS_QTYPE_A, req->seqa);
	else
		req->flags |= DNS_REQ_A_NX;

	if (dns_flags & QSE_HTTPD_DNS_SERVER_AAAA)
		req->qaaaalen = init_dns_query (req->qaaaa, QSE_SIZEOF(req->qaaaa), name, DNS_QTYPE_AAAA, req->seqaaaa);
	else
		req->flags |= DNS_REQ_AAAA_NX;

	if (req->qalen <= -1 || req->qaaaalen <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);
		goto oops;
	}

	QSE_MEMSET (&tmout_event, 0, QSE_SIZEOF(tmout_event));
	qse_gettime (&tmout_event.when);
	qse_addtime (&tmout_event.when, &req->dns_tmout, &tmout_event.when);
	tmout_event.ctx = req;
	tmout_event.handler = tmr_dns_tmout_handle;
	tmout_event.updater = tmr_dns_tmout_update;
	if (qse_httpd_insert_timer_event (httpd, &tmout_event, &req->tmr_tmout) <= -1) goto oops;

	if ((req->qalen > 0 && sendto (req->dns_socket, req->qa, req->qalen, 0, (struct sockaddr*)&req->dns_skad, req->dns_skadlen) != req->qalen) ||
	    (req->qaaaalen > 0 && sendto (req->dns_socket, req->qaaaa, req->qaaaalen, 0, (struct sockaddr*)&req->dns_skad, req->dns_skadlen) != req->qaaaalen))
	{
		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		/* Unix datagram socket seems to fail with EAGAIN often 
		 * even with increased SO_SNDBUF size. */
		if (httpd->errnum == QSE_HTTPD_EAGAIN && req->dns_retries > 0)
		{
			/* TODO: check writability of req->urs_socket instead of just retrying... */
			goto send_ok;
		}

		goto oops;
		
	}

send_ok:
	/* NOTE: 
	 *  if the sequence number is repeated before it timed out or resolved,
	 *  the newer request gets chained together with the older one.
	 *  it may not be so easy to determine which request to match an incoming 
	 *  response. 
	 */
	req->dc = dc;

	/* link the request to the front of the chain */
	if (dc->reqs[xid]) dc->reqs[xid]->prev = req;
	req->next = dc->reqs[xid];
	dc->reqs[xid] = req;

	/* increment the number of pending requests */
	dc->req_count++;

	return 0;

oops:
	if (req)
	{
		dns_remove_tmr_tmout (httpd, req);
		qse_httpd_freemem (httpd, req);
	}
	return -1;
}

static int dns_preresolve (qse_httpd_t* httpd, qse_httpd_client_t* client, const qse_mchar_t* host, qse_nwad_t* nwad)
{
	/* do nothing */
	return 1; /* unhandled */
}

