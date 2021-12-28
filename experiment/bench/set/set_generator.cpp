//
// Created by LintianShi on 2021/11/11.
//

#include "set_generator.h"
#include <math.h>
#include <string.h>

#define PA (pattern.PR_ADD)
#define PR (pattern.PR_ADD + pattern.PR_REM)
#define PC (pattern.PR_ADD + pattern.PR_REM + pattern.PR_CONTAINS)

set_generator::set_op_gen_pattern& set_generator::get_pattern(const string& name)
{
    static map<string, set_op_gen_pattern> patterns{{"default",
                                                     {.PR_ADD = 0.40,
                                                      .PR_REM = 0.20,
                                                      .PR_CONTAINS = 0.20,
                                                      .PR_SIZE = 0.20}},
                                                    };
    if (patterns.find(name) == patterns.end()) return patterns["default"];
    return patterns[name];
}

struct invocation* set_generator::exec_op(redis_client &c, cmd* op) 
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
            return NULL;
        }
        invocation* inv = new invocation;
        string operation = "add," + to_string(((set_add_cmd*)op) -> element) + ",null";
        inv->operation = operation;
        write_op_executed++;
        return inv;
    }
    else if (op->op_name == REM)
    {
        if (reply == NULL || reply->type == 6) {
            return NULL;
        }
        invocation* inv = new invocation;
        string operation = "remove," + to_string(((set_rem_cmd*)op) -> element) + ",null";
        inv->operation = operation;
        write_op_executed++;
        return inv;
    }
    else if (op->op_name == CONTAINS)
    {
        if (reply == NULL || reply->type == 6) {
            return NULL;
        }
        
        invocation* inv = new invocation;
        string operation = "contains," + to_string(((set_contains_cmd*)op) -> element) + "," + (reply->integer == 1 ? "true" : "false");
        inv->operation = operation;
        write_op_executed++;
        return inv;  
    } else if ((op->op_name == SIZE)) {
        if (reply == NULL || reply->type == 6) {
            return NULL;
        }
        
        invocation* inv = new invocation;
        string operation = "size," + to_string(reply->integer);
        inv->operation = operation;
        write_op_executed++;
        return inv;  
    }
    return NULL;
}

cmd* set_generator::generate_op()
{
    int e;
    double rand = decide();
    if (rand <= PA) { 
        return generate_add();
    }
    else if (rand <= PR)
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
        return new set_rem_cmd(zt, e, round);
    }
    else if (rand <= PC)
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
        return new set_contains_cmd(zt, e, round);
    } else {
        return new set_size_cmd(zt, round);
    }
}

cmd* set_generator::generate_add()
{
    int e;
    e = intRand(MAX_ELE);
    unordered_set<int>::iterator it = elements.find(e);
    if (it != elements.end()) {
        e = intRand(MAX_ELE);
    }
    elements.insert(e);
    return new set_add_cmd(zt, e, round);
}

cmd* set_generator::generate_dummy()
{
    return new set_cmd(zt, DUMMY, round);
}