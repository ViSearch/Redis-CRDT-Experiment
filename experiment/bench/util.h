//
// Created by admin on 2020/1/6.
//

#ifndef BENCH_UTIL_H
#define BENCH_UTIL_H

#include <sys/stat.h>

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "exp_setting.h"

#if defined(__linux__)
#include <hiredis/hiredis.h>
#elif defined(_WIN32)

#include <direct.h>

#include "../../redis-6.0.5/deps/hiredis/hiredis.h"

#endif

using namespace std;

int intRand(int min, int max);

static inline int intRand(int max) { return intRand(0, max - 1); }

static inline bool boolRand() { return intRand(0, 1); }

string strRand(int max_len = 3);

double doubleRand(double min, double max);

static inline double decide() { return doubleRand(0.0, 1.0); }

using redisReply_ptr = unique_ptr<redisReply, decltype(freeReplyObject) *>;

class cmd;

class redis_client
{
private:
    const char *ip;
    const int port;
    redisContext *c = nullptr;

    mutex m;
    condition_variable cv;
    list<cmd *> cmds;
    volatile bool run = false;
    thread pipeline;

    void connect()
    {
        if (c != nullptr) redisFree(c);
        c = redisConnect(ip, port);
        if (c == nullptr || c->err)
        {
            if (c)
            {
                cout << "\nError for redisConnect: " << c->errstr << ", ip:" << ip
                     << ", port:" << port << endl;
                redisFree(c);
                c = nullptr;
            }
            else
                cout << "\nCan't allocate redis context" << endl;
            exit(-1);
        }
    }

    void reply_error(const string &cmd);

    redisReply_ptr exec();

public:
    redis_client(const char *ip, int port) : ip(ip), port(port) { connect(); }

    void add_pipeline_cmd(cmd *command);

    redisReply_ptr exec(const string &cmd)
    {
        //cout<<"exec...\n";
        if (run)
        {
            cout << "\nYou cannot use pipeline cmd and exec cmd at the same redis_client." << endl;
            exit(-1);
        }
        bool retryed = false;
        auto r = static_cast<redisReply *>(redisCommand(c, cmd.c_str()));
        while (r == nullptr)
        {
            if (!retryed)
            {
                connect();
                retryed = true;
                r = static_cast<redisReply *>(redisCommand(c, cmd.c_str()));
                continue;
            }
            reply_error(cmd);
        }
        //cout<<"...exec\n";
        return redisReply_ptr(r, freeReplyObject);
    }

    redisReply_ptr exec(const cmd &cmd);

    redisReply_ptr exec(cmd* cm);

    ~redis_client()
    {
        if (run)
        {
            run = false;
            cv.notify_all();
            if (pipeline.joinable()) pipeline.join();
        }
        if (c != nullptr) redisFree(c);
    }
};

class cmd
{
protected:
    ostringstream stream;

    cmd() = default;

    template <typename T>
    inline void add_args(const T &arg)
    {
        stream << " " << arg;
    }

    template <typename T, typename... Types>
    inline void add_args(const T &arg, const Types &... args)
    {
        stream << " " << arg;
        add_args(args...);
    }

    friend class redis_client;

public:
    int op_name;

    virtual void handle_redis_return(const redisReply_ptr &r) = 0;

    void exec(redis_client &c)
    {
        auto r = c.exec(stream.str());
        if (r->type != REDIS_REPLY_ERROR) handle_redis_return(r);
    }
};

class generator
{
protected:
    int round;
    mutex m;
public:
    atomic<int> write_op_executed{0};
    atomic<int> workload_pointer{0};
    vector<cmd*> workload;
    virtual struct invocation* exec_op(redis_client &c, cmd* op) = 0;
    virtual cmd* generate_op() = 0;
    virtual cmd* generate_dummy() = 0;

    void init()
    {
        for (int i = 0; i < 200; i++) {
            workload.emplace_back(generate_dummy());
        }
        int i = 0;
        while (i < exp_setting::total_ops * 5) {
            cmd* c = generate_op();
            if (c != NULL) {
                workload.emplace_back(c);
                i++;
            }     
        }
    }

    cmd* get_op()
    {
        if (workload_pointer >= workload.size() - 2) {
            write_op_executed++;
            return NULL;
        }
        return workload[workload_pointer++];
    }

    int get_round() {
        return round;
    }

    generator(int r) : round(r) {;}

    generator(const generator&) = delete;

    generator &operator=(const generator&) = delete;

    ~generator() {
        for (cmd* &c : workload) {
            if (c != NULL) {
                delete c;
            } 
        }
    }
};

class rdt_exp
{
private:
    const char *rdt_type;
    exp_setting::default_setting &rdt_exp_setting;

    class exp_setter
    {
    public:
        exp_setter(rdt_exp &r, const string &type)
        {
            exp_setting::set_exp_subject(type.c_str(), r.rdt_type);
            exp_setting::set_default(&r.rdt_exp_setting);
        }

        ~exp_setter()
        {
            exp_setting::set_default(nullptr);
            exp_setting::set_exp_subject(nullptr, nullptr);
        }
    };

protected:
    vector<string> rdt_types;
    vector<string> rdt_patterns;

    void add_type(const char *type) { rdt_types.emplace_back(type); }

    void add_pattern(const char *pattern) { rdt_patterns.emplace_back(pattern); }

    explicit rdt_exp(exp_setting::default_setting &rdt_st, const char *rdt_type)
        : rdt_exp_setting(rdt_st), rdt_type(rdt_type)
    {}

    virtual void exp_impl(const string &type, const string &pattern, int round) = 0;

    inline void exp_impl(const string &type) { exp_impl(type, "default", 100); }

public:
    void test_patterns(int round)
    {
        for (auto &p : rdt_patterns)
            for (auto &t : rdt_types)
                pattern_fix(p, t, round);
    }

    void test_default_settings(int round)
    {
        for (auto &t : rdt_types)
            pattern_fix("default", t, round);
    }

    void pattern_fix(const string &pattern, const string &type, int round)
    {
        exp_setter s(*this, type);
        exp_setting::set_pattern(pattern);
        exp_impl(type, pattern, round);
    }
};

struct invocation
{
    long start_time;
    long end_time;
    string operation;
};


class thread_trace
{ 
private:
    vector<invocation*> log;
public:
    thread_trace(){}

    void insert(invocation* inv) {
        if (inv != NULL)
            log.emplace_back(inv);
        // else 
        //     printf("%s\n", "fail");
    }

    int size() {
        return log.size();
    }

    string toString();

    thread_trace(const thread_trace&) = delete;

    thread_trace &operator=(const thread_trace&) = delete;

    ~thread_trace() {
        for (invocation* i : log) {
            if (i != NULL)
                delete i;
        }
    }
};

class execution_trace {
private:
    mutex m;
    vector<thread_trace*> traces;
public:
    void outputTrace();
    void add(thread_trace* trace) {
        {
            m.lock();
            traces.push_back(trace);
            m.unlock();
        } 
    }
    ~execution_trace() {
        for (auto &t : traces) {
            if (t != NULL)
                delete t;
        }
    }
};

#endif  // BENCH_UTIL_H
