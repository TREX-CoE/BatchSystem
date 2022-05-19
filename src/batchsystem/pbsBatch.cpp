#include "batchsystem/pbsBatch.h"

#include <iostream>
#include <sstream>
#include <vector>

#include <libxml++/libxml++.h>

#include "batchsystem/internal/runCommand.h"
#include "batchsystem/internal/trim.h"
#include "batchsystem/internal/streamCast.h"
#include "batchsystem/internal/splitString.h"
#include "batchsystem/internal/timeToEpoch.h"
#include "batchsystem/internal/byteParse.h"

using namespace cw::batch;
using namespace cw::batch::internal;

namespace {

const CmdOptions optsDetect{"pbs-config", {"--version"}};
const CmdOptions optsVersion{"pbsnodes", {"--version"}};
const CmdOptions optsGetQueues{"qstat", {"-Qf"}};
const CmdOptions optsGetJobs{"qstat", {"-f", "-x"}};
const CmdOptions optsGetNodes{"pbsnodes", {"-x"}};

void toLowerCase(std::string& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
}

std::string xml_node_to_str(const xmlpp::Node* node) {
    std::string s;
    for (const auto& leaf : node->get_children()) {
        const auto nodeContent = dynamic_cast<const xmlpp::ContentNode*>(leaf);
        if (nodeContent) s += nodeContent->get_content().raw();
    }
    return s;
}

bool str_contains(const std::string& s, const std::string& part) {
	return s.find(part) != std::string::npos;
}

}

namespace cw {
namespace batch {
namespace pbs {

PbsBatch::PbsBatch(std::function<cmd_execute_f> func): _func(func) {}

void PbsBatch::resetCache() {
	_cache.clear();
}

std::string PbsBatch::runJob(const JobOptions& opts) const {
	std::vector<std::string> args;
	if (opts.numberNodes.has_value()) {
		args.push_back("-l");
		args.push_back("nodes="+std::to_string(opts.numberNodes.get()));
	}
	if (opts.numberTasks.has_value()) {
		args.push_back("-l");
		args.push_back("procs="+std::to_string(opts.numberTasks.get()));
	}
	if (opts.numberGpus.has_value()) {
		args.push_back("-l");
		args.push_back("gpus="+std::to_string(opts.numberGpus.get()));
	}
	args.push_back(opts.path.get());
	return trim_copy(runCommand(_func, {"qsub", args}));

}

bool PbsBatch::getBatchInfo(BatchInfo& info) {
	std::string out;
	int ret = _func(out, optsVersion);
	if (ret == -1) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		info.name = std::string("pbs");
		info.version = trim_copy(out);
		return true;
	}
}

bool PbsBatch::detect(bool& detected) {
	std::string out;
	int ret = _func(out, optsDetect);
	if (ret == -1) {
		return false;
	} else if (ret > 0) {
		detected = false;
		return true;
	} else {
		detected = true;
		return true;
	}
}

void PbsBatch::parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert) {
	xmlpp::DomParser parser;
	std::string s(output);
	ltrim(s); // skip empty lines etc.

	if (s.rfind("pbsnodes: Unknown node"/*  MSG=cannot locate specified node*/) == 0) {
		// just ignore missing node (-> user checks insert function calls to see if node found, more uniform way other batchsystems use this)
		return;
	}
	parser.parse_memory(s);
	const auto root = parser.get_document()->get_root_node();
	const auto nodes = root->find("/Data/Node");

	for (const auto& node : nodes)
	{
		Node batchNode = {};
		for (const auto& child : node->get_children()) {
			const auto name = child->get_name();
			if(name=="name")
			{
				batchNode.nameFull = xml_node_to_str(child);
				batchNode.name=batchNode.nameFull.get().substr(0,batchNode.nameFull.get().find('.'));
			}
			else if(name=="jobs")
			{
				std::string joblist = xml_node_to_str(child);
				uint32_t count = 0;
				splitString(joblist, ",", [&](size_t start, size_t end) {
					std::string jobinfo=trim_copy(joblist.substr(start, end));
					jobinfo=jobinfo.substr(0,jobinfo.find('.'));

					std::string jobId=jobinfo;
					jobId=jobId.substr(jobId.find('/')+1,jobId.length());

					batchNode.cpusOfJobs[jobId]++;
					++count;
					return true;
				});
				batchNode.cpusUsed=count;
			}
			else if(name=="state")
			{
				std::string stateList = xml_node_to_str(child);
				batchNode.rawState = stateList;
				toLowerCase(stateList);

				if(str_contains(stateList, "down"))
				{
					batchNode.state = NodeState::Offline;
				}
				else if(str_contains(stateList, "offline") || str_contains(stateList, "state-unknown") || str_contains(stateList, "stale") || str_contains(stateList, "provisioning") || str_contains(stateList, "wait-provisioning"))
				{
					batchNode.state = NodeState::Disabled;
				}
				else if(str_contains(stateList, "free") || str_contains(stateList, "job-exclusive") || str_contains(stateList, "job-busy") || str_contains(stateList, "busy") || str_contains(stateList, "resv-exclusive"))
				{
					batchNode.state = NodeState::Online;
				}
				else if(str_contains(stateList, "<various>"))
				{
					batchNode.state = NodeState::Various;
				}
			}
			else if(name=="np")
			{
				stream_cast(xml_node_to_str(child), batchNode.cpus.get());
			}
			else if(name=="note")
			{
				batchNode.comment = xml_node_to_str(child);
			}
			
		}

		if (!insert(batchNode)) return;
	}
}

bool PbsBatch::getNodes(const std::vector<std::string>& filterNodes, std::function<getNodes_inserter_f> insert) {
	CmdOptions opts = optsGetNodes;
	for (const auto& n : filterNodes) opts.args.push_back(n);

	std::string out;
	int ret = _func(out, opts);
	if (ret == -1) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		parseNodes(out, insert);
		return true;
	}
}

