#include "pbs.h"
#include "pbsBatch.h"

#include <iostream>
#include <sstream>

using namespace cw::batch;

namespace cw {
namespace batch {
namespace pbs {

void create_batch(BatchSystem& inf, std::function<cmd_execute_f> _func) {
	PbsBatch state(_func);
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
	inf.rescheduleRunningJobInQueue = [state](auto... args){ state.rescheduleRunningJobInQueue(args...); };
	inf.setNodeComment = [state](auto... args){ state.setNodeComment(args...); };
}

}
}
}