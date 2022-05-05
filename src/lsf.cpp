#include "lsf.h"
#include "lsfBatch.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <functional>

using namespace cw::batch;

namespace cw {
namespace batch {
namespace lsf {

void create_batch(BatchSystem& inf, std::function<cmd_execute_f> _func) {
	LsfBatch state(_func);
	inf.getNodes = [state](auto... args){ state.getNodes(args...); };
	inf.getJobs = [state](auto... args){ state.getJobs(args...); };
	inf.getQueues = [state](auto... args){ state.getQueues(args...); };
	inf.setQueueState = [state](auto... args){ state.setQueueState(args...); };
	inf.changeNodeState = [state](auto... args){ state.changeNodeState(args...); };
	inf.releaseJob = [state](auto... args){ state.releaseJob(args...); };
	inf.holdJob = [state](auto... args){ state.holdJob(args...); };
	inf.deleteJobByUser = [state](auto... args){ state.deleteJobByUser(args...); };
	inf.deleteJobById = [state](auto... args){ state.deleteJobById(args...); };
	inf.suspendJob = nullptr;
	inf.resumeJob = nullptr;
	inf.rescheduleRunningJobInQueue = nullptr;
	inf.setNodeComment = nullptr;
}

}
}
}