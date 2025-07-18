#include "package_manager.h"

const RepoMap& get_builtin_repos() {
    static const RepoMap repos = {
        {"fmt",    "https://github.com/fmtlib/fmt.git"},
        {"spdlog", "https://github.com/gabime/spdlog.git"},
        {"catch2", "https://github.com/catchorg/Catch2.git"},
        {"googletest", "https://github.com/google/googletest.git"},
        {"nlohmann_json", "https://github.com/nlohmann/json.git"},
        {"cpr",    "https://github.com/libcpr/cpr.git"},
        {"gtest",  "https://github.com/google/googletest.git"},
        {"tbb",    "https://github.com/oneapi-src/oneTBB.git"},
        {"eigen",  "https://gitlab.com/libeigen/eigen.git"},
        {"boost",  "https://github.com/boostorg/boost.git"}
    };
    return repos;
} 