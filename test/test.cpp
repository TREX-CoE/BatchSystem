#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <functional>

#include "batchsystem/batchsystem.h"
#include "batchsystem/slurm.h"
#include "batchsystem/json.h"
#include "batchsystem/internal/byteParse.h"
#include "batchsystem/slurmBatch.h"

using namespace std::placeholders;

const std::string TESTDATA_DIR = std::string(TEST_DIR);

namespace {

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

        slurm::create_batch(batchSystem, std::bind(runCommand, TESTDATA_DIR+"/1/", _1, _2));

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


const std::string sacctOutput = R"EOF(
Account|AdminComment|AllocCPUS|AllocNodes|AllocTRES|AssocID|AveCPU|AveCPUFreq|AveDiskRead|AveDiskWrite|AvePages|AveRSS|AveVMSize|BlockID|Cluster|Comment|Constraints|ConsumedEnergy|ConsumedEnergyRaw|CPUTime|CPUTimeRAW|DBIndex|DerivedExitCode|Elapsed|ElapsedRaw|Eligible|End|ExitCode|Flags|GID|Group|JobID|JobIDRaw|JobName|Layout|MaxDiskRead|MaxDiskReadNode|MaxDiskReadTask|MaxDiskWrite|MaxDiskWriteNode|MaxDiskWriteTask|MaxPages|MaxPagesNode|MaxPagesTask|MaxRSS|MaxRSSNode|MaxRSSTask|MaxVMSize|MaxVMSizeNode|MaxVMSizeTask|McsLabel|MinCPU|MinCPUNode|MinCPUTask|NCPUS|NNodes|NodeList|NTasks|Priority|Partition|QOS|QOSRAW|Reason|ReqCPUFreq|ReqCPUFreqMin|ReqCPUFreqMax|ReqCPUFreqGov|ReqCPUS|ReqMem|ReqNodes|ReqTRES|Reservation|ReservationId|Reserved|ResvCPU|ResvCPURAW|Start|State|Submit|Suspended|SystemCPU|SystemComment|Timelimit|TimelimitRaw|TotalCPU|TRESUsageInAve|TRESUsageInMax|TRESUsageInMaxNode|TRESUsageInMaxTask|TRESUsageInMin|TRESUsageInMinNode|TRESUsageInMinTask|TRESUsageInTot|TRESUsageOutAve|TRESUsageOutMax|TRESUsageOutMaxNode|TRESUsageOutMaxTask|TRESUsageOutMin|TRESUsageOutMinNode|TRESUsageOutMinTask|TRESUsageOutTot|UID|User|UserCPU|WCKey|WCKeyID|WorkDir
||1|0||0|||||||||cluster|||0|0|00:00:00|0|3|0:0|00:00:00|0|2022-04-21T14:31:02|Unknown|0:0||1000|kevth|4|4|jobscript.sh|||||||||||||||||||||1|1|None assigned||4294901757|debug|normal|1|ReqNodeNotAvail|Unknown|Unknown|Unknown|Unknown|1|0n|1|billing=1,cpu=1,node=1|||6-23:16:06|6-23:16:06|602166|Unknown|PENDING|2022-04-21T14:31:02|00:00:00|00:00:00||Partition_Limit|Partition_Limit|00:00:00|||||||||||||||||1000|kevth|00:00:00||0|/home/kevth/batch_test
)EOF";
TEST_CASE("Slurm") {
    using namespace cw::batch;
    std::vector<Job> jobs;
    slurm::SlurmBatch::parseJobsSacct(sacctOutput, [&](Job j){ jobs.push_back(j); return true; });
    REQUIRE(jobs.size() == 1);
    REQUIRE(json::serialize(jobs[0]) == R"EOF({"id":"4","name":"jobscript.sh","userId":1000,"groupId":1000,"queue":"debug","priority":4294901757,"state":"unknown","workdir":"/home/kevth/batch_test","comment":{"reason":"","user":"","admin":"","system":""},"time":{"submit":1650547862},"used":{"cpus":1},"requested":{"nodes":1,"general":{"billing":"1","cpu":"1","node":"1"}},"cpusFromNode":{},"variableList":{}})EOF");
}