#include "batchsystem/slurm.h"
#include "batchsystem/error.h"

#include <iostream>
#include <sstream>
#include <numeric>
#include <regex>
#include <iomanip>
#include <cassert>

#include "batchsystem/internal/trim.h"
#include "batchsystem/internal/streamCast.h"
#include "batchsystem/internal/splitString.h"
#include "batchsystem/internal/timeToEpoch.h"
#include "batchsystem/internal/joinString.h"

using namespace cw::batch::internal;

namespace {

using namespace cw::batch::slurm;

const char* to_cstr(error type) {
  switch (type) {
      case error::sacct_X_P_format_ALL_failed: return "sacct -X -P --format ALL failed";
      case error::scontrol_show_job_all_failed: return "scontrol show job --all failed";
      case error::scontrol_version_failed: return "scontrol --version failed";
      case error::scontrol_show_config_failed: return "scontrol show config failed";
      case error::scontrol_update_NodeName_State_failed: return "scontrol update NodeName State failed";
      case error::scontrol_update_NodeName_Comment_failed: return "scontrol update NodeName Comment failed";
      case error::scontrol_release_failed: return "scontrol release failed";
      case error::scontrol_hold_failed: return "scontrol hold failed";
      case error::scancel_failed: return "scancel failed";
      case error::scancel_u_failed: return "scancel -u failed";
      case error::scontrol_suspend_failed: return "scontrol suspend failed";
      case error::scontrol_resume_failed: return "scontrol resume failed";
      case error::scontrol_update_PartitionName_State_failed: return "scontrol update PartitionName State failed";
      case error::scontrol_requeuehold_failed: return "scontrol requeuehold failed";
      case error::scontrol_requeue_failed: return "scontrol requeue failed";
      case error::scontrol_show_node_failed: return "scontrol show node failed";
      case error::scontrol_show_partition_all_failed: return "scontrol show partition --all failed";
      case error::sbatch_failed: return "sbatch failed";
      case error::slurm_job_mode_out_of_enum: return "slurm job mode invalid enum value";
      default: return "(unrecognized error)";
  }
}

struct ErrCategory : std::error_category
{

  const char* name() const noexcept {
    return "cw::batch::slurm";
  }
 
  std::string message(int ev) const {
    return to_cstr(static_cast<error>(ev));
  }

