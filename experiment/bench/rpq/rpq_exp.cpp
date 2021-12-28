//
// Created by admin on 2020/6/30.
//

#include "rpq_exp.h"

#include "../exp_runner.h"
#include "rpq_generator.h"

exp_setting::default_setting rpq_exp::rpq_setting{
    .name = "Rpq",
    .total_sec = 14,
    .total_servers = 3,
    .op_per_sec = 1
};

void rpq_exp::exp_impl(const string& type, const string& pattern, int round)
{
    exp_env env(3, 1, 1, 1);
    for (int i = 0; i < round; i++) {
        rpq_generator gen(type, pattern, i);
        gen.init();
        exp_runner runner(gen, env);
        runner.run(); 
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}
