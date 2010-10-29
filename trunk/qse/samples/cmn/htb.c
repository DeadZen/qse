#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/stdio.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static qse_htb_mancbs_t mancbs1 =
{
	{
		QSE_HTB_COPIER_INLINE,
		QSE_HTB_COPIER_INLINE
	},
	{
		QSE_HTB_FREEER_DEFAULT,
		QSE_HTB_FREEER_DEFAULT
	},
	QSE_HTB_HASHER_DEFAULT,
	QSE_HTB_COMPER_DEFAULT,
	QSE_HTB_KEEPER_DEFAULT,
	QSE_HTB_SIZER_DEFAULT
};


static int test1_build (qse_htb_t* s1)
{
	int i;


	for (i = 1; i < 50; i++)
	{
		int j = i * 2;
		qse_printf (QSE_T("inserting a key %d and a value %d: "), i, j);

		if (qse_htb_insert (s1, &i, sizeof(i), &j, sizeof(j)) == QSE_NULL)
		{
			qse_printf (QSE_T("[FAILED]\n"));
			return -1;
		}
		
		qse_printf (QSE_T("[OK]\n"));
	}

	return 0;
}

static void test1_delete (qse_htb_t* s1)
{
	int i;
	int t[] = { 20, 11, 13, 40 };
	int t2[] = { 22, 21, 31, 14, 48, 32, 29 };

	for (i = 1; i < 53; i+=2)
	{
		qse_printf (QSE_T("deleting a key %d: "), i);
		if (qse_htb_delete (s1, &i, sizeof(i)) == -1)
			qse_printf (QSE_T("[FAILED]\n"));
		else
			qse_printf (QSE_T("[OK]\n"));
	}


	for (i = 0; i < QSE_COUNTOF(t); i++)
	{
		int k = t[i];
		int v = k * 1000;

		qse_printf (QSE_T("updating a key %d value %d: "), k, v);
		if (qse_htb_update (s1, &k, sizeof(k), &v, sizeof(v)) == QSE_NULL)
			qse_printf (QSE_T("[FAILED]\n"));
		else
			qse_printf (QSE_T("[OK]\n"));
	}

	for (i = 0; i < QSE_COUNTOF(t); i++)
	{
		int k = t[i];
		int v = k * 1000;

		qse_printf (QSE_T("inserting a key %d value %d: "), k, v);
		if (qse_htb_insert (s1, &k, sizeof(k), &v, sizeof(v)) == QSE_NULL)
			qse_printf (QSE_T("[FAILED]\n"));
		else
			qse_printf (QSE_T("[OK]\n"));
	}

	for (i = 0; i < QSE_COUNTOF(t2); i++)
	{
		int k = t2[i];
		int v = k * 2000;

		qse_printf (QSE_T("upserting a key %d value %d: "), k, v);
		if (qse_htb_upsert (s1, &k, sizeof(k), &v, sizeof(v)) == QSE_NULL)
			qse_printf (QSE_T("[FAILED]\n"));
		else
			qse_printf (QSE_T("[OK]\n"));
	}
}

static void test1_dump (qse_htb_t* s1)
{
	int i;

	qse_printf (QSE_T("map contains %u pairs\n"), (unsigned)QSE_HTB_SIZE(s1)); 
	for (i = 1; i < 50; i++)
	{
		qse_htb_pair_t* p = qse_htb_search (s1, &i, sizeof(i));
		if (p == QSE_NULL)
		{
			qse_printf (QSE_T("%d => unknown\n"), i);
		}
		else
		{
			qse_printf (QSE_T("%d => %d(%d)\n"), 
				i, *(int*)QSE_HTB_VPTR(p), (int)QSE_HTB_VLEN(p));
		}
	}
}

static int test1 ()
{
	qse_htb_t* s1;

	s1 = qse_htb_open (QSE_MMGR_GETDFL(), 0, 5, 70, 1, 1);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a map\n"));
		return -1;
	}
	qse_htb_setmancbs (s1, &mancbs1);

	if (test1_build(s1) == -1) 
	{
		qse_htb_close (s1);
		return -1;
	}
	test1_dump (s1);

	test1_delete (s1);
	test1_dump (s1);

	qse_htb_close (s1);
	return 0;
}

qse_htb_walk_t print_map_pair (qse_htb_t* map, qse_htb_pair_t* pair, void* arg)
{
	qse_printf (QSE_T("%.*s[%d] => %.*s[%d]\n"), 
		(int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair), (int)QSE_HTB_KLEN(pair),
		(int)QSE_HTB_VLEN(pair), QSE_HTB_VPTR(pair), (int)QSE_HTB_VLEN(pair));

	return QSE_HTB_WALK_FORWARD;
}

