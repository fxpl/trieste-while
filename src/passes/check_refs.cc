#include "../internal.hh"
#include "../utils.hh"
namespace whilelang {

    using namespace trieste;

    PassDef check_refs() {
        // Number of parameters of each function, by name
        auto arities = std::make_shared<std::map<std::string, size_t>>();
        auto defined = std::make_shared<std::set<std::string>>();

        PassDef check_refs = {
            "check_refs",
            statements_wf,
            dir::bottomup | dir::once,
            {
                T(Assign) << T(Ident)[Ident]([](auto &n) -> bool {
                    auto defs = lookup_variable(n.front());
                    return defs.size() == 1 && defs.front() == Param;
                }) >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Ident))
                                 << (ErrorMsg ^ "Parameters cannot be reassigned");
                },

                T(AExpr, Assign) << T(Ident)[Ident] >> [](Match &_) -> Node {
                    auto def = lookup_variable(_(Ident));
                    if (def.empty()) {
                        return Error << (ErrorAst << _(Ident))
                                     << (ErrorMsg ^ "Undefined variable");
                    } else if (def.size() > 1) {
                        return Error
                            << (ErrorAst << _(Ident))
                            << (ErrorMsg ^
                                "Variable can not be defined multiple times");
                    }

                    return NoChange;
                },

                T(Return)[Return] >> [](Match &_) -> Node {
                    auto fun_id = _(Return)->parent(FunDef) / FunId;
                    if (get_identifier(fun_id) == "main") {
                        return Error
                            << (ErrorAst << _(Return))
                            << (ErrorMsg ^ "The main function cannot return a value");
                    }

                    return NoChange;
                },

                T(FunCall)[FunCall] << (T(FunId)[FunId] * T(ArgList)[ArgList]) >>
                    [=](Match &_) -> Node {
                    auto fun_id = get_identifier(_(FunId));
                    auto arity = arities->find(fun_id);

                    std::string msg;
                    if (fun_id == "main") {
                        msg = "The main function cannot be called";
                    } else if (arity == arities->end()) {
                        msg = "Call to undefined function";
                    } else if (_(ArgList)->size() != arity->second) {
                        msg = "Wrong number of arguments";
                    } else {
                        return NoChange;
                    }

                    return Error << (ErrorAst << _(FunCall))
                                 << (ErrorMsg ^ msg);
                },

                T(FunDef) << T(FunId)[FunId] >> [=](Match &_) -> Node {
                    if (!defined->insert(get_identifier(_(FunId))).second) {
                        return Error << (ErrorAst << _(FunId))
                                     << (ErrorMsg ^ "Function defined multiple times");
                    }

                    return NoChange;
                },

                T(Program)[Program] >> [=](Match &_) -> Node {
                    if (!arities->contains("main")) {
                        return Error << (ErrorAst << _(Program))
                                     << (ErrorMsg ^ "Could not find main function");
                    }

                    return NoChange;
                },
            }};

        check_refs.pre([=](Node ast) {
            arities->clear();
            defined->clear();

            ast->traverse([&](Node node) {
                if (node != FunDef)
                    return true;

                auto fun_id = get_identifier(node / FunId);
                arities->emplace(fun_id, (node / ParamList)->size());
                return false;
            });
            return 0;
        });

        return check_refs;
    }
}
