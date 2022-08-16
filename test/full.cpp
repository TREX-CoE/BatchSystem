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

void runCommand(const std::string& rootdir, cw::batch::Cmd cmd, std::function<void(Result)> cb) {
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

TEST_CASE("BatchInfo", "[slurm]") {
    
    cw::batch::slurm::Slurm batch([](Cmd cmd, auto cb){ runCommand(TESTDATA_DIR+"/1/", cmd, cb); });
    batch.getBatchInfo([](BatchInfo info, auto ec){
        CHECK(!ec);
        CHECK((info.version.get() == "slurm-wlm 20.11.4"));
    });
}

TEST_CASE("Test system", "[full]") {
    SECTION("Batch system") {

        std::unique_ptr<BatchInterface> batch = create_batch(System::Slurm, [](Cmd cmd, auto cb){ runCommand(TESTDATA_DIR+"/1/", cmd, cb); });

        batch->getJobs(std::vector<std::string>{}, [](auto jobs, auto ec){
            CHECK(!ec);
            CHECK((jobs.size() == 5));
            CHECK((jobs[0].id.get()=="4"));
            std::string out;
            for (const auto& j : jobs) out += json::serialize(j) + "\n";
            std::cout << out << std::endl;
        });
    }
}

