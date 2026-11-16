#include "utils.hh"

namespace whilelang {
    using namespace trieste;

    std::string get_identifier(const Node &node) {
        return std::string(node->location().view());
    }

    Nodes lookup_variable(const Node &ident) {
        auto defs = ident->lookup();
        std::erase_if(defs, [](auto &def) { return def == FunDef; });
        return defs;
    }

    std::string build_qualified_name(const Node ident) {
        auto curr = ident;
        while (curr != FunDef) {
            curr = curr->parent();
        }

        auto fun_id = curr / FunId;
        return get_identifier(fun_id) + "-" + get_identifier(ident);
    };
}
