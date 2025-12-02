#pragma once
#include <trieste/trieste.h>

namespace whilelang {
    using namespace ::trieste;

    Node get_first_basic_child(Node n);

    Node get_last_basic_child(Node n);

    NodeSet get_last_basic_children(Node n);

    int get_int_value(const Node &node);

    std::string get_identifier(const Node &node);

    Node create_const_node(int value);

	void log_var_map(std::shared_ptr<std::map<std::string, std::string>> vars_map);
}
