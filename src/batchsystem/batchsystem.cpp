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


std::function<getNodes_f> BatchInterface::getNodes(std::vector<std::string>) { throw NotImplemented(__func__); }
std::function<getJobs_f> BatchInterface::getJobs(std::vector<std::string>) { throw NotImplemented(__func__); }
std::function<getQueues_f> BatchInterface::getQueues() { throw NotImplemented(__func__); }
std::function<bool()> BatchInterface::rescheduleRunningJobInQueue(const std::string&, bool) { throw NotImplemented(__func__); }
std::function<bool()> BatchInterface::setQueueState(const std::string&, QueueState, bool) { throw NotImplemented(__func__); }
std::function<bool()> BatchInterface::resumeJob(const std::string&, bool) { throw NotImplemented(__func__); }
std::function<bool()> BatchInterface::suspendJob(const std::string&, bool) { throw NotImplemented(__func__); }
std::function<bool()> BatchInterface::deleteJobByUser(const std::string&, bool) { throw NotImplemented(__func__); }
std::function<bool()> BatchInterface::deleteJobById(const std::string&, bool) { throw NotImplemented(__func__); }
std::function<bool()> BatchInterface::holdJob(const std::string&, bool) { throw NotImplemented(__func__); }
std::function<bool()> BatchInterface::releaseJob(const std::string&, bool) { throw NotImplemented(__func__); }
std::function<bool()> BatchInterface::setNodeComment(const std::string&, bool, const std::string&, bool) { throw NotImplemented(__func__); }
std::function<bool()> BatchInterface::changeNodeState(const std::string&, NodeChangeState, bool, const std::string&, bool) { throw NotImplemented(__func__); }
std::function<runJob_f> BatchInterface::runJob(const JobOptions&) { throw NotImplemented(__func__); }
std::function<detect_f> BatchInterface::detect() { throw NotImplemented(__func__); }
std::function<getBatchInfo_f> BatchInterface::getBatchInfo() { throw NotImplemented(__func__); }



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
