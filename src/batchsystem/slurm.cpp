#include "batchsystem/slurm.h"

#include <iostream>
#include <sstream>
#include <numeric>
#include <regex>
#include <iomanip>

#include "batchsystem/internal/trim.h"
#include "batchsystem/internal/streamCast.h"
#include "batchsystem/internal/splitString.h"
#include "batchsystem/internal/timeToEpoch.h"
#include "batchsystem/internal/joinString.h"
#include "batchsystem/internal/singleCmd.h"

using namespace cw::batch;
using namespace cw::batch::internal;

namespace {

JobState parseJobState(const std::string& str) {
	if(str=="PENDING")
	{
		return JobState::Queued;
	}
	else if(str=="RUNNING")
	{
		return JobState::Running;
	}
	else if(str=="SUSPENDED")
	{
		return JobState::Suspend;
	}
	else if(str=="COMPLETING")
	{
		return JobState::Terminating;
	}
	else if(str=="COMPLETED")
	{
		return JobState::Finished;
	}
	else if(str.rfind("CANCELLED", 0) == 0) // to handle "CANCELLED by 1000" (1000 is UID) syntax
	{
		return JobState::Cancelled;
	}
	else if(str=="FAILED")
	{
		return JobState::Failed;
	}
	else
	{
		return JobState::Unknown;
	}
}

bool getMatches(const std::regex& r, const std::string& s, size_t offset, std::vector<std::ssub_match>& matches, bool search=true) {
	std::smatch base_match;
	bool status = search ? std::regex_search(s.begin()+static_cast<ssize_t>(offset), s.end(), base_match, r) : std::regex_match(s.begin()+static_cast<ssize_t>(offset), s.end(), base_match, r);
	if (status) {
		for (size_t i = 0; i < base_match.size(); ++i) {
			matches.push_back(base_match[i]);
		}
	}
	return status;
}

const std::regex regSingleNode("^[^\\[^\\]^,]+");
const std::regex regNodeList("^([^\\[^\\]^,]+)\\[([^\\]]+)\\]");
const std::regex regNodeRange("(\\d+)\\-(\\d+)");
const std::regex regNode("(\\d+)");

std::vector<std::string> nodelist_helper(const std::string& nodeList) {
	std::vector<std::string> nodes;
	if(nodeList!="(null)" && nodeList!="None assigned") // handle case if no node assigned
	{
		std::string subValue=nodeList;

		size_t pos=0;
		while(!subValue.empty())
		{
			std::vector<std::ssub_match> posNodes;
			if(getMatches(regNodeList, subValue, 0, posNodes) && posNodes.size()==3)
			{
				std::string nodeName=posNodes[1].str();
				std::string range=posNodes[2].str();
				size_t offsetRange=0;
				while(1)
				{
					bool rangeMatched=false;
					bool nodeMatched=false;

					std::vector<std::ssub_match> posRange;
					if(getMatches(regNodeRange, range, offsetRange, posRange) && posRange.size()==3)
					{
						rangeMatched=true;
					}

					std::vector<std::ssub_match> posNode;
					if(getMatches(regNode, range, offsetRange, posNode) && posNode.size()==2)
					{
						nodeMatched=true;
					}

					if(rangeMatched && nodeMatched)
					{
						if(std::distance(range.cbegin(), posRange[0].second)<=std::distance(range.cbegin(), posNode[0].second))
						{
							nodeMatched=false;
						}
						else
						{
							rangeMatched=false;
						}
					}

					if(rangeMatched)
					{
						std::string nodeStart=posRange[1].str();
						std::string nodeEnd=posRange[2].str();

						offsetRange=static_cast<size_t>(std::distance(range.cbegin(), posRange[2].second));

						int nodeIdStart=not_finished;
						int nodeIdEnd=not_finished;
						int tmp;
						int padding;

						if(stream_cast<int>(nodeStart,tmp))
						{
							nodeIdStart=tmp;
						}
						if(stream_cast<int>(nodeEnd,tmp))
						{
							nodeIdEnd=tmp;
						}

						if(nodeIdStart>=0 && nodeIdEnd>=0)
						{
							padding=static_cast<int>(nodeEnd.length());

							for(int i=nodeIdStart;i<=nodeIdEnd;i++)
							{
								std::ostringstream oss;
								oss << std::setfill('0') << std::setw(padding) << i;
								nodes.push_back(oss.str());
							}
						}
					}
					else if(nodeMatched)
					{
						offsetRange=static_cast<size_t>(std::distance(range.cbegin(), posNode[1].second));
						nodes.push_back(nodeName+posNode[1].str());
					}
					else
					{
						break;
					}
				}

				pos=static_cast<size_t>(std::distance(nodeList.cbegin(), posNodes[2].second));
			}
			else if (getMatches(regSingleNode, subValue, 0, posNodes) && posNodes.size()==1)
			{
				nodes.push_back(posNodes[0].str());
				pos=static_cast<size_t>(std::distance(nodeList.begin(), posNodes[0].second));
			}

			pos=nodeList.find(',',pos);
			if(pos==nodeList.npos)
			{
				break;
			}

			subValue=nodeList.substr(pos+1);
		}
	}
	return nodes;
}

}

