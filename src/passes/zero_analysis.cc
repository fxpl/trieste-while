#include "../analyses/dataflow_analysis.hh"
#include "../analyses/zero.hh"
#include "../control_flow.hh"
#include "../internal.hh"

namespace whilelang {
    using namespace trieste;

    PassDef z_analysis(std::shared_ptr<ControlFlow> cfg) {
        auto analysis =
            std::make_shared<DataFlowAnalysis<State, ZeroLatticeValue>>(
                zero_create_state, zero_state_join, zero_flow);

        PassDef z_analysis = {
            "z_analysis", normalization_wf, dir::topdown | dir::once, {}};

        z_analysis.post([=](Node) {
            auto first_state = State();

            for (auto var : cfg->get_vars()) {
                first_state[var] = ZeroLatticeValue::top();
            }

            analysis->forward_worklist_algoritm(cfg, first_state);

            cfg->log_instructions();
            analysis->log_state_table(cfg);

            return 0;
        });

        return z_analysis;
    }
}
