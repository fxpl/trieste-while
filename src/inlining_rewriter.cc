#include "internal.hh"

namespace whilelang {
    using namespace trieste;

    Rewriter inlining_rewriter() {
        auto call_graph = std::make_shared<CallGraph>();

        Rewriter rewriter = {
            "inlining_rewriter",
            {
                build_call_graph(call_graph),
                inlining(call_graph),
            },
            whilelang::normalization_wf,
        };

        return rewriter;
    }
}
