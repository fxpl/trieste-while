#pragma once
#include <CLI/CLI.hpp>

#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <tuple>
#include <vector>

namespace whilelang {
    // Options for fuzz testing, mirroring those of Trieste's Driver
    struct TestOptions {
        uint32_t seed_count = 100;
        uint32_t seed = std::random_device()();
        std::string start_pass;
        std::string end_pass;
        size_t max_depth = 10;
        bool failfast = false;
        bool sequence = false;
        bool size_stats = false;
        std::optional<size_t> max_retries = std::nullopt;
        bool bound_vars = true;
        std::vector<std::tuple<std::string, size_t>> weights;
        std::vector<std::filesystem::path> sample_files;
        size_t sampling_level = 1;
        size_t sampling_frequency = 50;
    };

    void configure_test_options(CLI::App &test, TestOptions &options);

    int run_tests(TestOptions &options);
}
