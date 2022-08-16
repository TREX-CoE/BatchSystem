#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <iostream>
#include <sstream>
#include <fstream>
#include <functional>

#include "batchsystem/batchsystem.h"
#include "batchsystem/json.h"
#include "batchsystem/slurm.h"
#include "batchsystem/pbs.h"
#include "batchsystem/error.h"


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

TEST_CASE("Error code", "[error]") {
    REQUIRE(static_cast<std::error_code>(cw::batch::pbs::error::qsig_s_resume_failed) == batch_condition::command_failed);
    REQUIRE(static_cast<std::error_code>(cw::batch::pbs::error::qsig_s_resume_failed) != batch_condition::invalid_argument);
}


TEST_CASE("Slurm jobs", "[slurm][jobs]") {
    std::vector<Job> jobs;
    SECTION("sacct") {
        auto file = GENERATE("sacctAll", "sacctAllEmptyLines");
        slurm::Slurm::parseJobsSacct(readFile(file), jobs);
        REQUIRE(jobs.size() == 1);
        REQUIRE(json::serialize(jobs[0]) == R"EOF({"id":"4","name":"jobscript.sh","userId":1000,"groupId":1000,"queue":"debug","priority":4294901757,"state":"unknown","workdir":"/home/kevth/batch_test","comment":{"reason":"","user":"","admin":"","system":""},"time":{"submit":1650547862},"used":{"cpus":1},"requested":{"nodes":1,"general":{"billing":"1","cpu":"1","node":"1"}},"cpusFromNode":{},"variableList":{}})EOF");
    }

    SECTION("no jobs") {
        slurm::Slurm::parseJobsLegacy("No jobs in the system", jobs);
        REQUIRE(jobs.size() == 0);
    }
}


TEST_CASE("Slurm nodes", "[slurm][nodes]") {
    std::vector<Node> nodes;

    SECTION("partial unknown") {
        auto file = GENERATE("scontrolShowNodePartiallyUnknownBefore", "scontrolShowNodePartiallyUnknownAfter");
        slurm::Slurm::parseNodes(readFile(file), nodes);
        REQUIRE(nodes.size() == 1);
        REQUIRE(json::serialize(nodes[0]) == R"EOF({"name":"gmz01","state":"online","rawState":"IDLE","cpus":128,"cpusReserved":0,"cpusOfJobs":{}})EOF");
    }
    SECTION("unknown") {
        slurm::Slurm::parseNodes(readFile("scontrolShowNodeUnknown"), nodes);
        REQUIRE(nodes.size() == 0);
    }
}

TEST_CASE("Pbs nodes", "[pbs][nodes]") {
    std::vector<Node> nodes;

    SECTION("unknown node") {
        pbs::Pbs::parseNodes(readFile("pbsnodesxUnknown"), nodes);
        REQUIRE(nodes.size() == 0);
    }

    SECTION("successfull") {
        pbs::Pbs::parseNodes(readFile("pbsnodesx"), nodes);
        REQUIRE(nodes.size() == 1);
        REQUIRE(json::serialize(nodes[0]) == R"EOF({"name":"ubuntu","nameFull":"ubuntu","state":"offline","rawState":"down","comment":"testnote","cpus":1,"cpusOfJobs":{}})EOF");

    }
}

TEST_CASE("Pbs queues", "[pbs][queues]") {
    std::vector<Queue> queues;

    SECTION("successfull") {
        pbs::Pbs::parseQueues(readFile("qstatQf"), queues);
        REQUIRE(queues.size() == 1);
        REQUIRE(json::serialize(queues[0]) == R"EOF({"name":"batch","priority":4294967295,"state":"open","rawState":"enabled=True,started=True","jobs":{"total":0},"modifyTime":1649429720,"nodes":[]})EOF");

    }
}