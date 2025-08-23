#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    // Creates assignments for the parameters of the function call
    Node assign_params(Node &params, Node &args) {
        Node builder = Stmt;
        auto param_it = params->begin();
        auto arg_it = args->begin();

        while (param_it != params->end()) {
            auto atom = *arg_it / Atom;
            builder
                << (Stmt
                    << (Assign << (*param_it / Ident)->clone()
                               << (AExpr << atom->clone())));
            param_it++;
            arg_it++;
        }
        return builder;
    }

    PassDef inlining(
        std::shared_ptr<CallGraph> call_graph,
        std::shared_ptr<ControlFlow> cfg) {
        PassDef pass = {
            "inlining",
            normalization_wf,
            dir::topdown,
            {
                T(Stmt)
                        << (T(Assign) << T(Ident)[Ident] *
                                (T(AExpr) << T(FunCall)[FunCall])) >>
                    [=](Match &_) -> Node {
                    auto fun_call = _(FunCall);
                    auto fun_id = get_identifier(fun_call / FunId);

                    if (!call_graph->can_be_inlined(fun_id)) {
                        return NoChange;
                    }

                    cfg->set_dirty_flag(true);

                    auto fun_def = cfg->get_fun_def(fun_call);
                    auto arg_assignments =
                        assign_params(fun_def / ParamList, fun_call / ArgList);
                    auto fun_body = ((fun_def / Body) / Stmt)->clone();

                    // Ensure inlined function has unique variables
                    auto fresh_vars = std::map<Location, Location>();
                    fun_body->traverse([&](Node node) {
                        if (!(node == Var ||
                              (node == Expr && node / Expr != Ident)))
                            return true;

                        auto ident = node == Var ? (node / Ident) : (node / Expr);
                        auto loc = ident->location();

                        if (fresh_vars.find(loc) == fresh_vars.end()) {
                            fresh_vars[loc] = _.fresh();
                        }
                        Node new_ident = Ident ^ fresh_vars[loc];
                        ident->parent()->replace(ident, new_ident);

                        return true;
                    });

                    // Replace all return statements with assignments
                    Node ret_var = Ident ^ _.fresh();
                    fun_body->traverse([&](Node node) {
                        if (node != Return)
                            return true;

                        Node assign = Assign << ret_var->clone()
                                             << (AExpr << *node);

                        node->parent()->replace(node, assign);

                        return false;
                    });

                    // Replace the previous fun call with assignment
                    // to return ident
                    Node ret_assignment = Stmt
                        << (Assign << _(Ident)
                                   << (AExpr << (Atom << ret_var->clone())));

                    return Seq << *arg_assignments << *fun_body
                               << ret_assignment;
                },
            }};

        return pass;
    }
}
