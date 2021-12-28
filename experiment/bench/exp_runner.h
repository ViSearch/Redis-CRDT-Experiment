//
// Created by admin on 2020/1/7.
//

#ifndef BENCH_EXP_RUNNER_H
#define BENCH_EXP_RUNNER_H

#include <future>
#include <thread>

#include "exp_env.h"
#include "exp_setting.h"
#include "util.h"

#if defined(__linux__)

#include <hiredis/hiredis.h>

#elif defined(_WIN32)

#include <direct.h>

#include "../../redis-6.0.5/deps/hiredis/hiredis.h"

#endif

constexpr int THREAD_PER_SERVER = 1;
#define RUN_CONDITION gen.write_op_executed < exp_setting::total_ops

// time in seconds
#define INTERVAL_TIME ((double)TOTAL_SERVERS * THREAD_PER_SERVER / exp_setting::op_per_sec)
constexpr int TIME_OVERHEAD = 1;
constexpr int TIME_READ = 1;


using namespace std;
// extern const char *ips[];

class exp_runner
{
private:
    generator &gen;
    exp_env &env;

    vector<thread> thds;
    execution_trace traces;

    void conn_one_server_timed(const char* ip, int port)
    {
        for (int i = 0; i < THREAD_PER_SERVER; ++i)
        {
            thds.emplace_back([this, ip, port] {
                redis_client c(ip, port);
                thread_trace* trace = new thread_trace;
                for (int t = 1; RUN_CONDITION; ++t)
                {
                    trace->insert(gen.exec_op(c, gen.get_op()));
                    this_thread::sleep_for(chrono::nanoseconds(10));
                }
                traces.add(trace);
            });
        }
    }

public:
    exp_runner(generator &gen, exp_env &env) : gen(gen), env(env) {}

    void run()
    {
        string ips[3] = {"172.24.81.136", "172.24.81.133", "172.24.81.135"};
        // vector<redis_client> clients;
        for (int i = 0; i < env.get_cluster_num(); i++) {
            for (int j = 0; j < env.get_replica_nums()[i]; j++) {
                conn_one_server_timed(ips[i].c_str(), BASE_PORT + j);
            }
        }
        
        // thread progress_thread([this] {
        //     constexpr int barWidth = 50;
        //     double progress;
        //     while (RUN_CONDITION)
        //     {
        //         progress = gen.write_op_executed / ((double)exp_setting::total_ops);
        //         cout << "\r[";
        //         int pos = barWidth * progress;
        //         for (int i = 0; i < barWidth; ++i)
        //         {
        //             if (i < pos)
        //                 cout << "=";
        //             else if (i == pos)
        //                 cout << ">";
        //             else
        //                 cout << " ";
        //         }
        //         cout << "] " << (int)(progress * 100) << "%" << flush;
        //         this_thread::sleep_for(chrono::seconds(1));
        //     }
        //     cout << "\r[";
        //     for (int i = 0; i < barWidth; ++i)
        //         cout << "=";
        //     cout << "] 100%" << endl;
        // });

        auto start = chrono::steady_clock::now();
        for (auto &t : thds)
            if (t.joinable()) t.join();
        // if (progress_thread.joinable()) progress_thread.join();

        auto end = chrono::steady_clock::now();
        auto time = chrono::duration_cast<chrono::duration<double>>(end - start).count();
        cout << time << " seconds, " << gen.write_op_executed / time << " op/s\n";
        cout << "round " << gen.get_round() << ", " << gen.write_op_executed << " operations actually executed on redis." << endl;
        traces.outputTrace();
    }
};

#endif  // BENCH_EXP_RUNNER_H
