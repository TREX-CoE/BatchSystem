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
#include "batchsystem/pbsBatch.h"

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

        std::string out;
        for (const auto& j : jobs) out += json::serialize(j) + "\n";
        //std::cout << out << std::endl;
    }
}


const std::string sacctOutput = R"EOF(
Account|AdminComment|AllocCPUS|AllocNodes|AllocTRES|AssocID|AveCPU|AveCPUFreq|AveDiskRead|AveDiskWrite|AvePages|AveRSS|AveVMSize|BlockID|Cluster|Comment|Constraints|ConsumedEnergy|ConsumedEnergyRaw|CPUTime|CPUTimeRAW|DBIndex|DerivedExitCode|Elapsed|ElapsedRaw|Eligible|End|ExitCode|Flags|GID|Group|JobID|JobIDRaw|JobName|Layout|MaxDiskRead|MaxDiskReadNode|MaxDiskReadTask|MaxDiskWrite|MaxDiskWriteNode|MaxDiskWriteTask|MaxPages|MaxPagesNode|MaxPagesTask|MaxRSS|MaxRSSNode|MaxRSSTask|MaxVMSize|MaxVMSizeNode|MaxVMSizeTask|McsLabel|MinCPU|MinCPUNode|MinCPUTask|NCPUS|NNodes|NodeList|NTasks|Priority|Partition|QOS|QOSRAW|Reason|ReqCPUFreq|ReqCPUFreqMin|ReqCPUFreqMax|ReqCPUFreqGov|ReqCPUS|ReqMem|ReqNodes|ReqTRES|Reservation|ReservationId|Reserved|ResvCPU|ResvCPURAW|Start|State|Submit|Suspended|SystemCPU|SystemComment|Timelimit|TimelimitRaw|TotalCPU|TRESUsageInAve|TRESUsageInMax|TRESUsageInMaxNode|TRESUsageInMaxTask|TRESUsageInMin|TRESUsageInMinNode|TRESUsageInMinTask|TRESUsageInTot|TRESUsageOutAve|TRESUsageOutMax|TRESUsageOutMaxNode|TRESUsageOutMaxTask|TRESUsageOutMin|TRESUsageOutMinNode|TRESUsageOutMinTask|TRESUsageOutTot|UID|User|UserCPU|WCKey|WCKeyID|WorkDir
||1|0||0|||||||||cluster|||0|0|00:00:00|0|3|0:0|00:00:00|0|2022-04-21T14:31:02|Unknown|0:0||1000|kevth|4|4|jobscript.sh|||||||||||||||||||||1|1|None assigned||4294901757|debug|normal|1|ReqNodeNotAvail|Unknown|Unknown|Unknown|Unknown|1|0n|1|billing=1,cpu=1,node=1|||6-23:16:06|6-23:16:06|602166|Unknown|PENDING|2022-04-21T14:31:02|00:00:00|00:00:00||Partition_Limit|Partition_Limit|00:00:00|||||||||||||||||1000|kevth|00:00:00||0|/home/kevth/batch_test
)EOF";
TEST_CASE("Slurm jobs") {
    using namespace cw::batch;
    std::vector<Job> jobs;
    slurm::SlurmBatch::parseJobsSacct(sacctOutput, [&](Job j){ jobs.push_back(j); return true; });
    REQUIRE(jobs.size() == 1);
    REQUIRE(json::serialize(jobs[0]) == R"EOF({"id":"4","name":"jobscript.sh","userId":1000,"groupId":1000,"queue":"debug","priority":4294901757,"state":"unknown","workdir":"/home/kevth/batch_test","comment":{"reason":"","user":"","admin":"","system":""},"time":{"submit":1650547862},"used":{"cpus":1},"requested":{"nodes":1,"general":{"billing":"1","cpu":"1","node":"1"}},"cpusFromNode":{},"variableList":{}})EOF");
}

const std::string scontrolShowNodeFailedPartially = R"EOF(
NodeName=gmz01 Arch=x86_64 CoresPerSocket=8 
   CPUAlloc=0 CPUTot=128 CPULoad=0.00
   AvailableFeatures=(null)
   ActiveFeatures=(null)
   Gres=(null)
   NodeAddr=gmz01 NodeHostName=gmz01 Version=20.02.3
   OS=Linux 4.18.0-193.28.1.el8_2.x86_64 #1 SMP Thu Oct 22 00:20:22 UTC 2020 
   RealMemory=1 AllocMem=0 FreeMem=253909 Sockets=8 Boards=1
   State=IDLE ThreadsPerCore=2 TmpDisk=0 Weight=1 Owner=N/A MCS_label=N/A
   Partitions=ALL,CPU_7452,OS_CENTOS8 
   BootTime=2021-03-10T09:21:06 SlurmdStartTime=2021-04-21T16:50:40
   CfgTRES=cpu=128,mem=1M,billing=128
   AllocTRES=
   CapWatts=n/a
   CurrentWatts=0 AveWatts=0
   ExtSensorsJoules=n/s ExtSensorsWatts=0 ExtSensorsTemp=n/s

