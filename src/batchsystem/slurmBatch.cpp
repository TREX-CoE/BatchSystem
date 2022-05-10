#include "batchsystem/slurmBatch.h"

#include <iostream>
#include <sstream>
#include <numeric>
#include <regex>
#include <iomanip>

#include "batchsystem/internal/runCommand.h"
#include "batchsystem/internal/trim.h"
#include "batchsystem/internal/streamCast.h"
#include "batchsystem/internal/splitString.h"
#include "batchsystem/internal/timeToEpoch.h"

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

const std::string SlurmBatch::DefaultReason = "batchsystem_api";

SlurmBatch::SlurmBatch(std::function<cmd_execute_f> func, bool useSacct): _func(func), _useSacct(useSacct) {}
SlurmBatch::SlurmBatch(std::function<cmd_execute_f> func): _func(func) {
	_useSacct = checkSacct();
}

bool SlurmBatch::checkSacct() const {
	// use sacct --helpformat as sacct would list all jobs, sacct --helpformat would not fail if slurmdbd is not working
	std::string out;
	return (_func(out, {"sacct", {"--helpformat"}}) == 0);
}

void SlurmBatch::useSacct(bool useSacct) {
	_useSacct = useSacct;
}

BatchInfo SlurmBatch::getBatchInfo() const {
	BatchInfo info;
	info.name = std::string("slurm");
	info.version = trim_copy(runCommand(_func, {"slurmd", {"--version"}}));
	info.info["config"] = trim_copy(runCommand(_func, {"scontrol", {"show", "config"}}));
	info.info["sacctWorks"] = checkSacct() ? "true" : "false";
	info.info["sacctUsed"] = _useSacct ? "true" : "false";
	return info;
}

