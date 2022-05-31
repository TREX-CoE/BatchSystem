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

void runCommand(const std::string& rootdir, Result& res, const cw::batch::Cmd& cmd) {
    std::string path = rootdir + cmd.cmd;
    for (const auto& a : cmd.args) path += " " + a;
    std::cout << "! " << path << std::endl;
    std::ifstream f(path);
    if (f) {
        std::stringstream stream;
        stream << f.rdbuf();
        res.out = stream.str();
        res.exit = 0;
    } else {
        res.exit = 1;
    }
}

}

TEST_CASE("Test system", "[full]") {
    SECTION("Batch system") {

        std::unique_ptr<BatchInterface> batch = create_batch(System::Slurm, [](Result& res, const Cmd& cmd){ runCommand(TESTDATA_DIR+"/1/", res, cmd); });

        std::vector<Job> jobs;
        auto f = batch->getJobs(std::vector<std::string>{});
        while(!f([&](Job j){ jobs.push_back(j); return true; }));

        CHECK(jobs.size() == 5);
        CHECK(jobs[0].id.get()=="4");

        std::string out;
        for (const auto& j : jobs) out += json::serialize(j) + "\n";
        std::cout << out << std::endl;
    }
}

