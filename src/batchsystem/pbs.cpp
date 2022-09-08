#include "batchsystem/pbs.h"
#include "batchsystem/error.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
#include <libxml++/libxml++.h>

#include "batchsystem/internal/trim.h"
#include "batchsystem/internal/streamCast.h"
#include "batchsystem/internal/splitString.h"
#include "batchsystem/internal/timeToEpoch.h"
#include "batchsystem/internal/byteParse.h"

using namespace cw::batch::internal;

namespace {

using namespace cw::batch::pbs;

const char* to_cstr(error type) {
  switch (type) {
      case error::pbsnodes_x_xml_parse_error: return "error while parsing pbsnodes -x xml output";
      case error::qselect_u_failed: return "qselect -u failed";
      case error::qdel_failed: return "qdel failed";
      case error::qstat_f_x_xml_parse_error: return "error while parsing qstat -f -x xml output";
      case error::pbsnodes_change_node_state_failed: return "pbsnodes node state change failed";
      case error::pbsnodes_set_comment_failed: return "pbsnodes set comment failed";
      case error::qrls_failed: return "qrls failed";
      case error::qhold_failed: return "qhold failed";
      case error::qsig_s_suspend_failed: return "qsig -s suspend failed";
      case error::qsig_s_resume_failed: return "qsig -s resume failed";
      case error::qmgr_c_set_queue_failed: return "qmgr -c set queue failed";
      case error::qrerun_failed: return "qrerun failed";
      case error::pbsnodes_x_failed: return "pbsnodes -x failed";
      case error::qstat_Qf_failed: return "qstat -Qf failed";
      case error::qsub_failed: return "qsub failed";
      case error::qstat_f_x_failed: return "qstat -f -x failed";
      case error::pbsnodes_version_failed: return "pbsnodes --version failed";
      default: return "(unrecognized error)";
  }
}

struct ErrCategory : std::error_category
{

  const char* name() const noexcept {
    return "cw::batch::pbs";
  }
 
  std::string message(int ev) const {
    return to_cstr(static_cast<error>(ev));
  }

  std::error_condition default_error_condition(int err_value) const noexcept {
      switch (static_cast<error>(err_value)) {
        case error::pbsnodes_x_xml_parse_error: return cw::batch::batch_condition::command_failed;
        case error::qselect_u_failed: return cw::batch::batch_condition::command_failed;
        case error::qdel_failed: return cw::batch::batch_condition::command_failed;
        case error::qstat_f_x_xml_parse_error: return cw::batch::batch_condition::command_failed;
        case error::pbsnodes_change_node_state_failed: return cw::batch::batch_condition::command_failed;
        case error::pbsnodes_set_comment_failed: return cw::batch::batch_condition::command_failed;
        case error::qrls_failed: return cw::batch::batch_condition::command_failed;
        case error::qhold_failed: return cw::batch::batch_condition::command_failed;
        case error::qsig_s_suspend_failed: return cw::batch::batch_condition::command_failed;
        case error::qsig_s_resume_failed: return cw::batch::batch_condition::command_failed;
        case error::qmgr_c_set_queue_failed: return cw::batch::batch_condition::command_failed;
        case error::qrerun_failed: return cw::batch::batch_condition::command_failed;
        case error::pbsnodes_x_failed: return cw::batch::batch_condition::command_failed;
        case error::qstat_Qf_failed: return cw::batch::batch_condition::command_failed;
        case error::qsub_failed: return cw::batch::batch_condition::command_failed;
        case error::qstat_f_x_failed: return cw::batch::batch_condition::command_failed;
        case error::pbsnodes_version_failed: return cw::batch::batch_condition::command_failed;
        default: return cw::batch::batch_condition::command_failed;
      }
  }
};

const ErrCategory error_cat {};

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

const std::error_category& error_category() noexcept {
    return error_cat;
}

std::error_code make_error_code(error e) {
  return {static_cast<int>(e), error_cat};
}

Pbs::Pbs(cmd_f func): _cmd(func) {}

const char* Pbs::name() const { return "pbs"; }

bool Pbs::getNodes(supported_t) { return true; }
bool Pbs::getQueues(supported_t) { return true; }
bool Pbs::getJobs(supported_t) { return true; }
bool Pbs::getBatchInfo(supported_t) { return true; }
bool Pbs::deleteJobById(supported_t) { return true; }
bool Pbs::deleteJobByUser(supported_t) { return true; }
bool Pbs::changeNodeState(supported_t) { return true; }
bool Pbs::setQueueState(supported_t) { return true; }
bool Pbs::runJob(supported_t) { return true; }
bool Pbs::setNodeComment(supported_t) { return true; }
bool Pbs::holdJob(supported_t) { return true; }
bool Pbs::releaseJob(supported_t) { return true; }
bool Pbs::suspendJob(supported_t) { return true; }
bool Pbs::resumeJob(supported_t) { return true; }
bool Pbs::rescheduleRunningJobInQueue(supported_t) { return true; }


void Pbs::parseNodes(const std::string& output, std::vector<Node>& nodes) {
	xmlpp::DomParser parser;
	std::string s(output);
	ltrim(s); // skip empty lines etc.

	if (s.rfind("pbsnodes: Unknown node"/*  MSG=cannot locate specified node*/) == 0) {
		// just ignore missing node (-> user checks insert function calls to see if node found, more uniform way other batchsystems use this)
		return;
	}
	try {
		parser.parse_memory(s);
	} catch (const xmlpp::parse_error& e) {
		throw std::system_error(error::pbsnodes_x_xml_parse_error, e.what());
	}
	const auto root = parser.get_document()->get_root_node();
	const auto nodes_elems = root->find("/Data/Node");

	for (const auto& node : nodes_elems)
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

		nodes.push_back(batchNode);
	}
}

