#include "../internal.hh"
#include "../utils.hh"
namespace whilelang {

    using namespace trieste;

    PassDef check_refs() {
        return {
            "check_refs",
            statements_wf,
            dir::bottomup | dir::once,
            {
                T(Assign) << T(Ident)[Ident]([](auto &n) -> bool {
                    auto defs = n.front()->lookup();
                    return defs.size() == 1 && defs.front() == Param;
                }) >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Ident))
                                 << (ErrorMsg ^ "Parameters cannot be reassigned");
                },

                T(AExpr, Assign) << T(Ident)[Ident] >> [](Match &_) -> Node {
                    auto def = _(Ident)->lookup();
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

                T(FunCall)[FunCall] << T(FunId)[FunId] >> [](Match &_) -> Node {
                    if (get_identifier(_(FunId)) == "main") {
                        return Error
                            << (ErrorAst << _(FunCall))
                            << (ErrorMsg ^ "The main function cannot be called");
                    }

                    return NoChange;
                },
            }};
    }
}
