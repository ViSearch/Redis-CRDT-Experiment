//
// Created by admin on 2020/4/16.
//
#define LIBSSH_STATIC 1
#include <libssh/libssh.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#ifndef BENCH_EXP_ENV_H
#define BENCH_EXP_ENV_H
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <thread>
using namespace std;
#include "exp_setting.h"

#define REDIS_SERVER "../../redis-6.0.5/src/redis-server"
#define REDIS_CONF "../../redis-6.0.5/redis.conf"
#define REDIS_CLIENT "../../redis-6.0.5/src/redis-cli"

#define IP_BETWEEN_CLUSTER "127.0.0.3"
#define IP_WITHIN_CLUSTER "127.0.0.2"
#define IP_SERVER "127.0.0.1"
constexpr int BASE_PORT = 6379;

class exp_env
{
private:
    int cluster_num;
    int* replica_nums;
    int total_server;
    string exec_path;
    vector<ssh_session> sessions;
    string available_hosts[3] = {"172.24.81.133", "172.24.81.136", "172.24.81.135"};
    string available_ports[5] = {"6379", "6380", "6381", "6382", "6383"};
    static void shell_exec(const char* cmd, bool sudo)
    {
//#define PRINT_CMD
#ifdef PRINT_CMD
        if (sudo)
            cout << "\nsudo " << cmd << endl;
        else
            cout << "\n" << cmd << endl;
#endif
        ostringstream cmd_stream;
        if (sudo) cmd_stream << "echo " << sudo_pwd << " | sudo -S ";
        cmd_stream << cmd << " 1>/dev/null";  // or " 1>/dev/null 2>&1"
        system(cmd_stream.str().c_str());
    }

    static inline void shell_exec(const string& cmd, bool sudo) { shell_exec(cmd.c_str(), sudo); }

    int ssh_exec(ssh_session session, string cmd) {
        ssh_channel channel;
        channel = ssh_channel_new(session);
        if (channel == NULL) {
            fprintf(stderr, "%s\n", "channel is NULL");
            return -1;
        }

        int rc;
        rc = ssh_channel_open_session(channel);
        if (rc != SSH_OK)
        {
            ssh_channel_free(channel);
            return -1;
        }

        string exec_cmd = exec_path + cmd;
        rc = ssh_channel_request_exec(channel, exec_cmd.c_str());
        if (rc != SSH_OK)
        {
            ssh_channel_close(channel);
            ssh_channel_free(channel);
            printf("%s\n", "ssh is not OK");
            return -1;
        }
 
        int nbytes;
        char buffer[256];
        nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
        while (nbytes > 0)
        {
            if (write(1, buffer, nbytes) != (unsigned int) nbytes)
            {
                ssh_channel_close(channel);
                ssh_channel_free(channel);
                exit(-1);
            }
            nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
        }
    
        ssh_channel_send_eof(channel);
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return 0;
    }

    int connect_all() {
        for (int i = 0; i < cluster_num; i++) {
            if (connect_one_server(available_hosts[i]) != 0) {
                return -1;
            }
            
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        printf("connect all.\n");
        return 0;
    }

    int connect_one_server(string host) {
        ssh_session my_ssh_session = ssh_new();
        if (my_ssh_session == NULL)
            return -1;

        int port = 22;
        ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, host.c_str());
        ssh_options_set(my_ssh_session, SSH_OPTIONS_USER, "shilintian");
        ssh_options_set(my_ssh_session, SSH_OPTIONS_PORT, &port);

        int rc;
        rc = ssh_connect(my_ssh_session);
        if (rc != SSH_OK)
        {
            fprintf(stderr, "Error connecting to localhost: %s\n",
            ssh_get_error(my_ssh_session));
            ssh_free(my_ssh_session);
            return rc;
        }

        rc = ssh_userauth_password(my_ssh_session, NULL, exp_env::sudo_pwd.c_str());
        if (rc != SSH_AUTH_SUCCESS)
        {
            fprintf(stderr, "Error authenticating with password: %s\n", ssh_get_error(my_ssh_session));
            ssh_disconnect(my_ssh_session);
            ssh_free(my_ssh_session);
            return rc;
        }
        sessions.push_back(my_ssh_session);
        return 0;
    }