Node a not found
Node b not found
)EOF";

const std::string scontrolShowNodeFailedPartially2 = R"EOF(
Node a not found
Node b not found
NodeName=gmz01 Arch=x86_64 CoresPerSocket=8 
   CPUAlloc=0 CPUTot=128 CPULoad=0.00
   AvailableFeatures=(null)
   ActiveFeatures=(null)
   Gres=(null)
   NodeAddr=gmz01 NodeHostName=gmz01 Version=20.02.3
   OS=Linux 4.18.0-193.28.1.el8_2.x86_64 #1 SMP Thu Oct 22 00:20:22 UTC 2020 
   RealMemory=1 AllocMem=0 FreeMem=253909 Sockets=8 Boards=1
   State=IDLE ThreadsPerCore=2 TmpDisk=0 Weight=1 Owner=N/A MCS_label=N/A
   Partitions=ALL,CPU_7452,OS_CENTOS8 
   BootTime=2021-03-10T09:21:06 SlurmdStartTime=2021-04-21T16:50:40
   CfgTRES=cpu=128,mem=1M,billing=128
   AllocTRES=
   CapWatts=n/a
   CurrentWatts=0 AveWatts=0
   ExtSensorsJoules=n/s ExtSensorsWatts=0 ExtSensorsTemp=n/s
)EOF";

const std::string scontrolShowNodeFailed = R"EOF(
Node a not found
)EOF";

const std::string scontrolShowNodeRes = R"EOF({"name":"gmz01","state":"online","rawState":"IDLE","cpus":128,"cpusReserved":0,"cpusOfJobs":{}})EOF";


TEST_CASE("Slurm nodes") {
    using namespace cw::batch;
    SECTION("partial unknown 1") {
        std::vector<Node> nodes;
        slurm::SlurmBatch::parseNodes(scontrolShowNodeFailedPartially, [&](Node n){nodes.push_back(n); return true;});
        REQUIRE(nodes.size() == 1);
        REQUIRE(json::serialize(nodes[0]) == scontrolShowNodeRes);
    }
    SECTION("partial unknown 2") {
        std::vector<Node> nodes;
        slurm::SlurmBatch::parseNodes(scontrolShowNodeFailedPartially2, [&](Node n){nodes.push_back(n); return true;});
        REQUIRE(nodes.size() == 1);
        REQUIRE(json::serialize(nodes[0]) == scontrolShowNodeRes);

    }
    SECTION("unknown") {
        std::vector<Node> nodes;
        slurm::SlurmBatch::parseNodes(scontrolShowNodeFailed, [&](Node n){nodes.push_back(n); return true;});
        REQUIRE(nodes.size() == 0);
    }
}

const std::string pbsnodesxUnknown = R"EOF(
pbsnodes: Unknown node  MSG=cannot locate specified node
)EOF";

const std::string pbsnodesx = R"EOF(
<Data><Node><name>ubuntu</name><state>down</state><power_state>Running</power_state><np>1</np><ntype>cluster</ntype><note>testnote</note><mom_service_port>15002</mom_service_port><mom_manager_port>15003</mom_manager_port></Node></Data>
)EOF";
TEST_CASE("Pbs") {
    using namespace cw::batch;
    SECTION("unknown node") {
        std::vector<Node> nodes;
        pbs::PbsBatch::parseNodes(pbsnodesxUnknown, [&](Node n){nodes.push_back(n); return true;});
        REQUIRE(nodes.size() == 0);
    }

    SECTION("successfull") {
        std::vector<Node> nodes;
        pbs::PbsBatch::parseNodes(pbsnodesx, [&](Node n){nodes.push_back(n); return true;});
        REQUIRE(nodes.size() == 1);
        REQUIRE(json::serialize(nodes[0]) == R"EOF({"name":"ubuntu","nameFull":"ubuntu","state":"offline","rawState":"down","comment":"testnote","cpus":1,"cpusOfJobs":{}})EOF");

    }
}