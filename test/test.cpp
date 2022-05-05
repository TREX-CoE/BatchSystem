#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <functional>

#include "batchsystem/batchsystem.h"
#include "batchsystem/slurm.h"
#include "batchsystem/json.h"
#include "batchsystem/internal/byteParse.h"

using namespace std::placeholders;

const std::string TESTDATA_DIR = std::string(TEST_DIR);

namespace {

int runCommand(const std::string& rootdir, std::stringstream& stream, const std::string& cmd, const std::vector<std::string>& args) {
    std::string path = rootdir + cmd;
    for (const auto& a : args) path += " " + a;
    std::ifstream f(path);
    if (f) {
        stream << f.rdbuf();
        return 0;
    } else {
        return 1;
    }
}

}

TEST_CASE("si_byte_parse") {
    using namespace cw::batch::internal;
    uint64_t bytes = 0;
    REQUIRE(si_byte_parse("1000kb", bytes));
    REQUIRE(bytes==1000000);
    REQUIRE(si_byte_parse("1000", bytes));
    REQUIRE(bytes==1000);
    REQUIRE(si_byte_parse("1000mb", bytes));
    REQUIRE(bytes==1000000000);
    REQUIRE(!si_byte_parse("1000mib", bytes));
}

TEST_CASE("Test system") {
    SECTION("Batch system") {
        using namespace cw::batch;
        BatchSystem batchSystem;

        slurm::create_batch(batchSystem, std::bind(runCommand, TESTDATA_DIR+"/1/", _1, _2, _3));

        std::vector<Job> jobs;
        batchSystem.getJobs([&](Job j){ jobs.push_back(j); return true; });
        CHECK(jobs.size() == 5);
        CHECK(jobs[0].id.get()=="4");

        std::vector<std::string> lines;
        for (const auto& j : jobs) lines.push_back(json::serialize(j));

        for (const auto& l : lines) {
            std::cout << l << std::endl;
        }
    }
}