    void start_servers()
    {
        for (int i = 0; i < sessions.size(); i++) {
            ssh_session session = sessions[i];
            string cmd = "./server.sh";
            for (int j = 0; j < replica_nums[i]; j++) {
                cmd += " " + available_ports[j];
            }
            if (ssh_exec(session, cmd.c_str()) != 0) {
                printf("start failure\n");
            } else {
                printf("start success\n");
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        printf("start & connect.\n");
    }

    void construct_repl()
    {
        vector<string> repl_cmd;
        repl_cmd.push_back(string("../../redis-6.0.5/src/redis-cli -h 127.0.0.1 -p"));
        repl_cmd.push_back(string("1"));
        repl_cmd.push_back(string("REPLICATE"));
        repl_cmd.push_back(std::to_string(total_server));
        repl_cmd.push_back(string("0"));
        repl_cmd.push_back(string("exp_local"));
        int host_id = 0;
        for (int i = 0; i < cluster_num; i++) {
            for (int j = 0; j < replica_nums[i]; j++) {
                repl_cmd[1] = available_ports[j];
                ssh_exec(sessions[i], generate_repl_cmd(repl_cmd));
                repl_cmd[4] = to_string(host_id + 1);
                repl_cmd.push_back(available_hosts[i]);
                repl_cmd.push_back(available_ports[j]);
                host_id++;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        printf("Replication construct complete.\n");
    }

    string generate_repl_cmd(vector<string>& cmds) {
        string cmd = cmds[0];
        for (int i = 1; i < cmds.size(); i++) {
            cmd += " " + cmds[i];
        }
        return cmd;
    }

    void shutdown_servers()
    {
        for (int i = 0; i < sessions.size(); i++) {
            ssh_session session = sessions[i];
            string cmd = "./shutdown.sh";
            for (int j = 0; j < replica_nums[i]; j++) {
                cmd += " " + available_ports[j];
            }
            ssh_exec(session, cmd.c_str());
        }
        std::this_thread::sleep_for(std::chrono::seconds(4));
        printf("shutdown all.\n");
    }

    void clean() { 
        for (int i = 0; i < sessions.size(); i++) {
            ssh_session session = sessions[i];
            string cmd = "./clean.sh";
            for (int j = 0; j < replica_nums[i]; j++) {
                cmd += " " + available_ports[j];
            }
            ssh_exec(session, cmd.c_str());
        }
        std::this_thread::sleep_for(std::chrono::seconds(4));
        printf("clean.\n");
    }

public:
    static string sudo_pwd;

    int get_cluster_num() {
        return cluster_num;
    }

    int* get_replica_nums() {
        return replica_nums;
    }

    exp_env(int cluster, ...)
    {
        cluster_num = cluster;
        replica_nums = new int[cluster];
        total_server = 0;
        va_list argptr;
        va_start( argptr, cluster); 
        for (int i = 0; i < cluster; i++) {
            replica_nums[i] = va_arg(argptr, int);
            total_server += replica_nums[i];
        }
        va_end(argptr);

        exec_path = "cd /home/shilintian/crdt-redis-experiment/experiment/redis_test/;";
        if (connect_all() != 0) {
            exit(-1);
        }
        exp_setting::print_settings();
        start_servers();
        cout << "server started, " << flush;
        construct_repl();
        cout << "replication constructed, " << flush;
    }

    ~exp_env()
    {
        shutdown_servers();
        cout << "server shutdown, " << flush;
        clean();
        cout << "cleaned\n" << endl;
        for (int i = 0; i < sessions.size(); i++) {
            ssh_session session = sessions[i];
            ssh_disconnect(session);
            ssh_free(session);
        }
        delete []replica_nums;
    }
};

#endif  // BENCH_EXP_ENV_H
