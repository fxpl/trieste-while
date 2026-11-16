#include "test.hh"

#include "internal.hh"
#include "lang.hh"
#include "utils.hh"

#include <csignal>
#include <cstring>
#include <trieste/fuzzer.h>
#include <unistd.h>

namespace whilelang {
    using namespace trieste;

    namespace {
        // The component being tested, reported if the process crashes
        const char *current_component = nullptr;

        void report_crash(int sig) {
            if (current_component) {
                const char msg[] = "\nCrashed while testing component: ";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                write(STDERR_FILENO, current_component, strlen(current_component));
                write(STDERR_FILENO, "\n", 1);
            }
            signal(sig, SIG_DFL);
            raise(sig);
        }

        // Never allowed by any well-formedness, so that adding it to a tree
        // makes the Fuzzer report a failure (see ADR 0002)
        inline const auto AnalysisFailure =
            TokenDef("while-analysis_failure", flag::print);

        // A component is tested as a single pass that runs each of its passes
        // in turn on the same tree, stopping at the first that reports
        // errors. This keeps the auxiliary structures built by earlier passes
        // in sync with the tree that later passes see.
        //
        // Invalid input is reported as an error, which the Fuzzer counts as
        // benign, while any other exception is reported as a failure.
        PassDef component(const std::string &name, std::vector<Pass> passes) {
            auto component_name = std::make_shared<std::string>(name);
            PassDef component = {name, passes.back()->wf(), dir::once, {}};

            component.pre([=](Node ast) {
                current_component = component_name->c_str();

                size_t changes = 0;
                try {
                    for (auto &pass : passes) {
                        auto [_, count, pass_changes] = pass->run(ast);
                        changes += pass_changes;

                        Nodes errors;
                        ast->get_errors(errors);
                        if (!errors.empty())
                            break;
                    }
                } catch (const InvalidInput &e) {
                    ast << (Error << (ErrorMsg ^ e.what()));
                } catch (const std::exception &e) {
                    ast << (AnalysisFailure ^ e.what());
                }

                current_component = nullptr;
                return changes;
            });

            return component;
        }

        std::vector<Pass> control_flow(std::shared_ptr<ControlFlow> cfg) {
            return {
                gather_functions(cfg),
                gather_instructions(cfg),
                gather_flow_graph(cfg),
            };
        }

        // A component that runs a pass after building the control flow graph
        template<typename F>
        PassDef with_control_flow(const std::string &name, F make_pass) {
            auto cfg = std::make_shared<ControlFlow>();
            auto passes = control_flow(cfg);
            passes.push_back(make_pass(cfg));
            return component(name, passes);
        }

        // The components involved in static analysis (-O1)
        std::vector<Pass> analysis_components() {
            return {
                component(
                    "control_flow",
                    control_flow(std::make_shared<ControlFlow>())),
                with_control_flow("constant_propagation", constant_propagation),
                with_control_flow("liveness", liveness),
                with_control_flow("constant_folding", constant_folding),
                with_control_flow("dead_code_elimination", dead_code_elimination),
                component("dead_code_cleanup", {dead_code_cleanup()}),
            };
        }

        // The components involved in inlining (-O2)
        std::vector<Pass> inlining_components() {
            auto call_graph = std::make_shared<CallGraph>();
            return {
                component(
                    "call_graph",
                    {build_call_graph(std::make_shared<CallGraph>())}),
                component(
                    "inlining",
                    {build_call_graph(call_graph), inlining(call_graph)}),
            };
        }

        // The compiler's pipeline with the components of an optimisation
        // level in place of its optimisations. Unlike the compiler, each
        // component runs once rather than until nothing changes.
        Reader pipeline(int opt_level) {
            auto vars_map = std::make_shared<std::map<std::string, std::string>>();
            std::vector<Pass> components;
            if (opt_level >= 2)
                components = inlining_components();
            if (opt_level >= 1) {
                auto analysis = analysis_components();
                components.insert(components.end(), analysis.begin(), analysis.end());
            }

            return (whilelang::reader(vars_map) >>=
                    Rewriter("optimisations", components, normalization_wf)) >>=
                compiler();
        }

        bool contains(const std::vector<std::string> &names, const std::string &name) {
            return std::find(names.begin(), names.end(), name) != names.end();
        }

        Fuzzer &configure(
            Fuzzer &fuzzer,
            const TestOptions &options,
            const wf::TokenWeights &weights,
            Nodes &sample_trees) {
            return fuzzer
                .max_retries(options.max_retries.value_or(options.seed_count * 2))
                .max_depth(options.max_depth)
                .failfast(options.failfast)
                .seed_count(options.seed_count)
                .start_seed(options.seed)
                .bound_vars(options.bound_vars)
                .token_weights(wf::TokenWeights(weights))
                .size_stats(options.size_stats)
                .sample_trees(sample_trees)
                .sampling_level(options.sampling_level)
                .sampling_enabled(!sample_trees.empty())
                .sampling_frequency(options.sampling_frequency);
        }
    }

