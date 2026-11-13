#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    PassDef to3addr() {
        PassDef to3addr = {
            "to3addr",
            three_addr_wf,
            dir::bottomup | dir::once,
            {
                In(FunDef) * T(Block)[Body] >>
                  [](Match &_) -> Node {
                    auto body = _(Body);
                    auto fun_id = body->parent() / FunId;
                    bool is_main = get_identifier(fun_id) == "main";
                    bool has_label = body->at(0) / Stmt == Label;
                    if (has_label && !is_main) return NoChange;

                    if (!has_label) {
                        body->push_front(Stmt << (Label ^ _.fresh()));
                    }

                    // The main function implicitly returns nothing at the end
                    if (is_main) {
                        body->push_back(Stmt << Return);
                    }

                    return body;
                  },

                T(Stmt) <<
                  (T(If) << (T(BAtom)[BAtom] * T(Block)[Then] * T(Block)[Else])) >>
                    [](Match &_) -> Node {
                        auto then_label = Label ^ _.fresh();
                        auto else_label = Label ^ _.fresh();
                        auto end_label = Label ^ _.fresh();
                        auto cond_ident = Ident ^ _.fresh();

                        auto cond_assign = Stmt << (Assign << cond_ident
                                                           << (BExpr << _(BAtom)));

                        auto cond = (Stmt << (Cond << cond_ident->clone()
                                                   << then_label->clone()
                                                   << else_label->clone()));
                        return Seq << cond_assign
                                   << cond
                                   << (Stmt << then_label)
                                   << *_(Then)
                                   << (Stmt << (Jump << end_label->clone()))
                                   << (Stmt << else_label)
                                   << *_(Else)
                                   << (Stmt << (Jump << end_label->clone()))
                                   << (Stmt << end_label);
                    },

                T(Stmt) <<
                  (T(While) << (T(Block)[Block] * T(BAtom)[BAtom] * T(Block)[Do])) >>
                      [](Match &_) -> Node {
                            auto cond_label = Label ^ _.fresh();
                            auto do_label = Label ^ _.fresh();
                            auto end_label = Label ^ _.fresh();
                            auto cond_ident = Ident ^ _.fresh();

                            auto cond_block = _(Block);
                            auto body_block = _(Do);

                            auto cond_assign = Stmt << (Assign << cond_ident
                                                               << (BExpr << _(BAtom)));

                            auto cond = (Stmt << (Cond << cond_ident->clone()
                                                       << do_label->clone()
                                                       << end_label->clone()));
                            return Seq << (Stmt << cond_label)
                                       << *cond_block
                                       << cond_assign
                                       << cond
                                       << (Stmt << do_label)
                                       << *body_block
                                       << (Stmt << (Jump << cond_label->clone()))
                                       << (Stmt << end_label);
                      },

                T(Stmt) <<
                  (T(Return) << (T(Atom) << T(Ident)[Ident])) >>
                      [](Match &_) -> Node {
                            return Stmt << (Return << _(Ident));
                      },

                T(Stmt) <<
                  (T(Return) << (T(Atom)[Atom] << T(Int, Input))) >>
                      [](Match &_) -> Node {
                            auto tmp = Ident ^ _.fresh();
                            auto assign = Stmt << (Assign << tmp
                                                          << (AExpr << _(Atom)));
                            return Seq << assign
                                       << (Stmt << (Return << tmp->clone()));
                      },

                In(Block) * T(Stmt) << T(Block)[Block] >>
                    [](Match &_) -> Node {
                        return Seq << *_(Block);
                    },
            },
        };

        return to3addr;
    }
}
