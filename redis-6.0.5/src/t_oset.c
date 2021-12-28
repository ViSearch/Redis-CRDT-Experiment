//
// Created by LintianShi on 2021-12-24.
//

#include "CRDT.h"
#include "RWFramework.h"
#include "lamport_clock.h"
#include "server.h"

#ifdef CRDT_OVERHEAD

#define OSE_SIZE (sizeof(ose) + 2 * sizeof(list))
#define OSE_ASE_SIZE (sizeof(os_ase) + sizeof(lc) + sizeof(listNode))
#define OSE_RSE_SIZE (sizeof(lc) + sizeof(listNode))

#endif

#define ORI_SET_TABLE_SUFFIX "_osets_"
#define LOOKUP(e) (listLength((e)->aset) != 0)
#define SCORE(e) ((e)->exist->value)

typedef struct oset_aset_element
{
    lc *t;
    int value;
} os_ase;

typedef struct ORI_SET_element
{
    int current;
    os_ase * exist;
    list *aset;
    list *rset;
} ose;


ose *oseNew()
{
    ose *e = zmalloc(sizeof(ose));
    e->current = 0;
    e->exist = NULL;
    e->aset = listCreate();
    e->rset = listCreate();
    return e;
}

os_ase *o_asetGet(ose *e, lc *t, int delete)
{
    listNode *ln;
    listIter li;
    listRewind(e->aset, &li);
    while ((ln = listNext(&li)))
    {
        os_ase *a = ln->value;
        if (lc_cmp_as_tag(t, a->t) == 0)
        {
            if (delete)
            {
                listDelNode(e->aset, ln);
            }
            return a;
        }
    }
    return NULL;
}

lc *o_rsetGet(ose *e, lc *t, int delete)
{
    listNode *ln;
    listIter li;
    listRewind(e->rset, &li);
    while ((ln = listNext(&li)))
    {
        lc *a = ln->value;
        if (lc_cmp_as_tag(t, a) == 0)
        {
            if (delete)
            {
                listDelNode(e->rset, ln);
            }
            return a;
        }
    }
    return NULL;
}

// 下面函数对自己参数 t 没有所有权
os_ase *o_add_ase(ose *e, lc *t)
{
    os_ase *a = zmalloc(sizeof(os_ase));
    a->t = lc_dup(t);
    listAddNodeTail(e->aset, a);
    return a;
}

int update_exist(ose *e, lc *t, int v)
{
    if (o_rsetGet(e, t, 0) != NULL) return 0;
    os_ase *a = o_asetGet(e, t, 0);
    if (a == NULL) a = o_add_ase(e, t);
    a->value = v;
    if (e->exist == NULL || lc_cmp_as_tag(e->exist->t, a->t) < 0)
    {
        e->exist = a;
        return 1;
    }
    return 0;
}

// 没有整理
void set_remove_tag(ose *e, lc *t)
{
    if (o_rsetGet(e, t, 0) != NULL) return;
    lc *nt = lc_dup(t);
    listAddNodeTail(e->rset, nt);

    os_ase *a;
    if ((a = o_asetGet(e, t, 1)) != NULL)
    {
        if (e->exist == a) e->exist = NULL;
        zfree(a->t);
        zfree(a);
    }
}

void set_resort(ose *e)
{
    if (e->exist != NULL) return;

    listNode *ln;
    listIter li;
    listRewind(e->aset, &li);

    while ((ln = listNext(&li)))
    {
        os_ase *a = ln->value;
        if (e->exist == NULL || lc_cmp_as_tag(e->exist->t, a->t) < 0) e->exist = a;
    }
}

#define GET_OSE(arg_t, create)                                                     \
    (ose *)rehHTGet(c->db, c->arg_t[1], ORI_SET_TABLE_SUFFIX, c->arg_t[2], create, \
                    (rehNew_func_t)oseNew)

void osaddCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC(3);
            CHECK_CONTAINER_TYPE(OBJ_ZSET);
            ose *e = GET_OSE(argv, 1);
            if (LOOKUP(e))
            {
                addReply(c, shared.ele_exist);
                return;
            }
            lc *t = lc_new(e->current);
            e->current++;
            RARGV_ADD_SDS(lcToSds(t));
            lc_delete(t);
        CRDT_EFFECT
            long long v;
            getLongLongFromObject(c->rargv[2], &v);
            lc *t = sdsToLc(c->rargv[3]->ptr);
            ose *e = GET_OSE(rargv, 1);
            if (update_exist(e, t, v))
            {
                robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
                int flags = ZADD_NONE;
                zsetAdd(zset, SCORE(e), c->rargv[2]->ptr, &flags, NULL);
            }
            lc_delete(t);
            server.dirty++;
    CRDT_END
}


void osremCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC(3);
            CHECK_CONTAINER_TYPE(OBJ_ZSET);
            ose *e = GET_OSE(argv, 0);
            if (e == NULL || !LOOKUP(e))
            {
                addReply(c, shared.ele_nexist);
                return;
            }
            listNode *ln;
            listIter li;
            listRewind(e->aset, &li);
            while ((ln = listNext(&li)))
            {
                os_ase *a = ln->value;
                RARGV_ADD_SDS(lcToSds(a->t));
            }
        CRDT_EFFECT
            ose *e = GET_OSE(rargv, 1);
            for (int i = 3; i < c->rargc; i++)
            {
                lc *t = sdsToLc(c->rargv[i]->ptr);
                set_remove_tag(e, t);
                lc_delete(t);
            }
            if (e->exist == NULL)
            {
                set_resort(e);
                robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
                if (LOOKUP(e))
                {
                    int flags = ZADD_NONE;
                    zsetAdd(zset, SCORE(e), c->rargv[2]->ptr, &flags, NULL);
                }
                else
                {
                    zsetDel(zset, c->rargv[2]->ptr);
                }
            }
            server.dirty++;
    CRDT_END
}

void oscontainsCommand(client *c)
{
    robj *key = c->argv[1];
    robj *zobj;
    double score;

    if ((zobj = lookupKeyReadOrReply(c, key, shared.null[c->resp])) == NULL
        || checkType(c, zobj, OBJ_ZSET))
        return;

    if (zsetScore(zobj, c->argv[2]->ptr, &score) == C_ERR) { addReplyBool(c, 0); }
    else
    {
        addReplyBool(c, 1);
    }
}

void ossizeCommand(client *c)
{
    robj *zobj;
    if ((zobj = lookupKeyReadOrReply(c, c->argv[1], shared.emptyarray)) == NULL
        || checkType(c, zobj, OBJ_ZSET))
        return;
    unsigned long size = zsetLength(zobj);
    addReplyLongLong(c, size);
}