// see man pbs_job_attributes 
void Pbs::parseJobs(const std::string& output, std::vector<Job>& jobs) {
	xmlpp::DomParser parser;

	try {
		parser.parse_memory(output);
	} catch (const xmlpp::parse_error& e) {
		throw std::system_error(error::qstat_f_x_xml_parse_error, e.what());
	}
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
		jobs.push_back(job);
	}
}



void Pbs::parseQueues(const std::string& output, std::vector<Queue>& queues) {
	std::stringstream commandResult(output);

	std::string tmp, queueType;
	Queue queue {};
	bool enabled = false;
	bool started = false;

	auto insertQueue = [&](){
		if (queueType=="Execution") {
			queue.rawState = std::string("enabled=")+(enabled ? "True":"False")+",started="+(started ? "True":"False");
			if(enabled && started)
			{
				queue.state=QueueState::Open;
			}
			else if(enabled && !started)
			{
				queue.state=QueueState::Inactive;
			}
			else if(!enabled && started)
			{
				queue.state=QueueState::Draining;
			}
			else if(!enabled && !started)
			{
				queue.state=QueueState::Closed;
			}
			queues.push_back(queue);
		}
	};

	while(commandResult.good())
	{
		getline(commandResult,tmp);
		tmp=trim_copy(std::string(tmp));

		size_t pos=tmp.find('=');

		if(tmp.substr(0,6)=="Queue:")
		{
			// insert last queue as new starts
			insertQueue();
			queue = Queue{}; // reset
			enabled = false;
			started = false;
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
	insertQueue();
}

void Pbs::getNodes(std::vector<std::string> filterNodes, std::function<void(std::vector<Node> nodes, std::error_code ec)> cb) {
	std::vector<std::string> args{"-x"};
	for (const auto& n : filterNodes) args.push_back(n);
	_cmd({"pbsnodes", args, {}, cmdopt::capture_stdout}, [cb](auto res){
		std::vector<Node> nodes;
		if (!res.ec && res.exit == 0) Pbs::parseNodes(res.out, nodes);
		cb(nodes, res.exit != 0 ? error::pbsnodes_x_failed : res.ec);
	});
}

void Pbs::getJobs(std::vector<std::string> filterJobs, std::function<void(std::vector<Job> jobs, std::error_code ec)> cb) {
	(void)filterJobs;	
	_cmd({"qstat", {"-f", "-x"}, {}, cmdopt::capture_stdout}, [cb](auto res){
		std::vector<Job> jobs;
		if (!res.ec && res.exit == 0) Pbs::parseJobs(res.out, jobs);
		cb(jobs, res.exit != 0 ? error::qstat_f_x_failed : res.ec);
	});
}

void Pbs::getQueues(std::vector<std::string> filterQueues, std::function<void(std::vector<Queue> queues, std::error_code ec)> cb) {
	(void)filterQueues;
	_cmd({"qstat", {"-Qf"}, {}, cmdopt::capture_stdout}, [cb](auto res){
		std::vector<Queue> queues;
		if (!res.ec && res.exit == 0) Pbs::parseQueues(res.out, queues);
		cb(queues, res.exit != 0 ? error::qstat_Qf_failed : res.ec);
	});
}

void Pbs::rescheduleRunningJobInQueue(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	std::vector<std::string> args;
	if (force) args.push_back("-f");
	args.push_back(job);
	_cmd({"qrerun", args, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::qrerun_failed : res.ec);
	});
}

void Pbs::setQueueState(std::string name, QueueState state, bool force, std::function<void(std::error_code ec)> cb) {
	(void)force;
	bool enabled = false;
	bool started = false;
	switch (state) {
		case QueueState::Unknown: throw std::system_error(cw::batch::error::queue_state_unknown_not_supported);
		case QueueState::Open: enabled=true; started=true; break;
		case QueueState::Closed: enabled=false; started=false; break;
		case QueueState::Inactive: enabled=true; started=false; break;
		case QueueState::Draining: enabled=false; started=true; break;
		default: throw std::system_error(cw::batch::error::queue_state_out_of_enum);
	}

	_cmd({"qmgr", {"-c", "set queue " + name + " enabled="+(enabled ? "true" : "false") + ",started="+(started ? "true" : "false")}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::qmgr_c_set_queue_failed : res.ec);
	});
}

