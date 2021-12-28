#ifndef BENCH_LIST_GENERATOR_H
#define BENCH_LIST_GENERATOR_H

#include "../util.h"
#include "list_cmd.h"

class list_generator : public generator
{
private:
    struct list_op_gen_pattern
    {
        double PR_ADD;
        double PR_REM;
        double PR_READ;
    };

    static constexpr int MAX_ELE = 10000;

    list_op_gen_pattern &pattern;
    const string &type;
    unordered_set<int> elements;

    static list_op_gen_pattern &get_pattern(const string &name);
    cmd* generate_op() override;
    cmd* generate_insert();
    cmd* generate_dummy() override;
    int random_get();

    list_insert_cmd *gen_insert();
    struct invocation* exec_insert(redis_client& c);
    struct invocation* exec_getNext(redis_client& c, string& prev);
    struct invocation* exec_getIndex(redis_client& c, int index);
    struct invocation* exec_remove(redis_client& c, string &id);
    struct invocation* exec_dummy(redis_client& c);

public:
    list_generator(const string &type, const string &p, int r)
        : type(type), pattern(get_pattern(p)), generator(r) {}

    struct invocation* exec_op(redis_client &c, cmd* op) override;
};

#endif  // BENCH_LIST_GENERATOR_H