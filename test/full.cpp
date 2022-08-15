#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <functional>

#include "batchsystem/batchsystem.h"
#include "batchsystem/json.h"
#include "batchsystem/factory.h"
#include "batchsystem/slurm.h"

using namespace std::placeholders;

using namespace cw::batch;

namespace {

const std::string TESTDATA_DIR = std::string(TEST_DIR) + "/full";

void runCommand(const std::string& rootdir, Result& res, const cw::batch::Cmd& cmd) {
    std::string path = rootdir + cmd.cmd;
    for (const auto& a : cmd.args) path += " " + a;
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

void runCommand2(const std::string& rootdir, cw::batch::Cmd cmd, std::function<void(Result)> cb) {
    std::string path = rootdir + cmd.cmd;
    for (const auto& a : cmd.args) path += " " + a;
    std::ifstream f(path);
    Result res;
    if (f) {
        std::stringstream stream;
        stream << f.rdbuf();
        res.out = stream.str();
        res.exit = 0;
    } else {
        res.exit = 1;
    }
    cb(res);
}

}

TEST_CASE("Test 1", "[slurm2]") {
    cw::batch::slurm::Slurm batch([](Result& res, const Cmd& cmd){ runCommand(TESTDATA_DIR+"/1/", res, cmd); });
    batch.set_cmd2([](Cmd cmd, auto cb){ runCommand2(TESTDATA_DIR+"/1/", cmd, cb); });
    batch.getBatchInfo([](BatchInfo info, auto ec){ std::cout << info.version.get() << std::endl; (void)ec; });
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

