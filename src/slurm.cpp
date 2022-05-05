#include "slurm.h"
#include "slurmBatch.h"

using namespace cw::batch;
using namespace cw::batch::slurm;

namespace {

void create_batch(BatchSystem& inf, SlurmBatch state) {

	inf.runJob = [state](auto... args){ return state.runJob(args...); };
	inf.getBatchInfo = [state](){ return state.getBatchInfo(); };
	inf.getNodes = [state](auto... args){ state.getNodes(args...); };
	inf.getJobs = [state](auto... args){ state.getJobs(args...); };
	inf.getQueues = [state](auto... args){ state.getQueues(args...); };
	inf.setQueueState = [state](auto... args){ state.setQueueState(args...); };
	inf.changeNodeState = [state](auto... args){ state.changeNodeState(args...); };
	inf.releaseJob = [state](auto... args){ state.releaseJob(args...); };
	inf.holdJob = [state](auto... args){ state.holdJob(args...); };
	inf.deleteJobByUser = [state](auto... args){ state.deleteJobByUser(args...); };
	inf.deleteJobById = [state](auto... args){ state.deleteJobById(args...); };
	inf.suspendJob = [state](auto... args){ state.suspendJob(args...); };
	inf.resumeJob = [state](auto... args){ state.resumeJob(args...); };
	inf.rescheduleRunningJobInQueue = [state](const std::string& job, bool){ state.rescheduleRunningJobInQueue(job, false); };
	inf.setNodeComment = [state](auto... args){ state.setNodeComment(args...); };
}

}

namespace cw {
namespace batch {
namespace slurm {

void create_batch(BatchSystem& inf, std::function<cmd_execute_f> _func, bool useSacct) {
	::create_batch(inf, SlurmBatch(_func, useSacct));
}

void create_batch(BatchSystem& inf, std::function<cmd_execute_f> _func) {
	::create_batch(inf, SlurmBatch(_func));
}

}
}
}