    void configure_test_options(CLI::App &test, TestOptions &options) {
        // Every pass and component, whatever the optimisation level
        auto pass_names = pipeline(2).pass_names();
        auto targets = std::vector(pass_names.begin() + 1, pass_names.end());

        test.add_option(
                "-O,--opt-level",
                options.opt_level,
                "Also test the components of this optimisation level: "
                "0 (none), 1 (static analysis) or 2 (static analysis and "
                "inlining).")
            ->check(CLI::Range(0, 2));

        test.add_option(
            "-c,--seed_count", options.seed_count, "Number of iterations per pass");
        test.add_option("-s,--seed", options.seed, "Random seed for testing");

        test.add_option(
                "start",
                options.start_pass,
                "Start at this pass or component.")
            ->transform(CLI::IsMember(targets));
        test.add_option(
                "end",
                options.end_pass,
                "End at this pass or component.")
            ->transform(CLI::IsMember(targets));

        test.add_option(
            "-d,--max_depth", options.max_depth, "Maximum depth of AST to test");
        test.add_flag("-f,--failfast", options.failfast, "Stop on first failure");
        test.add_flag(
            "--sequence",
            options.sequence,
            "Test sequence of passes on generated trees");
        test.add_flag(
            "--size_stats",
            options.size_stats,
            "Collect size statistics for ASTs (defaults to log level Info)");
        test.add_option(
            "-r,--max_retries",
            options.max_retries,
            "Maximum number of retries for finding unique trees");
        test.add_option(
            "--gen_bound",
            options.bound_vars,
            "Generate bound variable names if possible");
        test.add_option(
            "--weights",
            options.weights,
            "Token weight pairs for generation. Use: --weights <token> <weight> "
            "[<token> <weight> ...]");

        test.add_option(
            "--samples",
            options.sample_files,
            "Files to extract sample nodes from for fuzz testing");
        test.add_option(
            "--sampling-level",
            options.sampling_level,
            "Level of sampling to use for selecting sample nodes");
        test.add_option(
                "--sampling-frequency",
                options.sampling_frequency,
                "Frequency of using sampled nodes over randomly generated nodes "
                "during testing (0-100)")
            ->check(CLI::Range(0, 100));
    }

    int run_tests(TestOptions &options) {
        auto reader = pipeline(options.opt_level);
        auto pass_names = reader.pass_names();

        wf::TokenWeights weights;
        for (auto [name, weight] : options.weights) {
            auto token = detail::find_token(name);
            if (token == Invalid) {
                logging::Error() << "Unknown token in weights: " << name << std::endl;
                return 1;
            }
            weights[token] = weight;
        }

        for (auto &name : {options.start_pass, options.end_pass}) {
            if (!name.empty() && !contains(pass_names, name)) {
                logging::Error() << "Testing " << name
                                 << " requires a higher optimisation level"
                                 << std::endl;
                return 1;
            }
        }

        // Sort the sample files so they are loaded in a deterministic order,
        // making fuzzing reproducible
        std::sort(options.sample_files.begin(), options.sample_files.end());

        Nodes sample_trees;
        for (auto const &sample_path : options.sample_files) {
            if (!(std::filesystem::is_regular_file(sample_path) ||
                  std::filesystem::is_symlink(sample_path))) {
                logging::Output() << "Not sampling " << sample_path << std::endl;
                continue;
            }

            Node sample = reader.parser().parse(sample_path);
            if (!is_valid_sample(reader.parser(), sample)) {
                logging::Warn() << "Skipping invalid sample " << sample_path
                                << std::endl;
                continue;
            }
            sample_trees.push_back(sample);
        }

        std::string start = options.start_pass;
        std::string end = options.end_pass;
        if (start.empty()) {
            start = pass_names.at(1);
            end = pass_names.back();
        } else if (end.empty()) {
            end = options.sequence ? pass_names.back() : start;
        }

        for (auto sig : {SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL})
            signal(sig, report_crash);

        logging::Output() << "Testing x" << options.seed_count
                          << ", seed: " << options.seed << std::endl;

        Fuzzer fuzzer(reader);
        configure(fuzzer, options, weights, sample_trees)
            .start_index(reader.pass_index(start))
            .end_index(reader.pass_index(end))
            .test_sequence(options.sequence);
        return fuzzer.test();
    }
}
