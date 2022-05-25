#include "batchsystem/batchsystem.h"

namespace {

std::string to_string(const cw::batch::CmdOptions& opts) {
	std::string s = opts.cmd;
	if (!opts.args.empty()) {
		for (const auto &arg : opts.args) {
			s += " " + arg;
		} 
	}
	return s;
}

}

namespace cw {
namespace batch {

NotImplemented::NotImplemented(const char* msg): std::logic_error(msg) { }


BatchInterface::~BatchInterface() {}

void BatchInterface::resetCache() {}

bool BatchInterface::getNodes(const std::vector<std::string>&, std::function<getNodes_inserter_f>) { throw NotImplemented(__func__); }
bool BatchInterface::getNodes(supported_t) { return false; }
bool BatchInterface::getQueues(std::function<getQueues_inserter_f>) { throw NotImplemented(__func__); }
bool BatchInterface::getQueues(supported_t) { return false; }
bool BatchInterface::getJobs(std::function<getJobs_inserter_f>) { throw NotImplemented(__func__); }
bool BatchInterface::getJobs(supported_t) { return false; }
bool BatchInterface::getBatchInfo(BatchInfo&) { throw NotImplemented(__func__); }
bool BatchInterface::getBatchInfo(supported_t) { return false; }
bool BatchInterface::deleteJobById(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::deleteJobById(supported_t) { return false; }
bool BatchInterface::deleteJobByUser(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::deleteJobByUser(supported_t) { return false; }
bool BatchInterface::changeNodeState(const std::string&, NodeChangeState, bool, const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::changeNodeState(supported_t) { return false; }
bool BatchInterface::setQueueState(const std::string&, QueueState, bool) { throw NotImplemented(__func__); }
bool BatchInterface::setQueueState(supported_t) { return false; }
bool BatchInterface::runJob(const JobOptions&, std::string&) { throw NotImplemented(__func__); }
bool BatchInterface::runJob(supported_t) { return false; }
bool BatchInterface::setNodeComment(const std::string&, bool, const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::setNodeComment(supported_t) { return false; }
bool BatchInterface::holdJob(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::holdJob(supported_t) { return false; }
bool BatchInterface::releaseJob(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::releaseJob(supported_t) { return false; }
bool BatchInterface::suspendJob(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::suspendJob(supported_t) { return false; }
bool BatchInterface::resumeJob(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::resumeJob(supported_t) { return false; }
bool BatchInterface::rescheduleRunningJobInQueue(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::rescheduleRunningJobInQueue(supported_t) { return false; }

CommandFailed::CommandFailed(const std::string& msg, const CmdOptions& cmd, int ret): std::runtime_error(msg+"|"+to_string(cmd)+" returned "+std::to_string(ret)), retno(ret) {}
int CommandFailed::returncode() const { return retno; }

bool operator<(const CmdOptions& l, const CmdOptions& r) {
     return (l.cmd<r.cmd || (l.cmd==r.cmd && l.args<r.args));
}

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
