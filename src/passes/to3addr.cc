#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef to3addr() {
        PassDef to3addr = {
            "to3addr",
            three_addr_wf,
            dir::bottomup | dir::once,
            {
                In(FunDef) * (T(Stmt) << T(Block)[Body]) >>
                  [](Match &_) -> Node {
                    auto body = _(Body);
                    if (body->at(0) / Stmt == Label) return NoChange;
                    body->push_front(Stmt << (Label ^ body->fresh()));

                    return Stmt << body;
                  },

                T(Stmt) <<
                  (T(If) << (T(BExpr)[BExpr] * T(Stmt)[Then] * T(Stmt)[Else])) >>
                    [](Match &_) -> Node {
                        auto then_label = Label ^ _(BExpr)->fresh();
                        auto else_label = Label ^ _(BExpr)->fresh();
                        auto end_label = Label ^ _(BExpr)->fresh();
                        auto cond_ident = Ident ^ _(BExpr)->fresh();

                        auto cond_assign = Stmt << (Assign << cond_ident
                                                           << _(BExpr));

                        auto cond = (Stmt << (Cond << cond_ident->clone()
                                                   << then_label->clone()
                                                   << else_label->clone()));
                        return Seq << cond_assign
                                   << cond
                                   << (Stmt << then_label)
                                   << *(_(Then) / Stmt)
                                   << (Stmt << (Jump << end_label->clone()))
                                   << (Stmt << else_label)
                                   << *(_(Else) / Stmt)
                                   << (Stmt << (Jump << end_label->clone()))
                                   << (Stmt << end_label);
                    },

                T(Stmt) <<
                  (T(While) << (T(BExpr)[BExpr] * T(Stmt)[Do])) >>
                      [](Match &_) -> Node {
                            auto cond_label = Label ^ _(BExpr)->fresh();
                            auto do_label = Label ^ _(BExpr)->fresh();
                            auto end_label = Label ^ _(BExpr)->fresh();
                            auto cond_ident = Ident ^ _(BExpr)->fresh();

                            auto cond_assign = Stmt << (Assign << cond_ident
                                                               << _(BExpr));

                            auto cond = (Stmt << (Cond << cond_ident->clone()
                                                       << do_label->clone()
                                                       << end_label->clone()));
                            return Seq << (Stmt << cond_label)
                                       << cond_assign
                                       << cond
                                       << (Stmt << do_label)
                                       << *(_(Do) / Stmt)
                                       << (Stmt << (Jump << cond_label->clone()))
                                       << (Stmt << end_label);
                      },

                T(Stmt) <<
                  (T(Return) << (T(Atom) << T(Ident)[Ident])) >>
                      [](Match &_) -> Node {
                            return Stmt << (Return << Ident);
                      },

                T(Stmt) <<
                  (T(Return) << (T(Atom) << T(Int)[Int])) >>
                      [](Match &_) -> Node {
                            auto tmp = Ident ^ _(Int)->fresh();
                            auto assign = Stmt << (Assign << tmp
                                                          << (AExpr << (Atom << _(Int))));
                            return Seq << assign
                                       << (Stmt << (Return << tmp->clone()));
                      },
            },
        };

        return to3addr;
    }
}
