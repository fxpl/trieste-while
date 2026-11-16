#include "control_flow.hh"

#include "utils.hh"

namespace whilelang {
    using namespace trieste;

    // Public
    ControlFlow::ControlFlow() {
        this->instructions = Nodes();
        this->vars = Vars();
        this->predecessor = NodeMap<NodeSet>();
        this->successor = NodeMap<NodeSet>();
        this->fun_call_to_def = NodeMap<Node>();
        this->fun_def_to_calls = NodeMap<NodeSet>();
        this->dirty_flag = false;
    }

    void ControlFlow::clear() {
        instructions.clear();
        vars.clear();
        predecessor.clear();
        successor.clear();
        fun_call_to_def.clear();
        fun_def_to_calls.clear();
        program_entries.clear();
        program_exits.clear();
    }

    void ControlFlow::add_var(Node ident) {
        vars.insert(get_identifier(ident));
    };

    void ControlFlow::add_edge(const Node &u, const Node &v) {
        append_to_nodemap(successor, u, v);
        append_to_nodemap(predecessor, v, u);
    }

    void ControlFlow::add_edge(const NodeSet &u, const Node &v) {
        append_to_nodemap(successor, u, v);
        append_to_nodemap(predecessor, v, u);
    }

    // Fill the map with function calls to their definitions
    void ControlFlow::set_functions_calls(
        std::shared_ptr<Nodes> fun_defs, std::shared_ptr<NodeSet> fun_calls) {
        for (auto fun_call : *fun_calls) {
            auto call_id_str = get_identifier(fun_call / FunId);

            for (auto fun_def : *fun_defs) {
                auto fun_def_str = get_identifier(fun_def / FunId);
                if (call_id_str == fun_def_str) {
                    append_to_nodemap(fun_def_to_calls, fun_def, fun_call);
                    fun_call_to_def.insert({fun_call, fun_def});
                }
            }
        }

        // Compilation requires a main function, but programs without one can
        // still be analysed by treating every function as an entry point
        auto main = std::find_if(fun_defs->begin(), fun_defs->end(), [](auto &f) {
            return get_identifier(f / FunId) == "main";
        });
        if (main != fun_defs->end()) {
            program_entries = {*main};
        } else {
            program_entries = *fun_defs;
        }

        for (auto entry : program_entries) {
            program_exits.push_back(get_last_basic_child(entry / Body));
        }
    }

    void ControlFlow::log_instructions() {
        logging::Debug() << "Instructions: ";
        for (size_t i = 0; i < instructions.size(); i++) {
            logging::Debug() << i + 1 << "\n" << instructions[i] << std::endl;
        }
    }

    // Private

    void ControlFlow::append_to_nodemap(
        NodeMap<NodeSet> &map, const Node &key, const Node &value) {
        auto res = map.find(key);

        if (res != map.end()) {
            // Append to nodemap entry if already exist
            res->second.insert(value);
        } else {
            // Otherwise create new entry
            map.insert({key, {value}});
        }
    }

    void ControlFlow::append_to_nodemap(
        NodeMap<NodeSet> &map, const Node &key, const NodeSet &values) {
        auto res = map.find(key);

        if (res != map.end()) {
            res->second.insert(values.begin(), values.end());
        } else {
            map.insert({key, values});
        }
    }

    void ControlFlow::append_to_nodemap(
        NodeMap<NodeSet> &map, const NodeSet &nodes, const Node &value) {
        for (auto &node : nodes) {
            append_to_nodemap(map, node, value);
        }
    }
}
