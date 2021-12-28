//
// Created by LintianShi on 2021/11/11.
//

#ifndef BENCH_SET_EXP_H
#define BENCH_SET_EXP_H

#include "../exp_setting.h"
#include "../util.h"

class set_exp : public rdt_exp
{
private:
    static exp_setting::default_setting set_setting;

    void exp_impl(const string& type, const string& pattern, int round) override;

public:
    set_exp() : rdt_exp(set_setting, "set")
    {
        // ! RPQ types: "o", "r", "rwf"
        //add_type("rwf");
        add_type("o");
        add_pattern("default");
    }
};

#endif  // BENCH_SET_EXP_H
