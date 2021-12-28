//
// Created by LintianShi on 2021-12-24.
//

#include "CRDT.h"
#include "RWFramework.h"
#include "server.h"

#define RW_SET_TABLE_SUFFIX "_rsets_"
#define LOOKUP(e) (listLength((e)->aset) != 0 && listLength((e)->rset) == 0)
#define SCORE(e) ((e)->exist->value)

typedef struct rset_aset_element
{
    vc *t;
    int value;
} rs_ase;

typedef struct RW_SET_element
{
    vc *current;
    rs_ase *exist;
    list *aset;
    list *rset;
    list *ops;
} rse;

rse *rseNew()
{
    rse *e = zmalloc(sizeof(rse));
    e->current = vc_new();
    e->exist = NULL;
    e->aset = listCreate();
    e->rset = listCreate();
    e->ops = listCreate();
    return e;
}

#define GET_RSE(arg_t, create)                                                    \
    (rse *)rehHTGet(c->db, c->arg_t[1], RW_SET_TABLE_SUFFIX, c->arg_t[2], create, \
                    (rehNew_func_t)rseNew)

enum rs_cmd_type
{
    RSADD,
    RSREM
};
typedef struct rsset_unready_command
{
    enum rs_cmd_type type;
    int value;
    vc *t;
} rs_cmd;


// get the ownership of t
rs_cmd *rs_cmdNew(enum rs_cmd_type type, int value, vc *t)
{
    rs_cmd *cmd = zmalloc(sizeof(rs_cmd));
    cmd->type = type;
    cmd->value = value;
    cmd->t = t;
    return cmd;
}

void rs_cmdDelete(rs_cmd *cmd)
{
    vc_delete(cmd->t);
    zfree(cmd);
}

// doesn't free t, doesn't own t
static void r_addFunc(rse *e, redisDb *db, robj *tname, robj *key, int value, vc *t)
{
    rs_ase *a = zmalloc(sizeof(rs_ase));
    a->t = vc_dup(t);
    a->value = value;
    listAddNodeTail(e->aset, a);

    listNode *ln;
    listIter li;
    int ase_removed = 0;

    listRewind(e->aset, &li);
    while ((ln = listNext(&li)))
    {
        rs_ase *ae = ln->value;
        if (vc_cmp(ae->t, t) == VC_LESS)
        {
            listDelNode(e->aset, ln);
            if (e->exist == ae) e->exist = NULL;
            vc_delete(ae->t);
            zfree(ae);
            ase_removed = 1;
        }
    }

    listRewind(e->rset, &li);
    while ((ln = listNext(&li)))
    {
        vc *r = ln->value;
        if (vc_cmp(r, t) == VC_LESS)
        {
            listDelNode(e->rset, ln);
            vc_delete(r);
        }
    }

    if(ase_removed && e->exist == NULL)
    {
        listRewind(e->aset, &li);
        while ((ln = listNext(&li)))
        {
            rs_ase *ae = ln->value;
            if (e->exist == NULL || e->exist->t->id < ae->t->id) e->exist = ae;
        }
    }
    if (e->exist == NULL || e->exist->t->id < t->id) e->exist = a;

    if (LOOKUP(e))
    {
        robj *zset = getZsetOrCreate(db, tname, key);
        int flags = ZADD_NONE;
        zsetAdd(zset, SCORE(e), key->ptr, &flags, NULL);
    }
    vc_update(e->current, t);
    server.dirty++;
}

static void r_removeFunc(rse *e, redisDb *db, robj *tname, robj *key, vc *t)
{
    vc *r = vc_dup(t);
    listAddNodeTail(e->rset, r);

    listNode *ln;
    listIter li;
    listRewind(e->aset, &li);
    while ((ln = listNext(&li)))
    {
        rs_ase *a = ln->value;
        if (vc_cmp(a->t, t) == VC_LESS)
        {
            listDelNode(e->aset, ln);
            if (e->exist == a) e->exist = NULL;
            vc_delete(a->t);
            zfree(a);
        }
    }

    if (e->exist == NULL)
    {
        listRewind(e->aset, &li);
        while ((ln = listNext(&li)))
        {
            rs_ase *a = ln->value;
            if (e->exist == NULL || e->exist->t->id < a->t->id) e->exist = a;
        }
    }

    if (!LOOKUP(e))
    {
        robj *zset = getZsetOrCreate(db, tname, key);
        zsetDel(zset, key->ptr);
    }
    vc_update(e->current, t);
    server.dirty++;
}

static void r_notifyLoop(rse *e, redisDb *db, robj *tname, robj *key)
{
    int changed;
    do
    {
        changed = 0;
        listNode *ln;
        listIter li;
        listRewind(e->ops, &li);
        while ((ln = listNext(&li)))
        {
            rs_cmd *cmd = ln->value;
            if (vc_causally_ready(e->current, cmd->t))
            {
                changed = 1;
                switch (cmd->type)
                {
                    case RSADD:
                        r_addFunc(e, db, tname, key, cmd->value, cmd->t);
                        break;
                    case RSREM:
                        r_removeFunc(e, db, tname, key, cmd->t);
                        break;
                    default:
                        serverPanic("unknown rzset cmd type.");
                }
                listDelNode(e->ops, ln);
                rs_cmdDelete(cmd);
                break;
            }
        }
    } while (changed);
}


void rsaddCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC(3);
            CHECK_CONTAINER_TYPE(OBJ_ZSET);
            rse *e = GET_RSE(argv, 1);
            if (LOOKUP(e))
            {
                addReply(c, shared.ele_exist);
                return;
            }
            RARGV_ADD_SDS(vc_now(e->current));
        CRDT_EFFECT
            long long v;
            getLongLongFromObject(c->rargv[2], &v);
            vc *t = sdsToVC(c->rargv[4]->ptr);
            rse *e = GET_RSE(rargv, 1);
            if (vc_causally_ready(e->current, t))
            {
                r_addFunc(e, c->db, c->rargv[1], c->rargv[2], v, t);
                vc_delete(t);
                r_notifyLoop(e, c->db, c->rargv[1], c->rargv[2]);
            }
            else
            {
                rs_cmd *cmd = rs_cmdNew(RSADD, v, t);
                listAddNodeTail(e->ops, cmd);
            }
    CRDT_END
}

void rsremCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC(3);
            CHECK_CONTAINER_TYPE(OBJ_ZSET);
            rse *e = GET_RSE(argv, 0);
            if (e == NULL || !LOOKUP(e))
            {
                addReply(c, shared.ele_nexist);
                return;
            }
            RARGV_ADD_SDS(vc_now(e->current));
        CRDT_EFFECT
            vc *t = sdsToVC(c->rargv[3]->ptr);
            rse *e = GET_RSE(rargv, 1);
            if (vc_causally_ready(e->current, t))
            {
                r_removeFunc(e, c->db, c->rargv[1], c->rargv[2], t);
                vc_delete(t);
                r_notifyLoop(e, c->db, c->rargv[1], c->rargv[2]);
            }
            else
            {
                rs_cmd *cmd = rs_cmdNew(RSREM, 0, t);
                listAddNodeTail(e->ops, cmd);
            }
    CRDT_END
}

void rscontainsCommand(client *c)
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

void rssizeCommand(client *c)
{
    robj *zobj;
    if ((zobj = lookupKeyReadOrReply(c, c->argv[1], shared.emptyarray)) == NULL
        || checkType(c, zobj, OBJ_ZSET))
        return;
    unsigned long size = zsetLength(zobj);
    addReplyLongLong(c, size);
}