void SlurmBatch::parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert) {
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

CmdOptions SlurmBatch::getNodesCmd() {
	return {"scontrol", {"show", "node", "--all"}};
}
void SlurmBatch::getNodes(const std::vector<std::string>& filterNodes, std::function<getNodes_inserter_f> insert) const {
	CmdOptions opts{"scontrol", {"show"}};
	if (filterNodes.empty()) {
		opts.args.push_back(std::accumulate(filterNodes.begin(), filterNodes.end(), std::string(), [](const std::string &s1, const std::string &s2){
			// join string with , separator
			return s1.empty() ? s2 : (s1 + "," + s2);
		}));
	}
	parseNodes(runCommand(_func, opts), insert);
}
void SlurmBatch::getNodes(std::function<getNodes_inserter_f> insert) const {
	parseNodes(runCommand(_func, getNodesCmd()), insert);
}


void SlurmBatch::getJobs(std::function<getJobs_inserter_f> insert) const {
	if (_useSacct) {
		getJobsSacct(insert); 
	} else {
		getJobsLegacy(insert); 
	}
}

void SlurmBatch::parseShowJob(const std::string& output, std::function<bool(std::map<std::string, std::string>)> insert) {
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

void SlurmBatch::jobMapToStruct(const std::map<std::string, std::string>& j, Job& job) {
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

void SlurmBatch::parseJobsLegacy(const std::string& output, std::function<getJobs_inserter_f> insert) {
	SlurmBatch::parseShowJob(output, [&insert](std::map<std::string, std::string> j){
		Job job {};
		SlurmBatch::jobMapToStruct(j, job);
		if (!insert(job)) return false;
		return true;
	});
}

void SlurmBatch::getJobsLegacy(std::function<getJobs_inserter_f> insert) const {
	parseJobsLegacy(runCommand(_func, {"scontrol", {"show", "job", "--all"}}), insert);
}

std::string SlurmBatch::runJob(const JobOptions& opts) const {
	std::vector<std::string> args{"--parsable"}; // --parsable to only print job id
	if (opts.numberNodes.has_value()) {
		args.push_back("-N");
		args.push_back(opts.numberNodesMax.has_value() ? std::to_string(opts.numberNodes.get()) : (std::to_string(opts.numberNodes.get())+"-"+std::to_string(opts.numberNodesMax.get())));
	}
	if (opts.numberTasks.has_value()) {
		args.push_back("-n");
		args.push_back(std::to_string(opts.numberTasks.get()));
	}
	if (opts.numberGpus.has_value()) {
		args.push_back("-G");
		args.push_back(std::to_string(opts.numberGpus.get()));
	}
	args.push_back(opts.path.get());
	return trim_copy(runCommand(_func, {"sbatch", args}));
}

void SlurmBatch::getJobsSacct(std::function<getJobs_inserter_f> insert) const {
	getJobsSacct(insert, "PD,R,RQ,S"); // show only active jobs
}

void SlurmBatch::parseSacct(const std::string& output, std::function<bool(std::map<std::string, std::string>)> insert) {

	std::stringstream commandResult(output);
	std::stringstream buffer;
	std::string tmp, name, value, jobid;

	std::string line;

	// parse header (skip empty lines)
	while (line.empty()) getline(commandResult, line);


	std::vector<std::string> header;
	splitString(line, "|", [&](size_t start, size_t end) {
		header.push_back(line.substr(start, end));
		return true;
	});
	
	while(commandResult.good()) {
		std::map<std::string, std::string> job;
		getline(commandResult, line);
		if (line.empty()) continue;
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

void SlurmBatch::parseJobsSacct(const std::string& output, std::function<getJobs_inserter_f> insert) {
	SlurmBatch::parseSacct(output, [&insert](std::map<std::string, std::string> j){
		Job job {};
		SlurmBatch::jobMapToStruct(j, job);
		if (!insert(job)) return false;
		return true;
	});
}

void SlurmBatch::getJobsSacct(std::function<getJobs_inserter_f> insert, const std::string& stateFilter) const {
	std::vector<std::string> args{
			"-X",
			"-P",
			"--format", 
			"ALL", 
	};
	if (!stateFilter.empty()) {
		args.push_back("--state");
		args.push_back(stateFilter);
	}
	parseJobsSacct(runCommand(_func, {"sacct", args}), insert);
}

void SlurmBatch::parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert) {
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

CmdOptions SlurmBatch::getQueuesCmd() {
	return {"scontrol", {"show", "partition", "--all"}};
}
void SlurmBatch::getQueues(std::function<getQueues_inserter_f> insert) const {
	parseQueues(runCommand(_func, getQueuesCmd()), insert);
}


void SlurmBatch::changeNodeState(const std::string& name, NodeChangeState state, bool, const std::string& reason, bool) const {

	std::string stateString;
	switch (state) {
		case NodeChangeState::Resume: stateString="RESUME"; break;
		case NodeChangeState::Drain: stateString="DRAIN"; break;
		case NodeChangeState::Undrain: stateString="UNDRAIN"; break;
		default: throw std::runtime_error("invalid state"); 
	} 
	std::vector<std::string> args{"update", "NodeName=" + name, "State="+stateString};
	// add reason if needed and force default if empty as needed by slurm!
	if (state != NodeChangeState::Resume) args.push_back("Reason="+(reason.empty() ? SlurmBatch::DefaultReason : reason));
	runCommand(_func, {"scontrol", args});
}

void SlurmBatch::setNodeComment(const std::string& name, bool, const std::string& comment, bool) const {
	runCommand(_func, {"scontrol", {"update", "NodeName="+name, "Comment="+comment}});
}

void SlurmBatch::releaseJob(const std::string& job, bool) const {
	runCommand(_func, {"scontrol", {"release", job}});
}
void SlurmBatch::holdJob(const std::string& job, bool) const {
	runCommand(_func, {"scontrol", {"hold", job}});
}
void SlurmBatch::deleteJobById(const std::string& job, bool) const {
	runCommand(_func, {"scancel", {job}});
}
void SlurmBatch::deleteJobByUser(const std::string& user, bool) const {
	runCommand(_func, {"scancel", {"-u", user}});
}
void SlurmBatch::suspendJob(const std::string& job, bool) const {
	runCommand(_func, {"scontrol", {"suspend", job}});
}
void SlurmBatch::resumeJob(const std::string& job, bool) const {
	runCommand(_func, {"scontrol", {"resume", job}});
}

void SlurmBatch::setQueueState(const std::string& name, QueueState state, bool) const {
	std::string stateStr;
	switch (state) {
		case QueueState::Unknown: throw std::runtime_error("unknown state");
		case QueueState::Open: stateStr="UP"; break;
		case QueueState::Closed: stateStr="DOWN"; break;
		case QueueState::Inactive: stateStr="INACTIVE"; break;
		case QueueState::Draining: stateStr="DRAIN"; break;
		default: throw std::runtime_error("unknown state");
	}
	runCommand(_func, {"scontrol", {"update", "PartitionName=" + name, "State="+stateStr}});
}

void SlurmBatch::rescheduleRunningJobInQueue(const std::string& job, bool hold) const {
	runCommand(_func, {"scontrol", {hold ? "requeuehold" : "requeue", job}});
}

}
}
}