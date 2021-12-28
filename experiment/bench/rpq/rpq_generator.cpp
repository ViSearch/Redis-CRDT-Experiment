//
// Created by admin on 2020/1/8.
//

#include "rpq_generator.h"
#include <math.h>
#include <string.h>

#define PA (pattern.PR_ADD)
#define PI (pattern.PR_ADD + pattern.PR_INC)
#define PM (pattern.PR_ADD + pattern.PR_INC + pattern.PR_MAX)
#define PS (pattern.PR_ADD + pattern.PR_INC + pattern.PR_MAX + pattern.PR_SCORE)
#define PAA (pattern.PR_ADD_CA)
#define PAR (pattern.PR_ADD_CA + pattern.PR_ADD_CR)
#define PRA (pattern.PR_REM_CA)
#define PRR (pattern.PR_REM_CA + pattern.PR_REM_CR)

rpq_generator::rpq_op_gen_pattern& rpq_generator::get_pattern(const string& name)
{
    static map<string, rpq_op_gen_pattern> patterns{{"default",
                                                     {.PR_ADD = 0.35,
                                                      .PR_INC = 0.20,
                                                      .PR_MAX = 0.10,
                                                      .PR_SCORE = 0.10,
                                                      .PR_REM = 0.25,
                                                      
                                                      .PR_ADD_CA = 0.15,
                                                      .PR_ADD_CR = 0.05,
                                                      .PR_REM_CA = 0.1,
                                                      .PR_REM_CR = 0.1}},

                                                    {"ardominant",
                                                     {.PR_ADD = 0.11,
                                                      .PR_INC = 0.6,
                                                      .PR_MAX = 0.1,
                                                      .PR_SCORE = 0.1,
                                                      .PR_REM = 0.09,

                                                      .PR_ADD_CA = 0.15,
                                                      .PR_ADD_CR = 0.05,
                                                      .PR_REM_CA = 0.1,
                                                      .PR_REM_CR = 0.1}}};
    if (patterns.find(name) == patterns.end()) return patterns["default"];
    return patterns[name];
}

struct invocation* rpq_generator::exec_op(redis_client &c, cmd* op) 
{
    if (op == NULL) {
        return NULL;
    }
    if (op->op_name == DUMMY) {
        return NULL;
    }
    
    redisReply_ptr reply = c.exec(op);
    if (op->op_name == ADD) {
        if (reply == NULL || reply->type == 6) {
            // printf("op: %d\n", op->op_name);
            return NULL;
        }
        invocation* inv = new invocation;
        string operation = "add," + to_string(((rpq_add_cmd*)op) -> element) + "," + to_string(((rpq_add_cmd*)op) -> value) + ",null";
        inv->operation = operation;
        write_op_executed++;
        return inv;
    }
    else if (op->op_name == INCRBY)
    {
        if (reply == NULL || reply->type == 6) {
            // printf("op: %d\n", op->op_name);
            return NULL;
        }
        invocation* inv = new invocation;
        string operation = "incrby," + to_string(((rpq_incrby_cmd*)op) -> element) + "," + to_string(((rpq_incrby_cmd*)op) -> value) + ",null";
        inv->operation = operation;
        write_op_executed++;
        return inv;
    }
    else if (op->op_name == REM)
    {
        if (reply == NULL || reply->type == 6) {
            // printf("op: %d\n", op->op_name);
            return NULL;
        }
        invocation* inv = new invocation;
        string operation = "rem," + to_string(((rpq_rem_cmd*)op) -> element) + ",null";
        inv->operation = operation;
        write_op_executed++;
        return inv;
    }
    else if (op->op_name == SCORE)
    {
        if (reply == NULL || reply->type == 6) {
            // printf("op: %d\n", op->op_name);
            return NULL;
        }
        invocation* inv = new invocation;
        if (reply->type == 1)
        {
            string operation = "score," + to_string(((rpq_score_cmd*)op) -> element) + "," + reply->str;
            inv->operation = operation;
        }
        else 
        {
            string operation = "score," + to_string(((rpq_score_cmd*)op) -> element) + "," + "null";
            inv->operation = operation;
        }
        write_op_executed++;
        return inv;
    }
    else if (op->op_name == MAX)
    {
        if (reply == NULL || reply->type == 6) {
            // printf("op: %d\n", op->op_name);
            return NULL;
        }
        invocation* inv = new invocation;
        if (reply->elements == 2) {
            string ret1 = reply->element[0]->str;
            string ret2 = reply->element[1]->str;
            string operation = "max," + ret1 + " " + ret2;
            inv->operation = operation;
        } else {
            string ret = "null";
            string operation = "max," + ret;
            inv->operation = operation;
        }
        write_op_executed++;
        return inv;
    }
    return NULL;
}

cmd* rpq_generator::generate_op()
{
    int e;
    double rand = decide();
    if (rand <= PA) { 
        return generate_add();
    }
    else if (rand <= PI)
    {
        if (elements.size() == 0) 
        {
            return generate_add();
        }
        e = intRand(MAX_ELE);
        unordered_set<int>::iterator it = elements.find(e);
        if (it == elements.end()) {
            e = intRand(MAX_ELE);
        }
        int d = intRand(-MAX_INCR, MAX_INCR);
        return new rpq_incrby_cmd(zt, e, d, round);
    }
    else if (rand <= PM)
    {
        if (elements.size() == 0) 
        {
            return generate_add();
        }
        return new rpq_max_cmd(zt, round);
    } 
    else if (rand <= PS)
    {
        if (elements.size() == 0) 
        {
            return generate_add();
        }
        e = intRand(MAX_ELE);
        unordered_set<int>::iterator it = elements.find(e);
        if (it == elements.end()) {
            e = intRand(MAX_ELE);
        }
        return new rpq_score_cmd(zt, e, round);
    }
    else
    {
        if (elements.size() == 0) 
        {
            return generate_add();
        }
        e = intRand(MAX_ELE);
        unordered_set<int>::iterator it = elements.find(e);
        if (it == elements.end()) {
            e = intRand(MAX_ELE);
        }
        return new rpq_rem_cmd(zt, e, round);
    }
}

cmd* rpq_generator::generate_add()
{
    int e;
    e = intRand(MAX_ELE);
    unordered_set<int>::iterator it = elements.find(e);
    if (it != elements.end()) {
        e = intRand(MAX_ELE);
    }
    elements.insert(e);
    int d = intRand(0, MAX_INIT);
    return new rpq_add_cmd(zt, e, d, round);
}

cmd* rpq_generator::generate_dummy()
{
    return new rpq_cmd(zt, DUMMY, round);
}