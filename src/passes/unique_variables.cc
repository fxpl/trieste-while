#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {

    using namespace trieste;

    // Gives every variable/parameter definition a fresh, function-unique name.
    //
    // Names are assigned per *definition* (the Var or Param node an Ident
    // binds to), not per source name, so a shadowing `var x` in a nested block
    // gets a different name from the outer `x`. After this pass all names in a
    // function are unique
    PassDef unique_variables(
        std::shared_ptr<std::map<std::string, std::string>> vars_map) {
        auto def_names = std::make_shared<std::map<Node, std::string>>();

        // The definition an Ident refers to, or nullptr if it is unbound
        // (e.g. inside an Error node produced by check_refs).
        auto definition_of = [](Node ident) -> Node {
            auto parent = ident->parent();

            // We must perform the lookup from the parent of the Ident
            if (parent == Var || parent == Param)
                return parent;

            auto defs = ident->lookup();
            return defs.empty() ? nullptr : defs.front();
        };

        PassDef pass = {
            "unique_variables",
            unique_variables_wf,
            dir::bottomup | dir::once,
            {
                T(Ident)[Ident] >> [=](Match &_) -> Node {
                    auto ident = _(Ident);
                    auto def = definition_of(ident);

                    if (!def)
                        return NoChange;

                    auto it = def_names->find(def);
                    if (it != def_names->end())
                        return Ident ^ it->second;

                    auto new_name = std::string(_.fresh().view());
                    def_names->insert({def, new_name});

                    // Debug log only: keep shadowed definitions apart by
                    // tagging them with their source position.
                    auto [line, col] = def->location().linecol();
                    vars_map->insert(
                        {build_qualified_name(ident) + "@" +
                             std::to_string(line + 1) + ":" +
                             std::to_string(col + 1),
                         new_name});

                    return Ident ^ new_name;
                },
            }};
        pass.pre([vars_map, def_names](Node) {
            vars_map->clear();
            def_names->clear();
            return 0;
        });
        return pass;
    }
}
