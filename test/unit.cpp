#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <functional>

#include "batchsystem/batchsystem.h"
#include "batchsystem/slurm.h"
#include "batchsystem/json.h"
#include "batchsystem/slurmBatch.h"
#include "batchsystem/pbsBatch.h"


using namespace cw::batch;

namespace {

const std::string TESTDATA_DIR = std::string(TEST_DIR) + "/unit/";

std::string readFile(const std::string& path) {
    std::ifstream f(TESTDATA_DIR + path);
    if (f) {
        std::stringstream stream;
        stream << f.rdbuf();
        return stream.str();
    } else {
        return "";
    }
}

}

TEST_CASE("Slurm jobs", "[slurm][jobs]") {
    std::vector<Job> jobs;
    auto insert = [&](Job j){ jobs.push_back(j); return true; };

    SECTION("sacct") {
        auto file = GENERATE("sacctAll", "sacctAllEmptyLines");
        slurm::SlurmBatch::parseJobsSacct(readFile(file), insert);
        REQUIRE(jobs.size() == 1);
        REQUIRE(json::serialize(jobs[0]) == R"EOF({"id":"4","name":"jobscript.sh","userId":1000,"groupId":1000,"queue":"debug","priority":4294901757,"state":"unknown","workdir":"/home/kevth/batch_test","comment":{"reason":"","user":"","admin":"","system":""},"time":{"submit":1650547862},"used":{"cpus":1},"requested":{"nodes":1,"general":{"billing":"1","cpu":"1","node":"1"}},"cpusFromNode":{},"variableList":{}})EOF");
    }

    SECTION("no jobs") {
        slurm::SlurmBatch::parseJobsLegacy("No jobs in the system", insert);
        REQUIRE(jobs.size() == 0);
    }
}


TEST_CASE("Slurm nodes", "[slurm][nodes]") {
    std::vector<Node> nodes;
    auto insert = [&](Node n){nodes.push_back(n); return true;};

    SECTION("partial unknown") {
        auto file = GENERATE("scontrolShowNodePartiallyUnknownBefore", "scontrolShowNodePartiallyUnknownAfter");
        slurm::SlurmBatch::parseNodes(readFile(file), insert);
        REQUIRE(nodes.size() == 1);
        REQUIRE(json::serialize(nodes[0]) == R"EOF({"name":"gmz01","state":"online","rawState":"IDLE","cpus":128,"cpusReserved":0,"cpusOfJobs":{}})EOF");
    }
    SECTION("unknown") {
        slurm::SlurmBatch::parseNodes(readFile("scontrolShowNodeUnknown"), insert);
        REQUIRE(nodes.size() == 0);
    }
}

TEST_CASE("Pbs nodes", "[pbs][nodes]") {
    std::vector<Node> nodes;
    auto insert = [&](Node n){nodes.push_back(n); return true;};

    SECTION("unknown node") {
        pbs::PbsBatch::parseNodes(readFile("pbsnodesxUnknown"), insert);
        REQUIRE(nodes.size() == 0);
    }

    SECTION("successfull") {
        pbs::PbsBatch::parseNodes(readFile("pbsnodesx"), insert);
        REQUIRE(nodes.size() == 1);
        REQUIRE(json::serialize(nodes[0]) == R"EOF({"name":"ubuntu","nameFull":"ubuntu","state":"offline","rawState":"down","comment":"testnote","cpus":1,"cpusOfJobs":{}})EOF");

    }
}