#include "utils.hh"
#include "../lang.hh"

namespace whilelang {
    using namespace trieste;

    Node get_first_basic_child(Node n) {
        while (n->type().in({Block, Stmt, While, If, Assign})) {
            if (n == Assign) {
                if ((n / Rhs) / Expr == FunCall) {
                    n = (n / Rhs) / Expr;
                }
                break;
            }

            n = n->front();
        }
        return n;
    }

    Node get_last_basic_child(Node n) {
        while (n->type().in({Block, Stmt, While})) {
            n = n->back();
        }
        return n;
    }

    NodeSet get_last_basic_children(const Node n) {
        std::set<Node> children;
        Node curr = n;

        while (curr->type().in({Block, Stmt})) {
            curr = curr->back();
        }

        if (curr == While) {
            children.insert(curr / BAtom);
        } else if (curr == If) {
            auto then_last_nodes = get_last_basic_children(curr / Then);
            auto else_last_nodes = get_last_basic_children(curr / Else);

            children.insert(then_last_nodes.begin(), then_last_nodes.end());
            children.insert(else_last_nodes.begin(), else_last_nodes.end());
        } else {
            children.insert(curr);
        }

        return children;
    }

    int get_int_value(const Node &node) {
        std::string text(node->location().view());
        return std::stoi(text);
    }

    Node create_const_node(int value) {
        return Int ^ std::to_string(value);
    };

    void
    log_var_map(std::shared_ptr<std::map<std::string, std::string>> vars_map) {
        const int width = 10;
        std::stringstream str_builder;

        str_builder << std::left << std::setw(width) << "Org Var"
                    << std::setw(width) << "New Var" << std::endl;

        str_builder << std::endl;
        str_builder << std::string(width * 3, '-') << std::endl;

        for (const auto &[old_var, new_var] : *vars_map) {
            str_builder << std::setw(width) << old_var << " -> "
                        << std::setw(width) << new_var << std::endl;
        }

        logging::Debug() << str_builder.str();
    }
}
