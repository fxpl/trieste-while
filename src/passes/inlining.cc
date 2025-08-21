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
            builder
                << (Stmt
                    << (Assign << (*param_it / Ident)->clone()
                               << (AExpr << (*arg_it / Atom)->clone())));
            param_it++;
            arg_it++;
        }
        return builder;
    }

    PassDef inlining(
        std::shared_ptr<CallGraph> call_graph,
        std::shared_ptr<ControlFlow> cfg) {
        auto create_return_ident = [=]() -> Node { return Ident ^ "__ret"; };

        return {
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

                    // Replace all return statements with assignments
                    fun_body->traverse([=](Node node) {
                        if (node != Return)
                            return true;

                        Node assign = Assign << create_return_ident()
                                             << (AExpr << *node);

                        node->parent()->replace(node, assign);

                        return false;
                    });

                    // Replace the previous fun call with assignment
                    // to return ident
                    Node ret_assignment = Stmt
                        << (Assign
                            << _(Ident)
                            << (AExpr << (Atom << create_return_ident())));

                    return Seq << *arg_assignments << *fun_body
                               << ret_assignment;
                },
            }};
    }
}