namespace cw {
namespace batch {
namespace slurm {

const std::string Slurm::DefaultReason = "batchsystem_api";

Slurm::Slurm(cmd_f func): _func(func), _mode(job_mode::unchecked) {}

bool Slurm::getNodes(supported_t) { return true; }
bool Slurm::getQueues(supported_t) { return true; }
bool Slurm::getJobs(supported_t) { return true; }
bool Slurm::getBatchInfo(supported_t) { return true; }
bool Slurm::deleteJobById(supported_t) { return true; }
bool Slurm::deleteJobByUser(supported_t) { return true; }
bool Slurm::changeNodeState(supported_t) { return true; }
bool Slurm::setQueueState(supported_t) { return true; }
bool Slurm::runJob(supported_t) { return true; }
bool Slurm::setNodeComment(supported_t) { return true; }
bool Slurm::holdJob(supported_t) { return true; }
bool Slurm::releaseJob(supported_t) { return true; }
bool Slurm::suspendJob(supported_t) { return true; }
bool Slurm::resumeJob(supported_t) { return true; }
bool Slurm::rescheduleRunningJobInQueue(supported_t) { return true; }

void Slurm::setJobMode(Slurm::job_mode mode) {
	_mode = mode;
}

Slurm::job_mode Slurm::getJobMode() const {
	return _mode;
}

void Slurm::parseNodes(const std::string& output, const std::function<getNodes_inserter_f>& insert) {
	std::stringstream commandResult(output);

	std::stringstream buffer;
	std::string tmp, name, value, nodeName;

	Node node {};

	while(commandResult.good())
	{
		getline(commandResult,tmp);
		// skip empty lines
		if (tmp.empty()) continue;
		// skip Node a not found lines
		if (tmp.rfind("Node ", 0) == 0) continue;

		buffer.clear();
		buffer.str("");

		buffer << tmp;

		while(buffer.good())
		{
			buffer >> tmp;

			size_t pos=tmp.find('=');
			if(pos!=tmp.npos)
			{
				name=tmp.substr(0,pos);
				value=tmp.substr(pos+1);

				if(name=="NodeName")
				{
					if (node.name.has_value()) {
						std::cout << "insert " << node.name.get() << std::endl;
						if (!insert(std::move(node))) return;
						node = Node{}; // reset to default
					}
					node.name=value;
				}
				else if(!node.name.get().empty())
				{
					if(name=="State")
					{
						node.rawState = value;
						if(value=="ALLOCATED" || value=="ALLOCATED*" || value=="MIXED" || value=="MIXED*" || value=="COMPLETING" || value=="COMPLETED" || value=="IDLE")
						{
							node.state = NodeState::Online;
						}
						else if(value=="DOWN" || value=="DOWN*" || value=="DOWN+DRAIN" || value=="DOWN*+DRAIN" || value=="DRAINED" || value=="DRAINING" || value=="IDLE+DRAIN" || value=="IDLE*+DRAIN" || value=="ALLOCATED+DRAIN" || value=="ALLOCATED*+DRAIN" || value=="MIXED+DRAIN" || value=="MIXED*+DRAIN")
						{
							node.state = NodeState::Disabled;
						}
						else if(value=="IDLE+POWER")
						{
							node.state = NodeState::Powersave;
						}
						else if(value=="MAINT" || value=="MAINT*")
						{
							node.state = NodeState::Maintainence;
						}
						else if(value=="RESERVED" || value=="RESERVED*")
						{
							node.state = NodeState::Reserved;
						}
						else if(value=="FAIL" || value=="FAILING")
						{
							node.state = NodeState::Offline;
						}
						else if(value=="FAILED" || value=="CANCELLED")
						{
							node.state = NodeState::Failed;
						}
						else
						{
							node.state = NodeState::Unknown;
						}
					}
					else if(name=="CPUTot")
					{
						stream_cast(value, node.cpus.get());
						node.cpusReserved = 0;
					}
					else if(name=="CPUAlloc")
					{
						stream_cast(value, node.cpusReserved.get());
					}
					else if(name=="Comment")
					{
						if (value != "(null)") node.comment = value;
					}
					else if(name=="Reason")
					{
						node.reason = value;
					}
				}
			}
		}
	}
	// check after last line if as uninserted node
	if (node.name.has_value()) insert(std::move(node));
}

void Slurm::parseShowJob(const std::string& output, std::function<bool(std::map<std::string, std::string>)> insert) {
	std::stringstream commandResult(output);
	std::stringstream buffer;
	std::string tmp, name, value, jobid;
	std::map<std::string, std::string> job;
	while(commandResult.good())
	{

		getline(commandResult,tmp);

		buffer.clear();
        buffer.str("");

		buffer << tmp;

		while(buffer.good())
		{
			buffer >> tmp;

			size_t pos=tmp.find('=');
			if(pos!=tmp.npos)
			{
				name=tmp.substr(0,pos);
				value=tmp.substr(pos+1);

				if(name=="JobId")
				{
					if (!jobid.empty()) {
						if (!insert(std::move(job))) return;
						job.clear();
						jobid.clear();
					}
					jobid=value;
					job[name] = value;
				}
				else if(!jobid.empty())
				{
					job[name] = value;
				}
			}
		}
	}
}

void Slurm::jobMapToStruct(const std::map<std::string, std::string>& j, Job& job) {
	for (const auto& p : j) {
		if (p.first=="JobId" || p.first=="JobID") {
			job.id = p.second;
		}
		else if(p.first=="JobState")
		{
			job.state = parseJobState(p.second);
			job.rawState = p.second;
		}
		else if(p.first=="SubmitTime" || p.first=="Submit")
		{
			timeToEpoch(p.second, job.submitTime.get(), "%Y-%m-%dT%H:%M:%S");
		}
		else if(p.first=="StartTime" || p.first=="Start")
		{
			timeToEpoch(p.second, job.startTime.get(), "%Y-%m-%dT%H:%M:%S");
		}
		else if(p.first=="EndTime" || p.first=="End")
		{
			timeToEpoch(p.second, job.endTime.get(), "%Y-%m-%dT%H:%M:%S");
		}
		else if(p.first=="UserId")
		{
			job.owner = p.second;
		}
		else if(p.first=="NumNodes" || p.first=="NNodes")
		{
			stream_cast(p.second, job.nodesRequested.get());
		}
		else if(p.first=="NumCPUs" || p.first=="NCPUS")
		{
			stream_cast(p.second, job.cpus.get());
		}
		else if(p.first=="Partition")
		{
			job.queue = p.second;
		}
		else if(p.first=="Priority")
		{
			stream_cast(p.second, job.priority.get());
		}
		else if(p.first=="JobName")
		{
			job.name = p.second;
		}
		else if(p.first=="NodeList")
		{
			for (const auto& node : nodelist_helper(p.second)) {
				job.cpusFromNode[node] = 0;
			}
		}
		else if(p.first=="BatchHost")
		{
			job.submitHost = p.second;
		}
		else if(p.first=="Reason")
		{
			job.reason = "None" ? "" : p.second;
		}
		else if(p.first=="Comment")
		{
			job.comment = p.second;
		}
		else if(p.first=="USER")
		{
			job.user = p.second;
		}
		else if(p.first=="UID")
		{
			stream_cast(p.second, job.userId.get());
		}
		else if(p.first=="UserId")
		{
			auto pos = p.second.find("(");
			job.user = p.second.substr(0,pos);
			if (pos != std::string::npos) {
				stream_cast(p.second.substr(pos+1, p.second.find(")")), job.userId.get());
			}
		}
		else if(p.first=="GROUP")
		{
			job.group = p.second;
		}
		else if(p.first=="GID")
		{
			stream_cast(p.second, job.groupId.get());
		}
		else if(p.first=="GroupId")
		{
			auto pos = p.second.find("(");
			job.group = p.second.substr(0,pos);
			if (pos != std::string::npos) {
				stream_cast(p.second.substr(pos+1, p.second.find(")")), job.groupId.get());
			}
		}
		else if(p.first=="WorkDir")
		{
			job.workdir = p.second;
		}
		else if(p.first=="ReqNodes")
		{
			stream_cast(p.second, job.nodesRequested.get());
		}
		else if(p.first=="AdminComment")
		{
			job.commentAdmin = p.second;
		}
		else if(p.first=="SystemComment")
		{
			job.commentSystem = p.second;
		}
		else if(p.first=="ReqTRES")
		{
			splitString(p.second, ",", [&](size_t start, size_t end){
				std::string entry = p.second.substr(start, end);
				auto pos = entry.find("=");
				if (pos != std::string::npos) {
					job.requestedGeneral[entry.substr(0, pos)] = entry.substr(pos+1);
				}
				return true;
			});
		}
		else if(p.first=="ExitCode")
		{
			auto pos = p.second.find("=");
			if (pos != std::string::npos) {
				stream_cast(p.second.substr(0, pos), job.exitCode.get());
				stream_cast(p.second.substr(pos+1), job.exitSignal.get());
			}
		}
	}
}

void Slurm::parseJobsLegacy(const std::string& output, std::function<getJobs_inserter_f> insert) {
	Slurm::parseShowJob(output, [&insert](std::map<std::string, std::string> j){
		Job job {};
		Slurm::jobMapToStruct(j, job);
		if (!insert(job)) return false;
		return true;
	});
}

void Slurm::parseSacct(const std::string& output, std::function<bool(std::map<std::string, std::string>)> insert) {
	std::stringstream commandResult(output);

	std::string line;
	
	// parse header (skip empty lines)
	while (line.empty()) getline(commandResult, line);

	std::vector<std::string> header;
	splitString(line, "|", [&](size_t start, size_t end) {
		header.push_back(line.substr(start, end));
		return true;
	});
	
	while(commandResult.good()) {
		getline(commandResult, line);
		if (line.empty()) continue;
		std::map<std::string, std::string> job;
		size_t i = 0;
		splitString(line, "|", [&](size_t start, size_t end) {
			if (i == header.size()) return false;
			job[header[i]]=line.substr(start, end);
			++i;
			return true;
		});
		if (!insert(job)) return;
	}
}

void Slurm::parseJobsSacct(const std::string& output, std::function<getJobs_inserter_f> insert) {
	Slurm::parseSacct(output, [&insert](std::map<std::string, std::string> j){
		Job job {};
		Slurm::jobMapToStruct(j, job);
		if (!insert(job)) return false;
		return true;
	});
}

void Slurm::parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert) {
	std::stringstream commandResult(output);

	std::stringstream buffer;
	std::string tmp,name,value;
	Queue queue {};

	while(commandResult.good())
	{
		getline(commandResult,tmp);

		buffer.clear();
		buffer.str("");

		buffer << tmp;

		while(buffer.good())
		{
			buffer >> tmp;

			size_t pos=tmp.find('=');
			if(pos!=tmp.npos)
			{
				name=tmp.substr(0,pos);
				value=tmp.substr(pos+1);

				if(name=="PartitionName")
				{
					if(queue.name.has_value()) {
						if (!insert(std::move(queue))) return;
						queue.name.reset();
					} else {
						queue.name=value;
					}
				}
				else if(queue.name.has_value())
				{
					if(name=="State")
					{
						queue.rawState = value;
						if(value=="UP")
						{
							queue.state = QueueState::Open;
						}
						else if(value=="DOWN")
						{
							queue.state = QueueState::Closed;
						}
						else if(value=="DRAIN")
						{
							queue.state = QueueState::Draining;
						}
						else if(value=="INACTIVE")
						{
							queue.state = QueueState::Inactive;
						}
						else
						{
							queue.state = QueueState::Unknown;
						}
					}
					else if(name=="TotalCPUs")
					{
						stream_cast(value, queue.cpus.get());
					}
					else if(name=="TotalNodes")
					{
						stream_cast(value, queue.nodesTotal.get());
					}
					else if(name=="PriorityTier")
					{
						stream_cast(value, queue.priority.get());
					}
					else if(name=="Nodes")
					{
						queue.nodes = nodelist_helper(value);
					}
					else if(name=="MaxNodes")
					{
						if (value != "UNLIMITED") stream_cast(value, queue.nodesMax.get());
					}
					else if(name=="MinNodes")
					{
						stream_cast(value, queue.nodesMin.get());
					}
					else if(name=="MaxTime")
					{
						if (value != "UNLIMITED") timeToEpoch(value, queue.wallTimeMax.get(), "%H:%M:%S");
					}
					else if(name=="MaxMemPerCPU")
					{
						if (value != "UNLIMITED") {
							uint64_t val = 0;
							if (stream_cast(value, val)) {
								queue.memPerCpuMax = val * 1000000; // given as MB in slurm output
							}
						}
					}
				}
			}
		}
	}
}

class ChangeNodeState: public SingleCmd {
private:
	std::string name;
	NodeChangeState nodeState;
	bool force;
	std::string reason;
	bool appendReason;
public:
	ChangeNodeState(cmd_f& cmd_, const std::string& name_, NodeChangeState nodeState_, bool force_, const std::string& reason_, bool appendReason_): SingleCmd(cmd_), name(name_), nodeState(nodeState_), force(force_), reason(reason_), appendReason(appendReason_) {}
    bool operator()() {
        switch (state) {
            case State::Starting: {
				std::string stateString;
				switch (nodeState) {
					case NodeChangeState::Resume: stateString="RESUME"; break;
					case NodeChangeState::Drain: stateString="DRAIN"; break;
					case NodeChangeState::Undrain: stateString="UNDRAIN"; break;
					default: throw std::runtime_error("invalid state"); 
				} 
				std::vector<std::string> args{"update", "NodeName=" + name, "State="+stateString};
				// add reason if needed and force default if empty as needed by slurm!
				if (nodeState != NodeChangeState::Resume) args.push_back("Reason="+(reason.empty() ? Slurm::DefaultReason : reason));

                cmd(res, {"scontrol", args, {}, cmdopt::none});
                state=State::Waiting;
			}
			// fall through
            case State::Waiting:
                if (!checkWaiting("scontrol update")) return false; 
                // fall through
            case State::Done:
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class SetNodeComment: public SingleCmd {
private:
	std::string name;
	bool force;
	std::string comment;
	bool appendComment;
public:
	SetNodeComment(cmd_f& cmd_, const std::string& name_, bool force_, const std::string& comment_, bool appendComment_): SingleCmd(cmd_), name(name_), force(force_), comment(comment_), appendComment(appendComment_) {}
    bool operator()() {
        switch (state) {
            case State::Starting:
                cmd(res, {"scontrol", {"update", "NodeName="+name, "Comment="+comment}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("scontrol update")) return false; 
                // fall through
            case State::Done:
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class ReleaseJob: public SingleCmd {
private:
	std::string job;
	bool force;
public:
	ReleaseJob(cmd_f& cmd_, const std::string& job_, bool force_): SingleCmd(cmd_), job(job_), force(force_) {}
    bool operator()() {
        switch (state) {
            case State::Starting:
                cmd(res, {"scontrol", {"release", job}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("scontrol release")) return false; 
                // fall through
            case State::Done:
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class HoldJob: public SingleCmd {
private:
	std::string job;
	bool force;
public:
	HoldJob(cmd_f& cmd_, const std::string& job_, bool force_): SingleCmd(cmd_), job(job_), force(force_) {}
    bool operator()() {
        switch (state) {
            case State::Starting:
                cmd(res, {"scontrol", {"hold", job}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("scontrol hold")) return false; 
                // fall through
            case State::Done:
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class DeleteJobById: public SingleCmd {
private:
	std::string job;
	bool force;
public:
	DeleteJobById(cmd_f& cmd_, const std::string& job_, bool force_): SingleCmd(cmd_), job(job_), force(force_) {}
    bool operator()() {
        switch (state) {
            case State::Starting:
                cmd(res, {"scancel", {job}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("scancel")) return false; 
                // fall through
            case State::Done:
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class DeleteJobByUser: public SingleCmd {
private:
	std::string user;
	bool force;
public:
	DeleteJobByUser(cmd_f& cmd_, const std::string& user_, bool force_): SingleCmd(cmd_), user(user_), force(force_) {}
    bool operator()() {
        switch (state) {
            case State::Starting:
                cmd(res, {"scancel", {"-u", user}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("scancel -u")) return false; 
                // fall through
            case State::Done:
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class SuspendJob: public SingleCmd {
private:
	std::string job;
	bool force;
public:
	SuspendJob(cmd_f& cmd_, const std::string& job_, bool force_): SingleCmd(cmd_), job(job_), force(force_) {}
    bool operator()() {
        switch (state) {
            case State::Starting:
                cmd(res, {"scontrol", {"suspend", job}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("scontrol suspend")) return false; 
                // fall through
            case State::Done:
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class ResumeJob: public SingleCmd {
private:
	std::string job;
	bool force;
public:
	ResumeJob(cmd_f& cmd_, const std::string& job_, bool force_): SingleCmd(cmd_), job(job_), force(force_) {}
    bool operator()() {
        switch (state) {
            case State::Starting:
                cmd(res, {"scontrol", {"resume", job}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("scontrol resume")) return false; 
                // fall through
            case State::Done:
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class SetQueueState: public SingleCmd {
private:
	std::string name;
	QueueState queueState;
	bool force;
public:
	SetQueueState(cmd_f& cmd_, const std::string& name_, QueueState state_, bool force_): SingleCmd(cmd_), name(name_), queueState(state_), force(force_) {}
    bool operator()() {
        switch (state) {
            case State::Starting: {
				std::string stateStr;
				switch (queueState) {
					case QueueState::Unknown: throw std::runtime_error("unknown state");
					case QueueState::Open: stateStr="UP"; break;
					case QueueState::Closed: stateStr="DOWN"; break;
					case QueueState::Inactive: stateStr="INACTIVE"; break;
					case QueueState::Draining: stateStr="DRAIN"; break;
					default: throw std::runtime_error("unknown state");
				}
				cmd(res, {"scontrol", {"update", "PartitionName=" + name, "State="+stateStr}, {}, cmdopt::none});
                state=State::Waiting;
			}
			// fall through
            case State::Waiting:
                if (!checkWaiting("scontrol update")) return false; 
                // fall through
            case State::Done: {
				return true;
			}
			default: throw std::runtime_error("invalid state");
        }
	}
};

class CheckSacct: public SingleCmd {
public:
	using SingleCmd::SingleCmd;
    bool operator()(bool& sacctSupported) {
        switch (state) {
            case State::Starting:
				// use sacct --helpformat as sacct would list all jobs, sacct --helpformat would not fail if slurmdbd is not working
                cmd(res, {"sacct", {"--helpformat"}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
				if (res.exit==not_finished) {
					return false;
				}
				state = State::Done;
                // fall through
            case State::Done:
				sacctSupported = res.exit==0;
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class Detect: public SingleCmd {
public:
	using SingleCmd::SingleCmd;
    bool operator()(bool& detected) {
        switch (state) {
            case State::Starting:
                cmd(res, {"sinfo", {"--version"}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
				if (res.exit==not_finished) {
					return false;
				}
				state = State::Done;
                // fall through
            case State::Done:
				detected = res.exit==0;
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class RescheduleRunningJobInQueue: public SingleCmd {
private:
	std::string job;
	bool hold;
public:
	RescheduleRunningJobInQueue(cmd_f& cmd_, const std::string& job_, bool hold_): SingleCmd(cmd_), job(job_), hold(hold_) {}
    bool operator()() {
        switch (state) {
            case State::Starting:
                cmd(res, {"scontrol", {hold ? "requeuehold" : "requeue", job}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("scontrol")) return false; 
                // fall through
            case State::Done:
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class GetNodes: public SingleCmd {
private:
	std::vector<std::string> filterNodes;
public:
	GetNodes(cmd_f& cmd_, std::vector<std::string> filterNodes_): SingleCmd(cmd_), filterNodes(filterNodes_) {}
    bool operator()(const std::function<getNodes_inserter_f>& insert) {
        switch (state) {
            case State::Starting: {
				std::vector<std::string> args{"show", "node"};
				if (filterNodes.empty()) {
					args.push_back("--all");
				} else {
					args.push_back(internal::joinString(filterNodes.begin(), filterNodes.end(), ","));
				}

                cmd(res, {"scontrol", args, {}, cmdopt::capture_stdout});
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting("scontrol show node")) return false; 
                // fall through
            case State::Done: 
				Slurm::parseNodes(res.out, insert);
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};


class GetQueues: public SingleCmd {
public:
	using SingleCmd::SingleCmd;
    bool operator()(const std::function<getQueues_inserter_f>& insert) {
        switch (state) {
            case State::Starting: {
                cmd(res, {"scontrol", {"show", "partition", "--all"}, {}, cmdopt::capture_stdout});
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting("scontrol show partition")) return false; 
                // fall through
            case State::Done: 
				Slurm::parseQueues(res.out, insert);
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class RunJob: public SingleCmd {
private:
	JobOptions opts;
public:
	RunJob(cmd_f& cmd_, JobOptions opts_): SingleCmd(cmd_), opts(opts_) {}

    bool operator()(std::string& jobName) {
        switch (state) {
            case State::Starting: {
				// --parsable to only print job id
				Cmd c{"sbatch", {"--parsable"}, {}, cmdopt::capture_stdout};
				if (opts.numberNodes.has_value()) {
					c.args.push_back("-N");
					c.args.push_back(opts.numberNodesMax.has_value() ? std::to_string(opts.numberNodes.get()) : (std::to_string(opts.numberNodes.get())+"-"+std::to_string(opts.numberNodesMax.get())));
				}
				if (opts.numberTasks.has_value()) {
					c.args.push_back("-n");
					c.args.push_back(std::to_string(opts.numberTasks.get()));
				}
				if (opts.numberGpus.has_value()) {
					c.args.push_back("-G");
					c.args.push_back(std::to_string(opts.numberGpus.get()));
				}
				c.args.push_back(opts.path.get());

                cmd(res, c);
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting("sbatch")) return false; 
                // fall through
            case State::Done: 
				jobName = trim_copy(res.out);
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};


class GetJobs {
private:
    cmd_f& cmd;
    Result sacctSupported;
    Result jobs;
	std::string stateFilter;
	std::vector<std::string> filterJobs;
    enum class State {
        SacctCheckStart,
        SacctCheckWaiting,
        ScontrolStart,
		ScontrolWaiting,
		ScontrolDone,
		SacctStart,
		SacctWaiting,
		SacctDone,
    };
    State state;
public:
    GetJobs(cmd_f& cmd_, Slurm::job_mode mode_, std::string stateFilter_, std::vector<std::string> filterJobs_): cmd(cmd_), stateFilter(stateFilter_), filterJobs(filterJobs_) {
		switch (mode_) {
			case Slurm::job_mode::unchecked: state = State::SacctCheckStart; break;
			case Slurm::job_mode::scontrol: state = State::ScontrolStart; break;
			case Slurm::job_mode::sacct: state = State::SacctStart; break;
			default: throw std::runtime_error("invalid mode");
		}
	}

    bool operator()(const std::function<getJobs_inserter_f>& insert) {
        switch (state) {
			case State::SacctCheckStart: {
				cmd(sacctSupported, {"sacct", {"--helpformat"}, {}, cmdopt::none});
				state = State::SacctCheckWaiting;
			}
			// fall through
			case State::SacctCheckWaiting: {
                if (sacctSupported.exit==not_finished) {
                    return false;
                } else if (sacctSupported.exit==0) {
					state=State::SacctStart;
					goto sacct;
                } else {
					state=State::ScontrolStart;
					goto scontrol;
				}
			}
			case State::SacctStart: {
				sacct:;
				std::vector<std::string> args{ "-X", "-P", "--format",  "ALL" };
				if (!stateFilter.empty()) {
					args.push_back("--state");
					args.push_back(stateFilter);
				}
				if (!filterJobs.empty()) {
					args.push_back("-j");
					args.push_back(internal::joinString(filterJobs.begin(), filterJobs.end(), ","));
				}
				cmd(jobs, {"sacct", args, {}, cmdopt::capture_stdout});
				state=State::SacctWaiting;
			}
			// fall through
			case State::SacctWaiting: {
				if (jobs.exit==not_finished) {
					return false;
				} else if (jobs.exit!=0) {
					throw CommandFailed("sacct -X -P --format ALL");
				}
				state=State::SacctDone;
			}
			// fall through
			case State::SacctDone: {
				Slurm::parseJobsSacct(jobs.out, insert);
				return true;
			}
			case State::ScontrolStart: {
				scontrol:;
				cmd(jobs, {"scontrol", {"show", "job", "--all"}, {}, cmdopt::capture_stdout});
				state=State::ScontrolWaiting;
			}
			// fall through
			case State::ScontrolWaiting: {
				if (jobs.exit==not_finished) {
					return false;
				} else if (jobs.exit!=0) {
					throw CommandFailed("scontrol show job --all");
				}
				state=State::ScontrolDone;
			}
			// fall through
			case State::ScontrolDone: {
				Slurm::parseJobsLegacy(jobs.out, insert);
				return true;
			}
			default: throw std::runtime_error("invalid state");
        }
    }
};

class GetBatchInfo {
private:
    cmd_f& cmd;
    Result version;
    Result config;
    enum class State {
        Starting,
        Waiting,
		Done,
    };
    State state = State::Starting;
public:
    GetBatchInfo(cmd_f& cmd_): cmd(cmd_) {}

    bool operator()(BatchInfo& info) {
        switch (state) {
			case State::Starting: {
				// start in parallel
				cmd(version, {"slurmd", {"--version"}, {}, cmdopt::capture_stdout});
				cmd(config, {"scontrol", {"show", "config"}, {}, cmdopt::capture_stdout});
				state = State::Waiting;
			}
			// fall through
			case State::Waiting: {
				if (version.exit>0 || config.exit>0) {
					throw CommandFailed("slurmd --version | scontrol show config");
				} else if (version.exit==0 && config.exit==0) {
					state = State::Done;
				} else {
					return false;
				}
			}
			// fall through
			case State::Done:
				info.name = std::string("slurm");
				info.version = trim_copy(version.out);
				info.info["config"] = trim_copy(config.out);
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};


std::function<getNodes_f> Slurm::getNodes(std::vector<std::string> filterNodes) { return GetNodes(_func, filterNodes); }
std::function<getJobs_f> Slurm::getJobs(std::vector<std::string> filterJobs) { return GetJobs(_func, getJobMode(), "PD,R,RQ,S", filterJobs); }
std::function<getQueues_f> Slurm::getQueues() { return GetQueues(_func); }
std::function<bool()> Slurm::rescheduleRunningJobInQueue(const std::string& job, bool force) { return RescheduleRunningJobInQueue(_func, job, force); }
std::function<bool()> Slurm::setQueueState(const std::string& name, QueueState state, bool force) { return SetQueueState(_func, name, state, force); }
std::function<bool()> Slurm::resumeJob(const std::string& job, bool force) { return ResumeJob(_func, job, force); }
std::function<bool()> Slurm::suspendJob(const std::string& job, bool force) { return SuspendJob(_func, job, force); }
std::function<bool()> Slurm::deleteJobByUser(const std::string& user, bool force) { return DeleteJobByUser(_func, user, force); }
std::function<bool()> Slurm::deleteJobById(const std::string& job, bool force) { return DeleteJobById(_func, job, force); }
std::function<bool()> Slurm::holdJob(const std::string& job, bool force) { return HoldJob(_func, job, force); }
std::function<bool()> Slurm::releaseJob(const std::string& job, bool force) { return ReleaseJob(_func, job, force); }
std::function<bool()> Slurm::setNodeComment(const std::string& name, bool force, const std::string& comment, bool appendComment) { return SetNodeComment(_func, name, force, comment, appendComment); }
std::function<bool()> Slurm::changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) { return ChangeNodeState(_func, name, state, force, reason, appendReason); }
std::function<runJob_f> Slurm::runJob(const JobOptions& opts) { return RunJob(_func, opts); }
std::function<bool(bool&)> Slurm::detect() { return Detect(_func); }
std::function<bool(bool&)> Slurm::checkSacct() { return CheckSacct(_func); }
std::function<bool(BatchInfo&)> Slurm::getBatchInfo() { return GetBatchInfo(_func); }

}
}
}