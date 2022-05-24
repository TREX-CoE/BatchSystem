#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <functional>

#include "batchsystem/batchsystem.h"
#include "batchsystem/json.h"
#include "batchsystem/factory.h"

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

        std::unique_ptr<BatchInterface> batch = create_batch(System::Slurm, [](std::string& out, const CmdOptions& opts){ return runCommand(TESTDATA_DIR+"/1/", out, opts); });

        std::vector<Job> jobs;
        batch->getJobs([&](Job j){ jobs.push_back(j); return true; });
        CHECK(jobs.size() == 5);
        CHECK(jobs[0].id.get()=="4");

        std::string out;
        for (const auto& j : jobs) out += json::serialize(j) + "\n";
        //std::cout << out << std::endl;
    }
}