  std::error_condition default_error_condition(int err_value) const noexcept {
      switch (static_cast<error>(err_value)) {
        case error::sacct_X_P_format_ALL_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_show_job_all_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_version_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_show_config_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_update_NodeName_State_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_update_NodeName_Comment_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_release_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_hold_failed: return cw::batch::batch_condition::command_failed;
        case error::scancel_failed: return cw::batch::batch_condition::command_failed;
        case error::scancel_u_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_suspend_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_resume_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_update_PartitionName_State_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_requeuehold_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_requeue_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_show_node_failed: return cw::batch::batch_condition::command_failed;
        case error::scontrol_show_partition_all_failed: return cw::batch::batch_condition::command_failed;
        case error::sbatch_failed: return cw::batch::batch_condition::command_failed;
        case error::slurm_job_mode_out_of_enum: return cw::batch::batch_condition::invalid_argument;
        default: assert(false && "unknown error");
      }
  }
};

const ErrCategory error_cat {};

struct BatchInfoState {
	Result version;
    Result config;
};

cw::batch::JobState parseJobState(const std::string& str) {
	if(str=="PENDING")
	{
		return cw::batch::JobState::Queued;
	}
	else if(str=="RUNNING")
	{
		return cw::batch::JobState::Running;
	}
	else if(str=="SUSPENDED")
	{
		return cw::batch::JobState::Suspend;
	}
	else if(str=="COMPLETING")
	{
		return cw::batch::JobState::Terminating;
	}
	else if(str=="COMPLETED")
	{
		return cw::batch::JobState::Finished;
	}
	else if(str.rfind("CANCELLED", 0) == 0) // to handle "CANCELLED by 1000" (1000 is UID) syntax
	{
		return cw::batch::JobState::Cancelled;
	}
	else if(str=="FAILED")
	{
		return cw::batch::JobState::Failed;
	}
	else
	{
		return cw::batch::JobState::Unknown;
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

						int nodeIdStart=-1;
						int nodeIdEnd=-1;
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

const std::error_category& error_category() noexcept {
    return error_cat;
}

std::error_code make_error_code(error e) {
  return {static_cast<int>(e), error_cat};
}

const std::string Slurm::DefaultReason = "batchsystem_api";

Slurm::Slurm(cmd_f func): _cmd(func), _mode(job_mode::unchecked) {}

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

void Slurm::parseJobsLegacy(const std::string& output, std::vector<Job>& jobs) {
	Slurm::parseShowJob(output, [&jobs](std::map<std::string, std::string> j){
		Job job {};
		Slurm::jobMapToStruct(j, job);
		jobs.push_back(job);
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

void Slurm::parseJobsSacct(const std::string& output, std::vector<Job>& jobs) {
	Slurm::parseSacct(output, [&jobs](std::map<std::string, std::string> j){
		Job job {};
		Slurm::jobMapToStruct(j, job);
		jobs.push_back(job);
		return true;
	});
}

void Slurm::parseQueues(const std::string& output, std::vector<Queue>& queues) {
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
						if (!queues.push_back(std::move(queue))) return;
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

void Slurm::getNodes(std::vector<std::string> filterNodes, std::function<void(std::vector<Node> nodes, std::error_code ec)> cb) {
	std::vector<std::string> args{"show", "node"};
	if (filterNodes.empty()) {
		args.push_back("--all");
	} else {
		args.push_back(internal::joinString(filterNodes.begin(), filterNodes.end(), ","));
	}

	cmd({"scontrol", args, {}, cmdopt::capture_stdout}, [cb](auto res){
		std::vector<Node> nodes;
		if (!res.ec && res.exit == 0) Slurm::parseNodes(res.out, nodes);
		cb(nodes, res.exit != 0 ? error::scontrol_show_node_failed : res.ec);
	});
}

void Slurm::getJobsSacct(std::vector<std::string> filterJobs, std::string stateFilter, std::function<void(std::vector<Job> jobs, std::error_code ec)> cb) {
	std::vector<std::string> args{ "-X", "-P", "--format",  "ALL" };
	if (!stateFilter.empty()) {
		args.push_back("--state");
		args.push_back(stateFilter);
	}
	if (!filterJobs.empty()) {
		args.push_back("-j");
		args.push_back(internal::joinString(filterJobs.begin(), filterJobs.end(), ","));
	}
	_cmd({"sacct", args, {}, cmdopt::capture_stdout}, [cb](auto res){
		std::vector<Job> jobs;
		if (!res.ec && res.exit == 0) Slurm::parseJobsSacct(jobs.out, jobs);
		cb(queues, res.exit != 0 ? error::sacct_X_P_format_ALL_failed : res.ec);
	});
}
void Slurm::getJobsLegacy(std::function<void(std::vector<Job> jobs, std::error_code ec)> cb) {
	_cmd({"scontrol", {"show", "job", "--all"}, {}, cmdopt::capture_stdout}, [cb](auto res){
		std::vector<Job> jobs;
		if (!res.ec && res.exit == 0) Slurm::parseJobsLegacy(jobs.out, jobs);
		cb(queues, res.exit != 0 ? error::scontrol_show_job_all_failed : res.ec);
	});
}

void Slurm::getJobs(std::vector<std::string> filterJobs, std::function<void(std::vector<Job> jobs, std::error_code ec)> cb, std::string stateFilter="PD,R,RQ,S") {	
	switch (mode_) {
		case Slurm::job_mode::scontrol: getJobsLegacy(cb); break;
		case Slurm::job_mode::sacct: getJobsSacct(filterJobs, stateFilter, cb); break;
		case Slurm::job_mode::unchecked: {
			checkSacct([filterJobs, cb](bool has_sacct, auto ec){
				if (!ec) {
					if (has_sacct) {
						getJobsSacct(filterJobs, cb);
					} else {
						getJobsLegacy(cb);
					}
				}
			});
			break;
		}
		default: throw std::system_error(error::slurm_job_mode_out_of_enum);
	}
}

void Slurm::getQueues(std::function<void(std::vector<Queue> queues, std::error_code ec)> cb) {
	_cmd({"scontrol", {"show", "partition", "--all"}, {}, cmdopt::capture_stdout}, [cb](auto res){
		std::vector<Queue> queues;
		if (!res.ec && res.exit == 0) Slurm::parseQueues(res.out, queues);
		cb(queues, res.exit != 0 ? error::scontrol_show_partition_all_failed : res.ec);
	});
}

void Slurm::rescheduleRunningJobInQueue(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	_cmd({"scontrol", {force ? "requeuehold" : "requeue", job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? (force ? error::scontrol_requeuehold_failed : error::scontrol_requeue_failed) : res.ec);
	});
}

void Slurm::setQueueState(std::string name, QueueState state, bool force, std::function<void(std::error_code ec)> cb) {
	std::string stateStr;
	switch (state) {
		case QueueState::Unknown: throw std::system_error(cw::batch::error::queue_state_unknown_not_supported);
		case QueueState::Open: stateStr="UP"; break;
		case QueueState::Closed: stateStr="DOWN"; break;
		case QueueState::Inactive: stateStr="INACTIVE"; break;
		case QueueState::Draining: stateStr="DRAIN"; break;
		default: throw std::system_error(cw::batch::error::queue_state_out_of_enum);
	}
	cmd({"scontrol", {"update", "PartitionName=" + name, "State="+stateStr}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::scontrol_update_PartitionName_State_failed : res.ec);
	});
}

void Slurm::resumeJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	_cmd({"scontrol", {"resume", job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::scontrol_resume_failed : res.ec);
	});
}

void Slurm::suspendJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	_cmd({"scontrol", {"suspend", job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::scontrol_suspend_failed : res.ec);
	});
}

void Slurm::deleteJobByUser(std::string user, bool force, std::function<void(std::error_code ec)> cb) {
	_cmd({"scancel", {"-u", user}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::scancel_u_failed : res.ec);
	});
}

void Slurm::deleteJobById(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	_cmd({"scancel", {job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::scancel_failed : res.ec);
	});
}

void Slurm::holdJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	_cmd({"scontrol", {"hold", job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::scontrol_hold_failed : res.ec);
	});
}

void Slurm::releaseJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	_cmd({"scontrol", {"release", job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::scontrol_release_failed : res.ec);
	});
}

void Slurm::setNodeComment(const std::string& name, bool force, const std::string& comment, bool appendComment, std::function<void(std::error_code ec)> cb) {
	_cmd({"scontrol", {"update", "NodeName="+name, "Comment="+comment}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::scontrol_update_NodeName_Comment_failed : res.ec);
	});
}

