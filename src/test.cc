#include "test.hh"

#include "lang.hh"

#include <trieste/fuzzer.h>

namespace whilelang {
    using namespace trieste;

    void configure_test_options(CLI::App &test, TestOptions &options) {
        auto vars_map = std::make_shared<std::map<std::string, std::string>>();
        auto pass_names = (reader(vars_map, false, false) >>= compiler()).pass_names();
        auto pass_names_no_parse =
            std::vector(pass_names.begin() + 1, pass_names.end());

        test.add_option(
            "-c,--seed_count", options.seed_count, "Number of iterations per pass");
        test.add_option("-s,--seed", options.seed, "Random seed for testing");

        test.add_option("start", options.start_pass, "Start at this pass.")
            ->transform(CLI::IsMember(pass_names_no_parse));
        test.add_option("end", options.end_pass, "End at this pass.")
            ->transform(CLI::IsMember(pass_names_no_parse));

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
        auto vars_map = std::make_shared<std::map<std::string, std::string>>();
        auto reader = whilelang::reader(vars_map, false, false) >>= whilelang::compiler();

        wf::TokenWeights weights;
        for (auto [name, weight] : options.weights) {
            auto token = detail::find_token(name);
            if (token == Invalid) {
                logging::Error() << "Unknown token in weights: " << name << std::endl;
                return 1;
            }
            weights[token] = weight;
        }

        auto pass_names = reader.pass_names();
        if (options.start_pass.empty()) {
            options.start_pass = pass_names.at(1);
            options.end_pass = pass_names.back();
        } else if (options.end_pass.empty()) {
            options.end_pass =
                options.sequence ? pass_names.back() : options.start_pass;
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

        logging::Output() << "Testing x" << options.seed_count
                          << ", seed: " << options.seed << std::endl;

        return Fuzzer(reader)
            .max_retries(options.max_retries.value_or(options.seed_count * 2))
            .max_depth(options.max_depth)
            .failfast(options.failfast)
            .seed_count(options.seed_count)
            .start_index(reader.pass_index(options.start_pass))
            .end_index(reader.pass_index(options.end_pass))
            .start_seed(options.seed)
            .bound_vars(options.bound_vars)
            .token_weights(std::move(weights))
            .test_sequence(options.sequence)
            .size_stats(options.size_stats)
            .sample_trees(sample_trees)
            .sampling_level(options.sampling_level)
            .sampling_enabled(!sample_trees.empty())
            .sampling_frequency(options.sampling_frequency)
            .test();
    }
}