static int test2 ()
{
	qse_htb_t* s1;
	int i;
	qse_char_t* keys[] = 
	{
		QSE_T("if you ever happen"),
		QSE_T("to want to link againt"),
		QSE_T("installed libraries"),
		QSE_T("in a given directory")
	};

	qse_char_t* vals[] = 
	{
		QSE_T("what the hell is this"),
		QSE_T("oh my goddess"),
		QSE_T("hello mr monkey"),
		QSE_T("is this good?")
	};

	s1 = qse_htb_open (QSE_MMGR_GETDFL(), 0, 1, 70, 
		QSE_SIZEOF(qse_char_t), QSE_SIZEOF(qse_char_t));
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a map\n"));
		return -1;
	}
	qse_htb_setmancbs (s1, &mancbs1);

	for (i = 0; i < QSE_COUNTOF(keys); i++)
	{
		int vi = QSE_COUNTOF(keys)-i-1;

		qse_printf (QSE_T("inserting a key [%s] and a value [%s]: "), keys[i], keys[vi]);
		if (qse_htb_insert (s1, 
			keys[i], qse_strlen(keys[i]), 
			keys[vi], qse_strlen(keys[vi])) == QSE_NULL)
		{
			qse_printf (QSE_T("[FAILED]\n"));
		}
		else
		{	
			qse_printf (QSE_T("[OK]\n"));
		}
	}
		
	qse_htb_walk (s1, print_map_pair, QSE_NULL);

	for (i = 0; i < QSE_COUNTOF(keys); i++)
	{
		qse_printf (QSE_T("updating a key [%s] and a value [%s]: "), keys[i], vals[i]);
		if (qse_htb_update (s1, 
			keys[i], qse_strlen(keys[i]), 
			vals[i], qse_strlen(vals[i])) == QSE_NULL)
		{
			qse_printf (QSE_T("[FAILED]\n"));
		}
		else
		{	
			qse_printf (QSE_T("[OK]\n"));
		}
	}
	qse_htb_walk (s1, print_map_pair, QSE_NULL);
		

	qse_htb_close (s1);
	return 0;
}

#if 0
static int comp_key (qse_htb_t* map, 
	const void* kptr1, qse_size_t klen1,
	const void* kptr2, qse_size_t klen2)
{
	return qse_strxncasecmp (kptr1, klen1, kptr2, klen2);
}

qse_htb_walk_t print_map_pair_3 (qse_htb_t* map, qse_htb_pair_t* pair, void* arg)
{
	qse_printf (QSE_T("%.*s[%d] => %d\n"), 
		(int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair), (int)QSE_HTB_KLEN(pair),
		*(int*)QSE_HTB_VPTR(pair));

	return QSE_HTB_WALK_FORWARD;
}

static int test3 ()
{
	qse_htb_t* s1;
	int i;
	qse_char_t* keys[] = 
	{
		QSE_T("one"),
		QSE_T("two"),
		QSE_T("three"),
		QSE_T("four"),
		QSE_T("ONE"),
		QSE_T("Two"),
		QSE_T("thRee")
	};

	s1 = qse_htb_open (QSE_MMGR_GETDFL(), 0, 1, 70);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a map\n"));
		return -1;
	}

	qse_htb_setscale (s1, QSE_HTB_KEY, QSE_SIZEOF(qse_char_t));
	qse_htb_setcopier (s1, QSE_HTB_KEY, QSE_HTB_COPIER_INLINE);
	qse_htb_setcopier (s1, QSE_HTB_VAL, QSE_HTB_COPIER_INLINE);
	qse_htb_setcomper (s1, comp_key);

	for (i = 0; i < QSE_COUNTOF(keys); i++)
	{
		qse_printf (QSE_T("inserting a key [%s] and a value %d: "), keys[i], i);
		if (qse_htb_insert (s1, keys[i], qse_strlen(keys[i]), &i, sizeof(i)) == QSE_NULL)
		{
			qse_printf (QSE_T("[FAILED]\n"));
		}
		else
		{	
			qse_printf (QSE_T("[OK]\n"));
		}
	}
		
	qse_htb_walk (s1, print_map_pair_3, QSE_NULL);

	qse_htb_close (s1);
	return 0;
}

static int test4 ()
{
	qse_htb_t* s1;
	int i;
	qse_char_t* keys[] = 
	{
		QSE_T("one"),
		QSE_T("two"),
		QSE_T("three"),
		QSE_T("four"),
		QSE_T("ONE"),
		QSE_T("Two"),
		QSE_T("thRee")
	};

	s1 = qse_htb_open (QSE_MMGR_GETDFL(), 0, 1, 70);
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a map\n"));
		return -1;
	}

	qse_htb_setscale (s1, QSE_HTB_KEY, QSE_SIZEOF(qse_char_t));
	qse_htb_setcopier (s1, QSE_HTB_KEY, QSE_HTB_COPIER_INLINE);
	qse_htb_setcopier (s1, QSE_HTB_VAL, QSE_HTB_COPIER_INLINE);
	qse_htb_setcomper (s1, comp_key);

	for (i = 0; i < QSE_COUNTOF(keys); i++)
	{
		qse_printf (QSE_T("upserting a key [%s] and a value %d: "), keys[i], i);
		if (qse_htb_upsert (s1, keys[i], qse_strlen(keys[i]), &i, sizeof(i)) == QSE_NULL)
		{
			qse_printf (QSE_T("[FAILED]\n"));
		}
		else
		{	
			qse_printf (QSE_T("[OK]\n"));
		}
	}
		
	qse_htb_walk (s1, print_map_pair_3, QSE_NULL);

	qse_htb_close (s1);
	return 0;
}
#endif

int main ()
{
	R (test1);
	R (test2);
#if 0
	R (test3);
	R (test4);
#endif
	return 0;
}
