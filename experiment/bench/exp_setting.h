//
// Created by admin on 2020/4/15.
//

#ifndef BENCH_EXP_SETTING_H
#define BENCH_EXP_SETTING_H

#include <cassert>
#include <iostream>

#define TOTAL_SERVERS exp_setting::total_servers

using namespace std;

class exp_setting
{
public:
    struct default_setting
    {
        string name;
        int total_sec;
        int total_servers;
        int op_per_sec;
    };
    static const char *alg_type;
    static const char *rdt_type;
    static default_setting *default_p;

private:
    static inline void apply_default()
    {
        assert(default_p != nullptr);
        total_servers = default_p->total_servers;
        op_per_sec = default_p->op_per_sec;
        name = default_p->name;
    }

public:
    static int total_servers;
    static int total_ops;
    static int op_per_sec;
    static string name;
    static bool compare;

#define EXP_TYPE_CODEC(ACTION) \
    ACTION(speed)              \
    ACTION(replica)            \
    ACTION(delay)              \
    ACTION(pattern)

#define DEFINE_ACTION(_name) _name,
    static enum class exp_type { EXP_TYPE_CODEC(DEFINE_ACTION) } type;
#undef DEFINE_ACTION

    static const char *type_str[];
    static string pattern_name;
    static int round_num;

    static inline void set_default(default_setting *p) { default_p = p; }

    static inline void set_exp_subject(const char *alg_name, const char *rdt_name)
    {
        alg_type = alg_name;
        rdt_type = rdt_name;
    }

    static inline void print_settings()
    {
        cout << "exp subject: [" << alg_type << "-" << rdt_type << "]" << "\n";
        cout << "exp on ";
        cout << "pattern: " << pattern_name;
        cout << ", total ops " << total_ops << endl;
    }

    static inline void set_pattern(const string &name)
    {
        apply_default();
        total_ops = default_p->total_sec * op_per_sec;
        pattern_name = name;
        type = exp_type::pattern;
    }
};

#endif  // BENCH_EXP_SETTING_H
