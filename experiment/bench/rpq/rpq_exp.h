//
// Created by admin on 2020/6/30.
//

#ifndef BENCH_RPQ_EXP_H
#define BENCH_RPQ_EXP_H

#include "../exp_setting.h"
#include "../util.h"

class rpq_exp : public rdt_exp
{
private:
    static exp_setting::default_setting rpq_setting;

    void exp_impl(const string& type, const string& pattern, int round) override;

public:
    rpq_exp() : rdt_exp(rpq_setting, "rpq")
    {
        // ! RPQ types: "o", "r", "rwf"
        add_type("r");
        // add_type("o");
        add_pattern("default");
    }
    
    rpq_exp(const string& type) : rdt_exp(rpq_setting, "rpq")
    {
        // ! RPQ types: "o", "r", "rwf"
        // add_type("rwf");
        add_type(type.c_str());
        add_pattern("default");
    }
};

#endif  // BENCH_RPQ_EXP_H
