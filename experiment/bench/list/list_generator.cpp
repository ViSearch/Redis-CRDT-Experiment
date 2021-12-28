#include "list_generator.h"

constexpr auto MAX_FONT_SIZE = 100;
constexpr auto TOTAL_FONT_TYPE = 10;
constexpr auto MAX_COLOR = 1u << 25u;

#define PA (pattern.PR_ADD)
#define PR (pattern.PR_ADD + pattern.PR_REM)

list_generator::list_op_gen_pattern &list_generator::get_pattern(const string &name)
{
    static map<string, list_op_gen_pattern> patterns{
        {"default", {.PR_ADD = 0.60, .PR_REM = 0.20, .PR_READ = 0.20}},
        {"upddominant", {.PR_ADD = 0.60, .PR_REM = 0.20, .PR_READ = 0.20}}};
    if (patterns.find(name) == patterns.end()) return patterns["default"];
    return patterns[name];
}

struct invocation* list_generator::exec_op(redis_client &c, cmd* op) 
{
    if (op == NULL) {
        return NULL;
    }
    if (op->op_name == DUMMY) {
        return NULL;
    }
    
    redisReply_ptr reply = c.exec(op);
    if (op->op_name == INSERT) {
        if (reply == NULL || reply->type == 6) {
            return NULL;
        }
        invocation* inv = new invocation;
        inv->operation = "insert," + ((list_insert_cmd*)op)->prev + "," + ((list_insert_cmd*)op)->id + "," + ((list_insert_cmd*)op)->content + ",null";
        write_op_executed++;
        return inv;
    }
    else if (op->op_name == REMOVE)
    {
        if (reply == NULL || reply->type == 6) {
            return NULL;
        }
        invocation* inv = new invocation;
        inv->operation = "remove," + ((list_remove_cmd*)op)->id + ",null";
        write_op_executed++;
        return inv;
    }
    else if (op->op_name == READ)
    {
        if (reply == NULL || reply->type == 6) {
            return NULL;
        }
        
        string result = "";
        int r_len = reply->elements;
        if (r_len != 0) {
            for (int i = 0; i < r_len; i++) {
                result += reply->element[i]->element[1]->str;
            }
        } else {
            result = "null";
        }
        invocation* inv = new invocation;
        inv->operation = "read," + result;
        write_op_executed++;
        return inv;
    }
    return NULL;
}

cmd* list_generator::generate_op()
{

    double rand = decide();
    if (rand <= PA) { 
        return generate_insert(); 
    }
    else if (rand <= PR) {
        int id = random_get();
        if (id == -1) {
            return generate_insert();
        }
        string id_str = to_string(id);
        return new list_remove_cmd(type, id_str, round);
    }
    else
    {
        return new list_read_cmd(type, round);
    }
}

cmd* list_generator::generate_insert()
{
    int id;
    id = intRand(MAX_ELE);
    unordered_set<int>::iterator it = elements.find(id);
    int i = 0;
    while (it != elements.end() && i < 5) {
        id = intRand(MAX_ELE);
        unordered_set<int>::iterator it = elements.find(id);
        i++;
    }
    
    int prev = random_get();
    elements.insert(id);
    string content = strRand();
    string id_str = to_string(id);
    if (prev == -1) {
        string prev_str = "null";
        return new list_insert_cmd(type, prev_str, id_str, content, round);
    } else {
        string prev_str = to_string(prev);
        return new list_insert_cmd(type, prev_str, id_str, content, round);
    }
}

cmd* list_generator::generate_dummy()
{
    return new list_cmd(type, DUMMY, round);
}

int list_generator::random_get() {
    if (elements.empty()) return -1;
    int pos = intRand(elements.size() + 1);  // NOLINT
    if (pos == elements.size()) return -1;
    auto random_it = next(begin(elements), pos);
    return *random_it;
}