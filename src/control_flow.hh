#pragma once
#include "analyses/utils.hh"
#include "lang.hh"

namespace whilelang {
    using namespace trieste;

    using Vars = std::set<std::string>;

    class ControlFlow {
      public:
        ControlFlow();

        void clear();

        inline const NodeSet &successors(const Node &node) {
            return successor[node];
        };

        inline const NodeSet &predecessors(const Node &node) {
            return predecessor[node];
        };

        inline const Nodes &get_instructions() {
            return instructions;
        };

        inline const Vars &get_vars() {
            return vars;
        };

        inline bool is_dirty() {
            return dirty_flag;
        }

        inline void set_dirty_flag(bool new_state) {
            dirty_flag = new_state;
        }

        inline void add_instruction(Node inst) {
            instructions.push_back(inst);
        };

        inline Node get_fun_def(Node fun_call) {
            return fun_call_to_def[fun_call];
        };

        inline NodeSet get_fun_calls_from_def(Node fun_def) {
            return fun_def_to_calls[fun_def];
        };

        // The main function if there is one, otherwise every function
        inline const Nodes &get_program_entries() {
            return program_entries;
        };

        inline const Nodes &get_program_exits() {
            return program_exits;
        };

        inline bool is_program_entry(const Node &node) {
            return std::find(program_entries.begin(), program_entries.end(), node) !=
                program_entries.end();
        };

        void set_functions_calls(
            std::shared_ptr<Nodes> fun_defs,
            std::shared_ptr<NodeSet> fun_calls);

        void add_var(Node ident);

        void add_edge(const Node &u, const Node &v);
        void add_edge(const Node &u, const NodeSet &v);
        void add_edge(const NodeSet &u, const Node &v);

        void log_predecessors_and_successors();
        void log_instructions();
        void log_variables();
        void log_functions();

      private:
        Nodes program_entries;
        Nodes program_exits;
        Nodes instructions;
        Vars vars;
        bool dirty_flag;
        NodeMap<Node> fun_call_to_def; // Maps fun calls to their declarations
        NodeMap<NodeSet> fun_def_to_calls; // Maps fun defs to their call sites
        NodeMap<NodeSet> predecessor;
        NodeMap<NodeSet> successor;

        void append_to_nodemap(
            NodeMap<NodeSet> &map, const Node &key, const Node &value);
        void append_to_nodemap(
            NodeMap<NodeSet> &map, const Node &key, const NodeSet &values);
        void append_to_nodemap(
            NodeMap<NodeSet> &map, const NodeSet &nodes, const Node &prev);
    };
}
