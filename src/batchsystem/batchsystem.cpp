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
/*
bool BatchInterface::detect(bool& detected) = 0;
bool BatchInterface::getNodes(const std::vector<std::string>& filterNodes, std::function<getNodes_inserter_f> insert) = 0;
bool BatchInterface::getQueues(std::function<getQueues_inserter_f> insert) = 0;
bool BatchInterface::getJobs(std::function<getJobs_inserter_f> insert) = 0;
bool BatchInterface::getBatchInfo(BatchInfo& info) = 0;
bool BatchInterface::deleteJobById(const std::string& job, bool force) = 0;
bool BatchInterface::deleteJobByUser(const std::string& user, bool force) = 0;
bool BatchInterface::changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) = 0;
bool BatchInterface::setQueueState(const std::string& name, QueueState state, bool force) = 0;
bool BatchInterface::runJob(const JobOptions& opts, std::string& jobName) = 0;
*/
bool BatchInterface::setNodeComment(const std::string&, bool, const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::holdJob(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::releaseJob(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::suspendJob(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::resumeJob(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::rescheduleRunningJobInQueue(const std::string&, bool) { throw NotImplemented(__func__); }
bool BatchInterface::rescheduleRunningJobInQueue(supported) { return false; }

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
