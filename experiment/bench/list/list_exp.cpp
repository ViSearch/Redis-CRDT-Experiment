#include "list_exp.h"

#include "../exp_runner.h"
#include "list_generator.h"

exp_setting::default_setting list_exp::list_setting{
    .name = "List",
    .total_sec = 14,
    .total_servers = 3,
    .op_per_sec = 1,
    };

void list_exp::exp_impl(const string& type, const string& pattern, int round)
{
    exp_env env(3, 1, 1, 1);
    for (int i = 0; i < round; i++) {
        list_generator gen(type, pattern, i);
        gen.init();
        exp_runner runner(gen, env);
        runner.run();
        this_thread::sleep_for(chrono::milliseconds(100));
    }    
}