bool PbsBatch::getQueues(std::function<getQueues_inserter_f> insert) {
	std::string out;
	int ret = _func(out, optsGetQueues);
	if (ret == -1) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		parseQueues(out, insert);
		return true;
	}
}



// see man pbs_job_attributes 
void PbsBatch::parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert) {
	xmlpp::DomParser parser;
	parser.parse_memory(output);
	const auto root = parser.get_document()->get_root_node();
	const auto nodes = root->find("/Data/Job");

	for (const auto& node : nodes) {
		Job job = {};

		for (const auto& child : node->get_children()) {
			const auto name = child->get_name();
			if(name=="Job_Id")
			{
				job.id=xml_node_to_str(child);
				job.idCompact=job.id.get().substr(0,job.id.get().find('.'));
			}
			else if(name=="Resource_List")
			{
				for (const auto& resourceChild : child->get_children()) {
					const auto resourceName = resourceChild->get_name();
					if (resourceName=="nodes") {
						const auto text = xml_node_to_str(resourceChild);

						std::string neednode=text.substr(0,text.find(':'));
						std::string ppn=text.substr(text.find('=')+1,text.length());

						stream_cast(neednode, job.nodesRequested.get());
						stream_cast(ppn, job.cpusPerNode.get());
						if (job.nodesRequested.has_value() && job.cpusPerNode.has_value()) job.cpus = job.nodesRequested.get() * job.cpusPerNode.get();
					} else if (resourceName=="nodect") {
						stream_cast(xml_node_to_str(resourceChild), job.nodes.get());
					} else if (resourceName=="nice") {
						stream_cast(xml_node_to_str(resourceChild), job.niceRequested.get());
					} else if (resourceName=="neednodes") {
						stream_cast(xml_node_to_str(resourceChild), job.nodesRequested.get());
					} else if (resourceName=="gres") {
						const auto gres = xml_node_to_str(resourceChild);
						splitString(gres, ",", [&](size_t start, size_t end){
							std::string entry = gres.substr(start, end); 
							auto pos = entry.find(":");
							if (pos != std::string::npos) {
								job.requestedGeneral[entry.substr(0, pos)] = entry.substr(pos+1);
							}
							return true;
						});
					} else if (resourceName=="other") {
						job.otherRequested = xml_node_to_str(resourceChild);
					} else if (resourceName=="walltime") {
						timeToEpoch(xml_node_to_str(resourceChild), job.wallTimeRequested.get(), "%H:%M:%S");
					}
				}
			}
			else if(name=="resources_used")
			{
				for (const auto& resourceChild : child->get_children()) {
					const auto resourceName = resourceChild->get_name();
					if (resourceName=="walltime") {
						timeToEpoch(xml_node_to_str(resourceChild), job.wallTimeUsed.get(), "%H:%M:%S");
					} else if (resourceName=="cput") {
						timeToEpoch(xml_node_to_str(resourceChild), job.cpuTimeUsed.get(), "%H:%M:%S");
					} else if (resourceName=="mem") {
						si_byte_parse(xml_node_to_str(resourceChild), job.memUsed.get());
					} else if (resourceName=="vmem") {
						si_byte_parse(xml_node_to_str(resourceChild), job.vmemUsed.get());
					}
				}
			}
			else if(name=="Job_Name")
			{
				job.name = xml_node_to_str(child);
			}
			else if(name=="Job_Owner")
			{
				job.owner = xml_node_to_str(child);
			}
			else if(name=="euser")
			{
				job.user = xml_node_to_str(child);
			}
			else if(name=="egroup")
			{
				job.group = xml_node_to_str(child);
			}
			else if(name=="Priority")
			{
				stream_cast(xml_node_to_str(child), job.priority.get());
			}
			else if(name=="job_state")
			{
				std::string text = xml_node_to_str(child);
				job.rawState = text;
				toLowerCase(text);
				if(text=="r" || text=="h")
				{
					job.state = JobState::Running;
				}
				else if(text=="q" || text=="s" || text=="u")
				{
					job.state = JobState::Queued;
				}
				else if(text=="e" || text=="w")
				{
					job.state = JobState::Terminating;
				}
				else if(text=="t")
				{
					job.state = JobState::Moved;
				}
				else if(text=="s")
				{
					job.state = JobState::Suspend;
				}
				else
				{
					job.state = JobState::Unknown;
				}
			}
			else if(name=="queue")
			{
				job.queue = xml_node_to_str(child);
			}
			else if(name=="ctime")
			{
				stream_cast(xml_node_to_str(child), job.submitTime.get());
			}
			else if(name=="qtime")
			{
				stream_cast(xml_node_to_str(child), job.queueTime.get());
			}
			else if(name=="etime")
			{
				stream_cast(xml_node_to_str(child), job.endTime.get());
			}
			else if(name=="start_time")
			{
				stream_cast(xml_node_to_str(child), job.startTime.get());
			}
			else if(name=="Variable_List")
			{
				const auto variableList = xml_node_to_str(child);
				splitString(variableList, ",", [&](size_t start, size_t end){
					std::string entry = variableList.substr(start, end); 
					auto pos = entry.find("=");
					if (pos != std::string::npos) {
						job.variableList[entry.substr(0, pos)] = entry.substr(pos+1);
					}
					return true;
				});
			}
			else if(name=="exec_host")
			{
				job.execHost = xml_node_to_str(child);
			}
			else if(name=="submit_host")
			{
				job.submitHost = xml_node_to_str(child);
			}
			else if(name=="server")
			{
				job.server = xml_node_to_str(child);
			}
			else if(name=="init_work_dir")
			{
				job.workdir = xml_node_to_str(child);
			}
			else if(name=="submit_args")
			{
				job.submitArgs = xml_node_to_str(child);
			}
		}
		if (!insert(job)) return;
	}
}

