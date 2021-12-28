//
// Created by LintianShi on 2021/11/11.
//

#include "set_exp.h"

#include "../exp_runner.h"
#include "set_generator.h"

exp_setting::default_setting set_exp::set_setting{
    .name = "Set",
    .total_sec = 14,
    .total_servers = 3,
    .op_per_sec = 1
};

void set_exp::exp_impl(const string& type, const string& pattern, int round)
{
    exp_env env(3, 1, 1, 1);
    for (int i = 0; i < round; i++) {
        set_generator gen(type, pattern, i);
        gen.init();
        exp_runner runner(gen, env);
        runner.run(); 
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}
