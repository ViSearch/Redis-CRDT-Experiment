//
// Created by LintianShi on 2021/11/11.
//

#ifndef BENCH_SET_GENERATOR_H
#define BENCH_SET_GENERATOR_H

#include "../util.h"
#include "set_cmd.h"

using namespace std;

class set_generator : public generator
{
private:
    struct set_op_gen_pattern
    {
        double PR_ADD;
        double PR_REM;
        double PR_CONTAINS;
        double PR_SIZE;
    };

    static constexpr int MAX_ELE = 6;

    set_op_gen_pattern &pattern;
    const string &zt;
    unordered_set<int> elements;

    static set_op_gen_pattern &get_pattern(const string &name);
    cmd* generate_op() override;
    cmd* generate_add();
    cmd* generate_dummy() override;


public:
    set_generator(const string &type, const string &p, int r)
        : zt(type), pattern(get_pattern(p)), generator(r) {}

    struct invocation* exec_op(redis_client &c, cmd* op) override;
};

#endif  // BENCH_SET_GENERATOR_H