void Pbs::resumeJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	(void)force;
	_cmd({"qsig", {"-s", "resume", job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::qsig_s_resume_failed : res.ec);
	});
}

void Pbs::suspendJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	(void)force;
	_cmd({"qsig", {"-s", "suspend", job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::qsig_s_suspend_failed : res.ec);
	});
}

void Pbs::getJobsByUser(std::string user, std::function<void(std::vector<std::string> jobs, std::error_code ec)> cb) {
	_cmd({"qselect", {"-u", user}, {}, cmdopt::capture_stdout}, [cb](auto res){
		std::vector<std::string> jobs;
		if (!res.ec && res.exit==0) {
			splitString(res.out, "\n", [&](size_t start, size_t end) {
				std::string job=trim_copy(res.out.substr(start, end));
				if (!job.empty()) jobs.push_back(std::move(job));
				return true;
			});
			cb(jobs, {});
		} else {
			cb(jobs, res.exit != 0 ? error::qselect_u_failed : res.ec);
		}
	});
}

void Pbs::deleteJobsById(std::vector<std::string> jobs, bool force, std::function<void(std::error_code ec)> cb) {
	if (force) jobs.push_back("-p"); // purge forces job to be deleted

	_cmd({"qdel", jobs, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::qdel_failed : res.ec);
	});
}

void Pbs::deleteJobByUser(std::string user, bool force, std::function<void(std::error_code ec)> cb) {
	// qdel -u only supported by SGE https://stackoverflow.com/questions/28857807/use-qdel-to-delete-all-my-jobs-at-once-not-one-at-a-time
	// instead manually query jobs of user first and then delete them
	getJobsByUser(user, [this, cb, force](auto jobs, auto ec){
		if (ec) {
			cb(ec);
			return;
		}

		deleteJobsById(jobs, force, cb);
	});
}

void Pbs::deleteJobById(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	deleteJobsById({job}, force, cb);
}

void Pbs::holdJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	(void)force;
	_cmd({"qhold", {job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::qhold_failed : res.ec);
	});
}

void Pbs::releaseJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	(void)force;
	_cmd({"qrls", {job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::qrls_failed : res.ec);
	});
}

void Pbs::setNodeComment(std::string name, bool force, std::string comment, bool appendComment, std::function<void(std::error_code ec)> cb) {
	(void)force;
	_cmd({"pbsnodes", {name, appendComment ? "-A" : "-N", comment}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::pbsnodes_set_comment_failed : res.ec);
	});
}

void Pbs::changeNodeState(std::string name, NodeChangeState state, bool force, std::string reason, bool appendReason, std::function<void(std::error_code ec)> cb) {
	(void)force;
	std::string stateArg;
	switch (state) {
		case NodeChangeState::Resume: stateArg="-r"; break;
		case NodeChangeState::Drain: stateArg="-o"; break;
		case NodeChangeState::Undrain: stateArg="-c"; break;
		default: throw std::system_error(cw::batch::error::node_change_state_out_of_enum);
	}

    _cmd({"pbsnodes", {stateArg, name, appendReason ? "-A" : "-N", reason}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::pbsnodes_change_node_state_failed : res.ec);
	});
}

void Pbs::runJob(JobOptions opts, std::function<void(std::string jobName, std::error_code ec)> cb) {
	Cmd c{"qsub", {}, {}, cmdopt::capture_stdout};
	if (opts.numberNodes.has_value()) {
		c.args.push_back("-l");
		c.args.push_back("nodes="+std::to_string(opts.numberNodes.get()));
	}
	if (opts.numberTasks.has_value()) {
		c.args.push_back("-l");
		c.args.push_back("procs="+std::to_string(opts.numberTasks.get()));
	}
	if (opts.numberGpus.has_value()) {
		c.args.push_back("-l");
		c.args.push_back("gpus="+std::to_string(opts.numberGpus.get()));
	}
	c.args.push_back(opts.path.get());

	_cmd(c, [cb](auto res){
		if (!res.ec && res.exit==0) {
			cb(trim_copy(res.out), {});
		} else {
			cb("", error::qsub_failed);
		}
	});
}

void Pbs::detect(std::function<void(bool has_batch, std::error_code ec)> cb) {
	_cmd({"pbs-config", {"--version"}, {}, cmdopt::none}, [cb](auto res){
		cb(!res.ec && res.exit==0, {});
	});
}

void Pbs::getBatchInfo(std::function<void(BatchInfo, std::error_code ec)> cb) {
	_cmd({"pbsnodes", {"--version"}, {}, cmdopt::capture_stdout}, [cb](auto res){
		BatchInfo info;
		
		if (res.exit == 0 && !res.ec) {
			info.name = std::string("pbs");
			info.version = trim_copy(res.out);
		}

		cb(info, res.exit != 0 ? error::pbsnodes_version_failed : res.ec);
	});
}

}
}
}
