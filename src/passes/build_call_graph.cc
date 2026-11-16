#include "../internal.hh"
#include "../utils.hh"

namespace whilelang {
    using namespace trieste;

    PassDef build_call_graph(std::shared_ptr<CallGraph> call_graph) {
        PassDef pass = {
            "build_call_graph",
            normalization_wf,
            dir::topdown | dir::once,
            {
                T(FunDef)[FunDef] >> [=](Match &_) -> Node {
                    call_graph->add_fun_def(_(FunDef));
                    return NoChange;
                },

                T(FunCall)[FunCall] >> [=](Match &_) -> Node {
                    auto caller = _(FunCall) / FunId;
                    auto surronding_fun = _(FunCall)->parent(FunDef) / FunId;

                    call_graph->add_edge(surronding_fun, caller);

                    return NoChange;
                },
            }};

        pass.pre([=](Node) {
            call_graph->clear();
            return 0;
        });

        pass.post([=](Node) {
            call_graph->calculate_inlineable_funs();
            return 0;
        });

        return pass;
    }
}
