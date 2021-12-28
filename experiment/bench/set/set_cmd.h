//
// Created by LintianShi on 2021/11/11.
//

#ifndef BENCH_SET_CMD_H
#define BENCH_SET_CMD_H

#define ADD 1
#define REM 2
#define CONTAINS 3
#define SIZE 4
#define DUMMY 5

#include "../util.h"
#include <string.h>

class set_cmd : public cmd
{
public:
    set_cmd(const string &type, int op, int round)
    {
        string method;
        if (op == ADD) {
            method = "add";
        } else if (op == REM) {
            method = "rem";
        } else if (op == CONTAINS) {
            method = "contains";
        } else if (op == SIZE) {
            method = "size";
        } else {
            method = "dummy";
        }
        stream << type << "s" << method << " " << type << "set" << round;
        this->op_name = op;
    }

    void handle_redis_return(const redisReply_ptr &r) override { ; }
};

class set_add_cmd : public set_cmd
{
public:
    int element;
    set_add_cmd(const string &type, int element, int round)
        : set_cmd(type, ADD, round), element(element)
    {
        add_args(element);
    }
};

class set_rem_cmd : public set_cmd
{
public:
    int element;

    set_rem_cmd(const string &type, int element, int round)
        : set_cmd(type, REM, round), element(element)
    {
        add_args(element);
    }
};

class set_contains_cmd : public set_cmd
{    
public:
    int element;
    set_contains_cmd(const string &type, int element, int round)
         : set_cmd(type, CONTAINS, round), element(element) 
         {
            add_args(element);
         }
};

class set_size_cmd : public set_cmd
{    
public:
    set_size_cmd(const string &type, int round)
         : set_cmd(type, SIZE, round) {}
};

#endif  // BENCH_SET_CMD_H