bool PbsBatch::getJobs(std::function<getJobs_inserter_f> insert) {
	std::string out;
	int ret = _func(out, optsGetJobs);
	if (ret == -1) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		parseJobs(out, insert);
		return true;
	}
}



void PbsBatch::parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert) {
	std::stringstream commandResult(output);

	std::string tmp, queueType;
	Queue queue {};
	while(commandResult.good())
	{
		getline(commandResult,tmp);
		tmp=trim_copy(std::string(tmp));

		size_t pos=tmp.find('=');

		bool enabled = false;
		bool started = false;

		if(tmp.substr(0,6)=="Queue:")
		{
			if (queueType=="Execution") {
				queue.rawState = std::string("enabled=")+(enabled ? "True":"False")+",started="+(started ? "True":"False");
				if(enabled && started)
				{
					queue.state = QueueState::Open;
				}
				else if(enabled && !started)
				{
					queue.state = QueueState::Inactive;
				}
				else if(!enabled && started)
				{
					queue.state = QueueState::Draining;
				}
				else if(!enabled && !started)
				{
					queue.state = QueueState::Closed;
				}

				if (!insert(queue)) return;
				queue = Queue{}; // reset
			}
			queue.name=trim_copy(tmp.substr(6));
			queueType="";
		}
		else if(pos!=tmp.npos)
		{
			std::string name=trim_copy(tmp.substr(0,pos));
			std::string value=trim_copy(tmp.substr(pos+1));


			if(!queue.name.get().empty())
			{
				if(name=="queue_type")
				{
					queueType=value;
				}


				if(queueType=="Execution")
				{
					if(name=="enabled")
					{
						if(value=="True")
						{
							enabled = true;
						}
						else if(value=="False")
						{
							enabled = false;
						}
					}
					else if(name=="started")
					{
						if(value=="True")
						{
							started = true;
						}
						else if(value=="False")
						{
							started = false;
						}
					}
					else if(name=="Priority")
					{
						stream_cast(value, queue.priority.get());
					}
					else if(name=="resources_assigned.ncpus" || name=="resources_available.ncpus")
					{
						stream_cast(value, queue.cpus.get());
					}
					else if(name=="resources_assigned.nodect" || name=="resources_available.nodect")
					{
						stream_cast(value, queue.nodesTotal.get());
					}
					else if(name=="total_jobs")
					{
						stream_cast(value, queue.jobsTotal.get());
					}
					else if(name=="mtime")
					{
						stream_cast(value, queue.modifyTime.get());
					}
					else if(name=="resources_max.nodect")
					{
						stream_cast(value, queue.nodesMax.get());
					}
					else if(name=="resources_min.nodect")
					{
						stream_cast(value, queue.nodesMin.get());
					}
					else if(name=="resources_max.walltime")
					{
						timeToEpoch(value, queue.wallTimeMax.get(), "%H:%M:%S");
					}
					else if(name=="resources_max.pvmem")
					{
						si_byte_parse(value, queue.memPerCpuMax.get());
					}
				}
			}
		}
	}
}

