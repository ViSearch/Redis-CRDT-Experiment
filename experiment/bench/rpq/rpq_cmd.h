//
// Created by admin on 2020/1/8.
//

#ifndef BENCH_RPQ_CMD_H
#define BENCH_RPQ_CMD_H

#define ADD 1
#define INCRBY 2
#define REM 3
#define MAX 4
#define SCORE 5
#define DUMMY 6

#include "../util.h"
#include <string.h>

class rpq_cmd : public cmd
{
public:
    rpq_cmd(const string &type, int op, int round)
    {
        string method;
        if (op == ADD) {
            method = "add";
        } else if (op == INCRBY) {
            method = "incrby";
        } else if (op == REM) {
            method = "rem";
        } else if (op == MAX) {
            method = "max";
        } else if (op == SCORE) {
            method = "score";
        } else {
            method = "dummy";
        }
        stream << type << "z" << method << " " << type << "rpq" << round;
        this->op_name = op;
    }

    void handle_redis_return(const redisReply_ptr &r) override { ; }
};

class rpq_add_cmd : public rpq_cmd
{
public:
    int element;
    int value;
    rpq_add_cmd(const string &type, int element, int value, int round)
        : rpq_cmd(type, ADD, round), element(element), value(value)
    {
        add_args(element, value);
    }
};

class rpq_incrby_cmd : public rpq_cmd
{
public:
    int element;
    int value;

    rpq_incrby_cmd(const string &type, int element, int value, int round)
        : rpq_cmd(type, INCRBY, round), element(element), value(value)
    {
        add_args(element, value);
    }
};

class rpq_rem_cmd : public rpq_cmd
{
public:
    int element;

    rpq_rem_cmd(const string &type, int element, int round)
        : rpq_cmd(type, REM, round), element(element)
    {
        add_args(element);
    }
};

class rpq_max_cmd : public rpq_cmd
{
public:
    rpq_max_cmd(const string &type, int round) : rpq_cmd(type, MAX, round) {}
};

class rpq_score_cmd : public rpq_cmd
{    
public:
    int element;
    rpq_score_cmd(const string &type, int element, int round)
         : rpq_cmd(type, SCORE, round), element(element) 
         {
            add_args(element);
         }
};

#endif  // BENCH_RPQ_CMD_H
