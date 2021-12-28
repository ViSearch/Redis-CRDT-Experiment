//
// Created by LintianShi on 21-11-11.
//

#include "CRDT.h"
#include "RWFramework.h"
#include "server.h"

#ifdef CRDT_OVERHEAD
#define RWFSE_SIZE (sizeof(rwfse) + REH_SIZE_ADDITIONAL)
#endif

#define RWF_SET_TABLE_SUFFIX "_rwfsets_"

typedef struct RWF_SET_element
{
    reh header;
    int value;
} rwfse;

rwfse *rwfseNew()
{
    rwfse *e = zmalloc(sizeof(rwfse));
    REH_INIT(e);
    return e;
}

#define SCORE(e) ((e)->value)

#define GET_RWFSE(arg_t, create)                                                     \
    (rwfse *)rehHTGet(c->db, c->arg_t[1], RWF_SET_TABLE_SUFFIX, c->arg_t[2], create, \
                      (rehNew_func_t)rwfseNew)

// This doesn't free t.
static void removeFunc(client *c, rwfse *e, vc *t)
{
    if (removeCheck((reh *)e, t))
    {
        robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
        zsetDel(zset, c->rargv[2]->ptr);
        server.dirty++;
    }
}

void rwfsaddCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC(3);
            CHECK_CONTAINER_TYPE(OBJ_ZSET);
            CHECK_ARG_TYPE_INT(c->argv[2]);
            rwfse *e = GET_RWFSE(argv, 1);  //add an element in set, if not exist then create one
            PREPARE_PRECOND_ADD(e); //element cannot exist already
            ADD_CR_NON_RMV(e);  //add a vc for add command
        CRDT_EFFECT
            long long v = 0;
            getLongLongFromObject(c->rargv[2], &v);
            vc *t = CR_GET_LAST;    //get command's vc
            rwfse *e = GET_RWFSE(rargv, 1); //get element according to the key
            removeFunc(c, e, t);    //if this element is old, remove
            if (addCheck((reh *)e, t))  //element should be the one insert in PREPARE
            {
                e->value = v;
                robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);  //get set
                int flags = ZADD_NONE;
                zsetAdd(zset, SCORE(e), c->rargv[2]->ptr, &flags, NULL);    //add element to set
                server.dirty++;
            }
            vc_delete(t);
    CRDT_END
}

void rwfsremCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC(3);
            CHECK_CONTAINER_TYPE(OBJ_ZSET);
            rwfse *e = GET_RWFSE(argv, 0);
            PREPARE_PRECOND_NON_ADD(e);
            ADD_CR_RMV(e);
        CRDT_EFFECT
            rwfse *e = GET_RWFSE(rargv, 1);
            vc *t = CR_GET_LAST;
            removeFunc(c, e, t);
            vc_delete(t);
    CRDT_END
}

void rwfscontainsCommand(client *c)
{
    // CHECK_ARGC(3);
    // rwfse *e = GET_RWFSE(argv, 0);
    // if (e != NULL) {
    //     addReplyBool(c, 1);
    // } else {
    //     addReplyBool(c, 0);
    // }

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

void rwfssizeCommand(client *c)
{
    robj *zobj;
    if ((zobj = lookupKeyReadOrReply(c, c->argv[1], shared.emptyarray)) == NULL
        || checkType(c, zobj, OBJ_ZSET))
        return;
    unsigned long size = zsetLength(zobj);
    addReplyLongLong(c, size);
}

#ifdef CRDT_ELE_STATUS
void rwfsestatusCommand(client *c)
{
    rwfse *e = GET_RWFSE(argv, 0);
    if (e == NULL)
    {
        addReply(c, shared.emptyarray);
        return;
    }

    // unsigned long len = 6 + listLength(e->ops);
    // addReplyArrayLen(c, len);
    addReplyArrayLen(c, 5);

    addReplyBulkSds(c, sdscatprintf(sdsempty(), "add id:%d", PID(e)));
    addReplyBulkSds(c, sdsnew("current:"));
    addReplyBulkSds(c, vcToSds(CURRENT(e)));

    addReplyBulkSds(c, sdscatprintf(sdsempty(), "value:%d", e->value));
}
#endif

#ifdef CRDT_OPCOUNT
void rwfsopcountCommand(client *c) { addReplyLongLong(c, op_count_get()); }
#endif

/* Actually the hash set used here to store rwfze structures is not necessary.
 * We can store rwfze in the zset, for it's whether ziplist or dict+skiplist.
 * We use the hash set here for fast implementing our CRDT Algorithms.
 * We may optimize our implementation by not using the hash set and using
 * zset's own dict instead in the future.
 * As for metadata overhead calculation, we here do it as if we have done
 * such optimization. The commented area is the overhead if we take the
 * hash set into account.
 *
 * optimized:
 * zset:
 * key -> score(double)
 * --->
 * key -> pointer that point to metadata (rwfze*)
 *
 * the metadata contains score information
 * overall the metadata overhead is size used by rwfze
 * */
#ifdef CRDT_OVERHEAD

void rwfsoverheadCommand(client *c) { addReplyLongLong(c, ovhd_get()); }

#elif 0

void rwfzoverheadCommand(client *c)
{
    robj *htname = createObject(OBJ_STRING, sdscat(sdsdup(c->argv[1]->ptr), RWF_RPQ_TABLE_SUFFIX));
    robj *ht = lookupKeyRead(c->db, htname);
    long long size = 0;

    /*
     * The overhead for database to store the hash set information.
     * sds temp = sdsdup(htname->ptr);
     * size += sizeof(dictEntry) + sizeof(robj) + sdsAllocSize(temp);
     * sdsfree(temp);
     */

    decrRefCount(htname);
    if (ht == NULL)
    {
        addReplyLongLong(c, 0);
        return;
    }

    hashTypeIterator *hi = hashTypeInitIterator(ht);
    while (hashTypeNext(hi) != C_ERR)
    {
        sds value = hashTypeCurrentObjectNewSds(hi, OBJ_HASH_VALUE);
        rwfze *e = *(rwfze **)value;
        size += RWFZE_SIZE;
        sdsfree(value);
    }
    hashTypeReleaseIterator(hi);
    addReplyLongLong(c, size);
    /*
    if (ht->encoding == OBJ_ENCODING_ZIPLIST)
    {
        // Not implemented. We show the overhead calculation method:
        // size += (size of the ziplist structure itself) + (size of keys and values);
        // Iterate the ziplist to get each rwfze* e;
        // size += RWFZE_SIZE;
    }
    else if (ht->encoding == OBJ_ENCODING_HT)
    {
        dict *d = ht->ptr;
        size += sizeof(dict) + sizeof(dictType) + (d->ht[0].size + d->ht[1].size) * sizeof(dictEntry
    *)
                + (d->ht[0].used + d->ht[1].used) * sizeof(dictEntry);
        dictIterator *di = dictGetIterator(d);
        dictEntry *de;
        while ((de = dictNext(di)) != NULL)
        {
            sds key = dictGetKey(de);
            sds value = dictGetVal(de);
            size += sdsAllocSize(key) + sdsAllocSize(value);
            rwfze *e = *(rwfze **) value;
            size += RWFZE_SIZE;
        }
        dictReleaseIterator(di);
    }
    else
    {
        serverPanic("Unknown hash encoding");
    }
    */
}

#endif