void PbsBatch::setNodeComment(const std::string& name, bool, const std::string& comment, bool appendComment) const {
	runCommand(_func, {"pbsnodes", {name, appendComment ? "-A" : "-N", comment}});
}

void PbsBatch::setQueueState(const std::string& name, QueueState state, bool) const {
	bool enabled = false;
	bool started = false;
	switch (state) {
		case QueueState::Unknown: throw std::runtime_error("unknown state");
		case QueueState::Open: enabled=true; started=true; break;
		case QueueState::Closed: enabled=false; started=false; break;
		case QueueState::Inactive: enabled=true; started=false; break;
		case QueueState::Draining: enabled=false; started=true; break;
		default: throw std::runtime_error("invalid state"); break;
	}
	runCommand(_func, {"qmgr", {"-c", "set queue " + name + " enabled="+(enabled ? "true" : "false") + ",started="+(started ? "true" : "false")}});
}

void PbsBatch::changeNodeState(const std::string& name, NodeChangeState state, bool, const std::string& reason, bool appendReason) const {
	std::string stateArg;
	switch (state) {
		case NodeChangeState::Resume: stateArg="-r"; break;
		case NodeChangeState::Drain: stateArg="-o"; break;
		case NodeChangeState::Undrain: stateArg="-c"; break;
		default: throw std::runtime_error("invalid state");
	}
	runCommand(_func, {"pbsnodes", {stateArg, name, appendReason ? "-A" : "-N", reason}});
}
void PbsBatch::releaseJob(const std::string& job, bool) const {
	runCommand(_func, {"qrls", {job}});
}
void PbsBatch::holdJob(const std::string& job, bool) const {
	runCommand(_func, {"qhold", {job}});
}
void PbsBatch::suspendJob(const std::string& job, bool) const {
	runCommand(_func, {"qsig", {"-s", "suspend", job}});
}
void PbsBatch::resumeJob(const std::string& job, bool) const {
	runCommand(_func, {"qsig", {"-s", "resume", job}});
}
void PbsBatch::getJobsByUser(const std::string& user, std::function<bool(std::string)> inserter) const {
	std::string userjobsStr = runCommand(_func, {"qselect", {"-u", user}});
	splitString(userjobsStr, "\n", [&](size_t start, size_t end) {
		std::string job=trim_copy(userjobsStr.substr(start, end));
		if (!job.empty()) {
			if (!inserter(job)) return false;
		}
		return true;
	});
}

void PbsBatch::deleteJobByUser(const std::string& user, bool force) const {
	std::vector<std::string> args;
	if (force) args.push_back("-p"); // purge forces job to be deleted

	// qdel -u only supported by SGE https://stackoverflow.com/questions/28857807/use-qdel-to-delete-all-my-jobs-at-once-not-one-at-a-time
	// instead manually query jobs of user first and then delete them
	getJobsByUser(user, [&](std::string job){args.push_back(job);return true;});
	runCommand(_func, {"qdel", args});
}
void PbsBatch::deleteJobById(const std::string& job, bool force) const {
	std::vector<std::string> args;
	if (force) args.push_back("-p"); // purge forces job to be deleted
	args.push_back(job);
	runCommand(_func, {"qdel", args});
}
void PbsBatch::rescheduleRunningJobInQueue(const std::string& name, bool force) const {
	std::vector<std::string> args;
	if (force) args.push_back("-f");
	args.push_back(name);
	runCommand(_func, {"qrerun", args});
}

}
}
}
