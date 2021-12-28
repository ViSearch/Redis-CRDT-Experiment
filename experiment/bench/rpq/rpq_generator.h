//
// Created by user on 18-11-14.
//

#ifndef BENCH_RPQ_GENERATOR_H
#define BENCH_RPQ_GENERATOR_H

#include "../util.h"
#include "rpq_cmd.h"

using namespace std;

class rpq_generator : public generator
{
private:
    struct rpq_op_gen_pattern
    {
        double PR_ADD;
        double PR_INC;
        double PR_MAX;
        double PR_SCORE;
        double PR_REM;

        double PR_ADD_CA;
        double PR_ADD_CR;
        double PR_REM_CA;
        double PR_REM_CR;
    };

    static constexpr int MAX_ELE = 6;
    static constexpr int MAX_INIT = 1000;
    static constexpr int MAX_INCR = 500;

    rpq_op_gen_pattern &pattern;
    const string &zt;
    unordered_set<int> elements;

    static rpq_op_gen_pattern &get_pattern(const string &name);
    cmd* generate_op() override;
    cmd* generate_add();
    cmd* generate_dummy() override;


public:
    rpq_generator(const string &type, const string &p, int r)
        : zt(type), pattern(get_pattern(p)), generator(r) {}

    struct invocation* exec_op(redis_client &c, cmd* op) override;
};

#endif  // BENCH_RPQ_GENERATOR_H
