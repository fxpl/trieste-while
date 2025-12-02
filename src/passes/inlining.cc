#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    // Creates assignments for the parameters of the function call
    Node assign_params(Match &_, std::map<Location, Location> &fresh_vars, Node &params, Node &args) {
        Node builder = Stmt;
        auto param_it = params->begin();
        auto arg_it = args->begin();

        while (param_it != params->end()) {
            auto atom = *arg_it / Atom;
            auto ident = *param_it / Ident;

            fresh_vars[ident->location()] = _.fresh();
            auto new_ident = Ident ^ fresh_vars[ident->location()];

            builder
                << (Stmt
                    << (Var << new_ident))
                << (Stmt
                    << (Assign << new_ident->clone()
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

                    auto fresh_vars = std::map<Location, Location>();
                    auto arg_assignments =
                        assign_params(_, fresh_vars, fun_def / ParamList, fun_call / ArgList);

                    // Ensure inlined function has unique variables and uses
                    // fresh parameter names
                    auto fun_body = (fun_def / Body)->clone();
                    fun_body->traverse([&](Node node) {
                        if (node != Ident)
                            return true;

                        auto loc = node->location();

                        if (fresh_vars.find(loc) == fresh_vars.end()) {
                            fresh_vars[loc] = _.fresh();
                        }
                        Node new_ident = Ident ^ fresh_vars[loc];
                        node->parent()->replace(node, new_ident);

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