void Slurm::changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason, std::function<void(std::error_code ec)> cb) {
	std::string stateString;
	switch (state) {
		case NodeChangeState::Resume: stateString="RESUME"; break;
		case NodeChangeState::Drain: stateString="DRAIN"; break;
		case NodeChangeState::Undrain: stateString="UNDRAIN"; break;
		default: throw std::system_error(cw::batch::error::node_change_state_out_of_enum);
	} 
	std::vector<std::string> args{"update", "NodeName=" + name, "State="+stateString};
	// add reason if needed and force default if empty as needed by slurm!
	if (nodeState != NodeChangeState::Resume) args.push_back("Reason="+(reason.empty() ? Slurm::DefaultReason : reason));

	_cmd({"scontrol", args, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::scontrol_update_NodeName_State_failed : res.ec);
	});
}

void Slurm::runJob(JobOptions opts, std::function<void(std::string jobName, std::error_code ec)> cb) {
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

	_cmd(c, [cb](auto res){
		cb((res.exit==0 && !res.ec) ? trim_copy(res.out) : "", res.exit != 0 ? error::sbatch_failed : res.ec);
	});
}

void Slurm::detect(std::function<void(bool has_batch, std::error_code ec)> cb) {
	_cmd({"sinfo", {"--version"}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit==0, {});
	});
}

void Slurm::checkSacct(std::function<void(bool has_sacct, std::error_code ec)> cb) {
	// use sacct --helpformat as sacct would list all jobs, sacct --helpformat would not fail if slurmdbd is not working
	_cmd({"sacct", {"--helpformat"}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit==0, {});
	});
}

void Slurm::getBatchInfo(std::function<void(BatchInfo, std::error_code ec)> cb) {
	auto state = std::shared_ptr<BatchInfoState>(new BatchInfoState());
	auto cb1 = [state, cb](){
		BatchInfo info;
		if (state->version.exit==0 && state->config.exit==0) {
			info.name = std::string("slurm");
			info.version = trim_copy(state->version.out);
			info.info["config"] = trim_copy(state->config.out);
			cb(info, {});
		} else if (state->version.exit>0) {
			cb(info, error::scontrol_version_failed);
			return;
		} else if (state->config.exit>0) {
			cb(info, error::scontrol_show_config_failed);
			return;
		}
	};
	_cmd({"sinfo", {"--version"}, {}, cmdopt::capture_stdout}, [state, cb1](auto res){
		state->version = res;
		cb1();
	});
	_cmd({"scontrol", {"show", "config"}, {}, cmdopt::capture_stdout}, [state, cb1](auto res){
		state->config = res;
		cb1();
	});
}

}
}
}