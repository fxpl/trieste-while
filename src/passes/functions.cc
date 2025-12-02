#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef functions() {
        return {
            "functions",
            functions_wf,
            dir::topdown,
            {
                In(Top) * T(File) << T(Group)[Group] >>
                    [](Match &_) -> Node { return Program << *_(Group); },

                T(FunDef) << (T(Group) << (T(Ident)[Ident] * T(Paren)[Paren] * T(Brace)[Brace] * End)) >>
                    [](Match &_) -> Node {
                    return FunDef << (FunId ^ _(Ident))
                                  << (ParamList << *_(Paren))
                                  << (Body << *_(Brace));
                },

                In(ParamList) * T(Comma, Group)[Group] >>
                    [](Match &_) -> Node {
                    return Seq << *_(Group);
                },

                In(ParamList) * T(Ident)[Ident] >>
                    [](Match &_) -> Node { return (Param << _(Ident)); },

                // Error rules
                In(Program, File) * (!T(FunDef))[Expr] >> [](Match &_) -> Node {
                    return Error
                        << (ErrorAst << _(Expr))
                        << (ErrorMsg ^ "Unexpected term outside of function declaration");
                },

                T(FunDef) << (T(Group) << (T(Ident) * T(Paren) * T(Brace) * Any[Expr])) >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Expr))
                                 << (ErrorMsg ^ "Expected function declaration");
                },

                T(FunDef)[FunDef] << (--(T(FunId) * T(ParamList) * T(Body))) >>
                    [](Match &_) -> Node {
                    return Error << (ErrorAst << _(FunDef))
                                 << (ErrorMsg ^ "Invalid function declaration");
                },

                T(Program, File) << End >> [](Match &_) -> Node {
                    return Error << (ErrorAst << _(Program))
                                 << (ErrorMsg ^ "Expected function declaration");
                },
            }};
    }
}
