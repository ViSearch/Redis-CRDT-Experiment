#ifndef BENCH_LIST_CMD_H
#define BENCH_LIST_CMD_H

#define INSERT 1
#define UPDATE 2
#define REMOVE 3
#define READ 4
#define DUMMY 5

#include "../util.h"
#include "list_generator.h"

class list_cmd : public cmd
{
public:
    list_cmd(const string &type, int op, int round)
    {
        string method;
        if (op == INSERT) {
            method = "insert";
        } else if (op == UPDATE) {
            method = "update";
        } else if (op == REMOVE) {
            method = "rem";
        } else if (op == READ) {
            method = "list";
        } else {
            method = "dummy";
        }
        stream << type << "l" << method << " " << type << "list" <<round;
        this->op_name = op;
    }

    void handle_redis_return(const redisReply_ptr &r) override {}
};

class list_insert_cmd : public list_cmd
{
public:
    string prev, id, content;

    list_insert_cmd(const string &type, string &prev, string &id, string &content, int round)
        : list_cmd(type, INSERT, round),
          prev(prev),
          id(id),
          content(content)
    {
        add_args(prev, id, content, 1, 12, 0U, 0);
    }
};

class list_remove_cmd : public list_cmd
{
public:
    string id;

    list_remove_cmd(const string &type, string &id, int round)
        : list_cmd(type, REMOVE, round), id(id)
    {
        add_args(id);
    }
};

class list_read_cmd : public list_cmd
{
public:
    list_read_cmd(const string &type, int round) : list_cmd(type, READ, round) {}
};

#endif  // BENCH_LIST_CMD_H