#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {

    using namespace trieste;

    PassDef unique_variables(
        std::shared_ptr<std::map<std::string, std::string>> vars_map) {
        PassDef pass = {
            "unique_variables",
            statements_wf,
            dir::bottomup | dir::once,
            {
                T(Ident)[Ident] >> [=](Match &_) -> Node {
                    auto var = build_qualified_name(_(Ident));
                    auto new_var = vars_map->find(var);

                    if (new_var == vars_map->end()) {
                        auto new_name = std::string(_.fresh().view());
                        vars_map->insert({var, new_name});

                        return Ident ^ new_name;
                    } else {
                        return Ident ^ new_var->second;
                    }
                },
            }};
        pass.pre([vars_map](Node) {
            vars_map->clear();
            return 0;
        });
        return pass;
    }
}
