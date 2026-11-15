#include "lang.hh"
#include "analyses/utils.hh"
#include "test.hh"

#include <CLI/CLI.hpp>
#include <trieste/trieste.h>
#include <vbcc.h>

int main(int argc, char const *argv[]) {
    CLI::App app;
    app.require_subcommand(0, 1);

    std::filesystem::path input_path;
    app.add_option("input", input_path, "Path to the input file ");

    std::string log_level;
    auto log_description =
        "Set the log level (None, Error, Output, Warn, Info, Debug, Trace).";
    app.add_option("-l,--log", log_level, log_description)
        ->check(trieste::logging::set_log_level_from_string);

    bool run = false;
    int opt_level = 0;
    bool run_gather_stats = false;
    bool run_mermaid = false;
    app.add_flag("-r,--run", run, "Run the program (prompting inputs).");
    app.add_option(
           "-O,--opt-level",
           opt_level,
           "Optimisation level: 0 (none), 1 (static analysis), "
           "2 (static analysis and inlining).")
        ->check(CLI::Range(0, 2));

    app.add_flag(
        "-p, --print-stats",
        run_gather_stats,
        "Runs the gather stats pass displaying total instructions and "
        "variables after normalization. Needed for the ./stats.sh script.");
    app.add_flag(
        "-m, --mermaid",
        run_mermaid,
        "Runs the mermaid pass which parses the final AST into mermaid "
        "format ");

    std::filesystem::path output_path = "";
    app.add_option(
        "-o,--output",
        output_path,
        "Output file for the compiled program. If not specified, "
        "the output will be the input file name with .trieste extension.");

    auto test = app.add_subcommand("test", "Fuzz test the compiler passes");
    whilelang::TestOptions test_options;
    whilelang::configure_test_options(*test, test_options);
    test->add_option("-l,--log", log_level, log_description)
        ->check(trieste::logging::set_log_level_from_string);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    if (*test) {
        if (test_options.size_stats && log_level.empty())
            trieste::logging::set_log_level_from_string("Info");

        return whilelang::run_tests(test_options);
    }

    if (input_path.empty()) {
        std::cerr << "An input file is required" << std::endl
                  << "Run with --help for more information." << std::endl;
        return 1;
    }

    auto vars_map = std::make_shared<std::map<std::string, std::string>>();
    auto reader = whilelang::reader(vars_map, run_gather_stats, run_mermaid)
                      .file(input_path);

    try {
        auto program_empty = [](trieste::Node ast) -> bool {
            return ast->front()->empty();
        };

        auto result = reader.read();

        bool run_static_analysis = opt_level >= 1;
        bool run_inlining = opt_level >= 2;

        if (run_inlining) {
            result = result >> whilelang::inlining_rewriter();
        }

        if (run_static_analysis) {
            do {
                result = result >>
                    whilelang::optimization_analysis();
            } while (result.ok && result.total_changes > 0 &&
                     !program_empty(result.ast));
        }

        trieste::Rewriter compiler = whilelang::compiler();
        result = result >> compiler;

        trieste::logging::Debug() << "AST after compilation: " << std::endl
                                  << result.ast;

        // If any result above was not ok it will carry through to here
        if (!result.ok) {
            trieste::logging::Error err;
            result.print_errors(err);
            trieste::logging::Debug() << result.ast;
            return 1;
        }
        whilelang::log_var_map(vars_map);

        if (output_path.empty())
            output_path = input_path.stem().replace_extension(".trieste");

        std::ofstream f(output_path, std::ios::binary | std::ios::out);
        if (f) {
            // Write the AST to the output file.
            f << "vbcc" << std::endl << "VIR" << std::endl << result.ast;
        } else {
            trieste::logging::Error() << "Could not open " << output_path
                                      << " for writing." << std::endl;
            return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << "Program failed with an exception: " << e.what()
                  << std::endl;
    }

    return 0;
}
