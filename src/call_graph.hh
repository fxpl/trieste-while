#pragma once
#include "internal.hh"
#include "utils.hh"

namespace whilelang {
    using Vertex = std::string;
    using Vertices = std::set<std::string>;

    // Strongly connected component
    class SCC {
      public:
        Vertex root;
        Vertices nodes;

        SCC(Vertex new_root) {
            this->root = new_root;
            this->nodes = Vertices();
        }
    };

    class CallGraph {
      public:
        Vertices vertices;
        std::map<Vertex, Vertices> edges;
        std::vector<SCC> SCCs;
        Vertices non_inline_funs;

        CallGraph() {
            this->vertices = Vertices();
            this->edges = std::map<Vertex, Vertices>();
            this->SCCs = std::vector<SCC>();
        }

        void add_edge(const Node &surronding, const Node &fun_call) {
            // Add them as vertices first;
            this->vertices.insert(get_identifier(surronding));
            this->vertices.insert(get_identifier(fun_call));

            auto caller = get_identifier(surronding);
            auto callee = get_identifier(fun_call);
            auto res = this->edges.find(caller);

            if (res == this->edges.end()) {
                this->edges.insert({caller, {callee}});
            } else {
                res->second.insert(callee);
            }
        };

        void calculate_inlineable_funs() {
            this->SCC_kosaraju();

            // Handles mutual recursiveness
            for (auto &SCC : this->SCCs) {
                if (SCC.nodes.size() > 1) {
                    for (auto &vertex : SCC.nodes) {
                        this->non_inline_funs.insert(vertex);
                    }
                }
            }

            // Handles normal recursiveness
            for (auto vertex : this->vertices) {
                if (this->edges[vertex].contains(vertex)) {
                    this->non_inline_funs.insert(vertex);
                }
            }
        }

        bool can_be_inlined(const Vertex &fun) {
            return !non_inline_funs.contains(fun);
        };

        void log_functions_to_inline() {
            std::stringstream str_builder;
            str_builder << "The functions not allowed to be inlined are:\n";

            for (auto &fun : this->non_inline_funs) {
                str_builder << fun << ", ";
            }

            logging::Debug() << str_builder.str();
        }

        void log_graph() {
            for (Vertex caller : this->vertices) {
                logging::Debug() << "Function: " << caller
                                 << " calls the following functions:";
                for (Vertex callee : this->edges[caller]) {
                    logging::Debug() << "\t" << callee;
                }
            }
        }

        void log_SCCs() {
            std::stringstream str_builder;
            for (auto &SCC : this->SCCs) {
                str_builder << "\nThis SCC has nodes: \n";
                for (auto &u : SCC.nodes) {
                    str_builder << u << ", ";
                }
            }
            logging::Debug() << str_builder.str();
        }

      private:
        void add_edge(const Vertex &caller, const Vertex &callee) {
            auto res = this->edges.find(caller);

            if (res == this->edges.end()) {
                this->edges.insert({caller, {callee}});
            } else {
                res->second.insert(callee);
            }
        };

        CallGraph create_transposed_graph() const {
            auto transposed_graph = CallGraph();
            transposed_graph.vertices = this->vertices;

            for (auto [u, nodes] : this->edges) {
                for (Vertex v : nodes) {
                    transposed_graph.add_edge(v, u);
                }
            }
            return transposed_graph;
        }

        void SCC_kosaraju() {
            auto visited = std::map<Vertex, bool>();
            auto stack = std::deque<Vertex>();
            auto belongs_to_scc = std::map<Vertex, bool>();

            // Mark all nodes as unvisited
            for (const Vertex &u : this->vertices) {
                visited.insert({u, false});
                belongs_to_scc.insert({u, false});
            }

            for (auto &[vertex, _] : visited) {
                this->visit(vertex, visited, stack);
            }

            CallGraph transposed_graph = this->create_transposed_graph();

            for (auto &u : stack) {
                transposed_graph.assign(u, u, belongs_to_scc, this->SCCs);
            }
        }

        void visit(
            const Vertex &u,
            std::map<Vertex, bool> &visited,
            std::deque<Vertex> &stack) {
            if (!visited[u]) {
                visited[u] = true;
                for (auto &neighbour : this->edges[u]) {
                    this->visit(neighbour, visited, stack);
                }
                stack.push_front(u);
            }
        }

        void assign(
            const Vertex &u,
            const Vertex &root,
            std::map<Vertex, bool> &belongs_to_scc,
            std::vector<SCC> &SCC_list) {
            if (!belongs_to_scc[u]) {
                for (auto &SCC : SCC_list) {
                    if (root == SCC.root) {
                        SCC.nodes.insert(u);
                        belongs_to_scc[u] = true;
                    }
                }
                if (!belongs_to_scc[u]) {
                    auto new_scc = SCC(root);
                    new_scc.nodes.insert(u);
                    SCC_list.push_back(new_scc);
                    belongs_to_scc[u] = true;
                }
                for (auto &neighbour : this->edges[u]) {
                    assign(neighbour, root, belongs_to_scc, SCC_list);
                }
            }
        }
    };

}
