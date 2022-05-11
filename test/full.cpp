#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <functional>

#include "batchsystem/batchsystem.h"
#include "batchsystem/slurm.h"
#include "batchsystem/json.h"
#include "batchsystem/slurmBatch.h"
#include "batchsystem/pbsBatch.h"

using namespace std::placeholders;

using namespace cw::batch;

namespace {

const std::string TESTDATA_DIR = std::string(TEST_DIR) + "/full";

int runCommand(const std::string& rootdir, std::string& out, const cw::batch::CmdOptions& opts) {
    std::string path = rootdir + opts.cmd;
    for (const auto& a : opts.args) path += " " + a;
    std::ifstream f(path);
    if (f) {
        std::stringstream stream;
        stream << f.rdbuf();
        out = stream.str();
        return 0;
    } else {
        return 1;
    }
}

}

TEST_CASE("Test system", "[full]") {
    SECTION("Batch system") {
        BatchSystem batchSystem;

        slurm::create_batch(batchSystem, std::bind(runCommand, TESTDATA_DIR+"/1/", _1, _2));

        std::vector<Job> jobs;
        batchSystem.getJobs([&](Job j){ jobs.push_back(j); return true; });
        CHECK(jobs.size() == 5);
        CHECK(jobs[0].id.get()=="4");

        std::string out;
        for (const auto& j : jobs) out += json::serialize(j) + "\n";
        //std::cout << out << std::endl;
    }
}