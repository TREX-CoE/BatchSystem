#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {

CommandFailed::CommandFailed(const std::string& msg, const std::string& cmd, int ret): std::runtime_error(msg+"|"+cmd+" returned "+std::to_string(ret)), retno(ret) {}
int CommandFailed::returncode() const { return retno; }

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
	}
	return "unknown";
}

const char* to_cstr(const QueueState& state) {
	switch (state) {
		case QueueState::Unknown: return "unknown";
		case QueueState::Open: return "open";
		case QueueState::Closed: return "closed";
		case QueueState::Inactive: return "inactive";
		case QueueState::Draining: return "draining";
	}
	return "unknown";
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
	}
	return "unknown";
}

}
}