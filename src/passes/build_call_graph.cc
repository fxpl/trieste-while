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
                T(FunCall)[FunCall] >> [=](Match &_) -> Node {
                    auto caller = _(FunCall) / FunId;
                    auto surronding_fun = _(FunCall)->parent(FunDef) / FunId;

                    //call_graph->add_vertex(caller);
                    //call_graph->add_vertex(surronding_fun);
                    call_graph->add_edge(surronding_fun, caller);

                    return NoChange;
                },
            }};

        pass.post([=](Node) {
            call_graph->calculate_inlineable_funs();
			call_graph->log_functions_to_inline();
            return 0;
        });

        return pass;
    }
}
