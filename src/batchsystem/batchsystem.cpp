#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {

NotImplemented::NotImplemented(const char* msg): std::logic_error(msg) { }

BatchInterface::~BatchInterface() {}

bool BatchInterface::getNodes(supported_t) { return false; }
bool BatchInterface::getQueues(supported_t) { return false; }
bool BatchInterface::getJobs(supported_t) { return false; }
bool BatchInterface::getBatchInfo(supported_t) { return false; }
bool BatchInterface::deleteJobById(supported_t) { return false; }
bool BatchInterface::deleteJobByUser(supported_t) { return false; }
bool BatchInterface::changeNodeState(supported_t) { return false; }
bool BatchInterface::setQueueState(supported_t) { return false; }
bool BatchInterface::runJob(supported_t) { return false; }
bool BatchInterface::setNodeComment(supported_t) { return false; }
bool BatchInterface::holdJob(supported_t) { return false; }
bool BatchInterface::releaseJob(supported_t) { return false; }
bool BatchInterface::suspendJob(supported_t) { return false; }
bool BatchInterface::resumeJob(supported_t) { return false; }
bool BatchInterface::rescheduleRunningJobInQueue(supported_t) { return false; }


void BatchInterface::getNodes(std::vector<std::string>, std::function<void(std::vector<Node> nodes, std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::getJobs(std::vector<std::string>, std::function<void(std::vector<Job> jobs, std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::getQueues(std::vector<std::string>, std::function<void(std::vector<Queue> queues, std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::rescheduleRunningJobInQueue(std::string, bool, std::function<void(std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::setQueueState(std::string, QueueState, bool, std::function<void(std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::resumeJob(std::string, bool, std::function<void(std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::suspendJob(std::string, bool, std::function<void(std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::deleteJobByUser(std::string, bool, std::function<void(std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::deleteJobById(std::string, bool, std::function<void(std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::holdJob(std::string, bool, std::function<void(std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::releaseJob(std::string, bool, std::function<void(std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::setNodeComment(std::string, bool, std::string, bool, std::function<void(std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::changeNodeState(std::string, NodeChangeState, bool, std::string, bool, std::function<void(std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::runJob(JobOptions, std::function<void(std::string jobName, std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::detect(std::function<void(bool has_batch, std::error_code ec)>)  { throw NotImplemented(__func__); }
void BatchInterface::getBatchInfo(std::function<void(BatchInfo info, std::error_code ec)>)  { throw NotImplemented(__func__); }

const char* to_cstr(const JobState& state) {
	switch (state) {
		case JobState::Unknown: return "unknown";
		case JobState::Running: return "running";
		case JobState::Queued: return "queued";
		case JobState::Requeued: return "requeued";
		case JobState::Terminating: return "terminating";
		case JobState::Finished: return "finished";
		case JobState::Cancelled: return "cancelled";
		case JobState::Failed: return "failed";
		case JobState::Moved: return "moved";
		case JobState::Suspend: return "suspend";
		case JobState::Zombie: return "zombie";
		default: return "invalid";
	}
}

const char* to_cstr(const QueueState& state) {
	switch (state) {
		case QueueState::Unknown: return "unknown";
		case QueueState::Open: return "open";
		case QueueState::Closed: return "closed";
		case QueueState::Inactive: return "inactive";
		case QueueState::Draining: return "draining";
		default: return "invalid";
	}
}

const char* to_cstr(const NodeState& state) {
	switch (state) {
		case NodeState::Unknown: return "unknown";
		case NodeState::Various: return "various";
		case NodeState::Online: return "online";
		case NodeState::Offline: return "offline";
		case NodeState::Disabled: return "disabled";
		case NodeState::Powersave: return "powersave";
		case NodeState::Reserved: return "reserved";
		case NodeState::Maintainence: return "maintainence";
		case NodeState::Failed: return "failed";
		default: return "invalid";
	}
}

}
}
