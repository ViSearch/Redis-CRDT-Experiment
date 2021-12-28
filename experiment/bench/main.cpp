#include "exp_env.h"
#include "exp_setting.h"
#include "rpq/rpq_exp.h"
#include "list/list_exp.h"
#include "set/set_exp.h"

using namespace std;

int main(int argc, char *argv[])
{
    istream::sync_with_stdio(false);
    ostream::sync_with_stdio(false);
    int round = 10;
    if (argc == 3)
    {
        if (strcmp(argv[1], "stop") == 0) {
            exp_env::sudo_pwd = argv[2];
            exp_env e(3, 1, 1, 1);
            return 0;
        } else {
            exp_env::sudo_pwd = argv[2];
            round = atoi(argv[1]);
            rpq_exp re;
            exp_setting::compare = false;
            re.test_default_settings(round);
            return 0;
        }
    }
    else if (argc == 4) {
        exp_env::sudo_pwd = argv[3];
        round = atoi(argv[2]);
        if (strcmp(argv[1], "rpq") == 0) {
            rpq_exp re;
            exp_setting::compare = false;
            re.test_default_settings(round);
            return 0;
        } else if (strcmp(argv[1], "list") == 0) {
            list_exp le;
            exp_setting::compare = false;
            le.test_default_settings(round);
            return 0;
        } else if (strcmp(argv[1], "set") == 0) {
            set_exp se;
            exp_setting::compare = false;
            se.test_default_settings(round);
            return 0;
        }
    }

    return -1;
}
