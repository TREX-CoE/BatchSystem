#include "batchsystem/pbs.h"

#include <iostream>
#include <sstream>
#include <vector>

#include <libxml++/libxml++.h>

#include "batchsystem/internal/trim.h"
#include "batchsystem/internal/streamCast.h"
#include "batchsystem/internal/splitString.h"
#include "batchsystem/internal/timeToEpoch.h"
#include "batchsystem/internal/byteParse.h"
#include "batchsystem/internal/singleCmd.h"

using namespace cw::batch;
using namespace cw::batch::internal;

namespace {

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

Pbs::Pbs(cmd_f func): _func(func) {}

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


void Pbs::parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert) {
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

// see man pbs_job_attributes 
void Pbs::parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert) {
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

void Pbs::parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert) {
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


class DeleteJobByUser {
private:
    cmd_f& cmd;
	std::string user;
	bool force;
    Result qselect;
    Result qdel;
    enum class State {
        QselectStart,
        QselectWaiting,
        QdelStart,
		QdelWaiting,
		Done,
    };
    State state = State::QselectStart;
public:
    DeleteJobByUser(cmd_f& cmd_, const std::string& user_, bool force_): cmd(cmd_), user(user_), force(force_) {}

    bool operator()() {
        switch (state) {
			case State::QselectStart: {
				// qdel -u only supported by SGE https://stackoverflow.com/questions/28857807/use-qdel-to-delete-all-my-jobs-at-once-not-one-at-a-time
				// instead manually query jobs of user first and then delete them
				cmd(qselect, {"qselect", {"-u", user}, {}, runopt_capture_stdout});
				state = State::QselectWaiting;
			}
			// fall through
			case State::QselectWaiting: {
                if (qselect.exit==-1) {
                    return false;
                } else if (qselect.exit==0) {
					state=State::QdelStart;
                } else {
					throw CommandFailed("qselect -u");
				}
			}
			// fall through
			case State::QdelStart: {
				std::vector<std::string> args;
				splitString(qselect.out, "\n", [&](size_t start, size_t end) {
					std::string job=trim_copy(qselect.out.substr(start, end));
					if (!job.empty()) args.push_back(std::move(job));
					return true;
				});
				if (force) args.push_back("-p"); // purge forces job to be deleted

				cmd(qselect, {"qdel", args, {}, runopt_none});
				state = State::QdelWaiting;
			}
			// fall through
			case State::QdelWaiting: {
				if (qdel.exit==-1) {
					return false;
				} else if (qdel.exit!=0) {
					throw CommandFailed("qdel");
				}
				state=State::Done;
			}
			// fall through
			case State::Done: {
				return true;
			}
			default: throw std::runtime_error("invalid state");
        }
    }
};


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
				std::string stateArg;
				switch (nodeState) {
					case NodeChangeState::Resume: stateArg="-r"; break;
					case NodeChangeState::Drain: stateArg="-o"; break;
					case NodeChangeState::Undrain: stateArg="-c"; break;
					default: throw std::runtime_error("invalid state");
				}

                cmd(res, {"pbsnodes", {stateArg, name, appendReason ? "-A" : "-N", reason}, {}, runopt_none});
                state=State::Waiting;
			}
			// fall through
            case State::Waiting:
                if (!checkWaiting("pbsnodes")) return false; 
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
                cmd(res, {"pbsnodes", {name, appendComment ? "-A" : "-N", comment}, {}, runopt_none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("pbsnodes")) return false; 
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
                cmd(res, {"qrls", {job}, {}, runopt_none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("qrls")) return false; 
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
                cmd(res, {"qhold", {job}, {}, runopt_none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("qhold")) return false; 
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
            case State::Starting: {
				// purge forces job to be deleted
				std::vector<std::string> args;
				if (force) args.push_back("-p");
				args.push_back(job);
                cmd(res, {"qdel", args, {}, runopt_none});
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting("qdel")) return false; 
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
                cmd(res, {"qsig", {"-s", "suspend", job}, {}, runopt_none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting("qsig -s suspend")) return false; 
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
                cmd(res, {"qsig", {"-s", "resume", job}, {}, runopt_none});
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
				bool enabled = false;
				bool started = false;
				switch (queueState) {
					case QueueState::Unknown: throw std::runtime_error("unknown state");
					case QueueState::Open: enabled=true; started=true; break;
					case QueueState::Closed: enabled=false; started=false; break;
					case QueueState::Inactive: enabled=true; started=false; break;
					case QueueState::Draining: enabled=false; started=true; break;
					default: throw std::runtime_error("invalid state"); break;
				}

				cmd(res, {"qmgr", {"-c", "set queue " + name + " enabled="+(enabled ? "true" : "false") + ",started="+(started ? "true" : "false")}, {}, runopt_none});
                state=State::Waiting;
			}
			// fall through
            case State::Waiting:
                if (!checkWaiting("qmgr -c")) return false; 
                // fall through
            case State::Done: {
				return true;
			}
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
                cmd(res, {"pbs-config", {"--version"}, {}, runopt_none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
				if (res.exit==-1) {
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
            case State::Starting: {
				std::vector<std::string> args;
				if (hold) args.push_back("-f");
				args.push_back(job);
                cmd(res, {"qrerun", args, {}, runopt_none});
                state=State::Waiting;
			}
			// fall through
            case State::Waiting:
                if (!checkWaiting("qrerun")) return false; 
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
				std::vector<std::string> args{"-x"};
				for (const auto& n : filterNodes) args.push_back(n);
                cmd(res, {"pbsnodes", args, {}, runopt_capture_stdout});
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting("pbsnodes -x")) return false; 
                // fall through
            case State::Done: 
				Pbs::parseNodes(res.out, insert);
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
                cmd(res, {"qstat", {"-Qf"}, {}, runopt_capture_stdout});
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting("qstat -Qf")) return false; 
                // fall through
            case State::Done: 
				Pbs::parseQueues(res.out, insert);
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
				Cmd c{"qsub", {}, {}, runopt_capture_stdout};
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

                cmd(res, c);
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting("qsub")) return false; 
                // fall through
            case State::Done: 
				jobName = trim_copy(res.out);
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};

class GetJobs: public SingleCmd {
public:
	using SingleCmd::SingleCmd;

    bool operator()(const std::function<getJobs_inserter_f>& insert) {
        switch (state) {
            case State::Starting: {
                cmd(res, {"qstat", {"-f", "-x"}, {}, runopt_capture_stdout});
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting("qstat -f -x")) return false; 
                // fall through
            case State::Done: 
				Pbs::parseJobs(res.out, insert);
				return true;
			default: throw std::runtime_error("invalid state");
        }
	}
};

class GetBatchInfo: public SingleCmd {
public:
	using SingleCmd::SingleCmd;

    bool operator()(BatchInfo& info) {
        switch (state) {
			case State::Starting: {
				// start in parallel
				cmd(res, {"pbsnodes", {"--version"}, {}, runopt_capture_stdout});
				state = State::Waiting;
			}
			// fall through
			case State::Waiting:
                if (!checkWaiting("scontrol update")) return false; 
				// fall through
			case State::Done:
				info.name = std::string("pbs");
				info.version = trim_copy(res.out);
				return true;
			default: throw std::runtime_error("invalid state");
        }
    }
};


std::function<bool(const std::function<getNodes_inserter_f>& insert)> Pbs::getNodes(std::vector<std::string> filterNodes) { return GetNodes(_func, filterNodes); }
std::function<bool(const std::function<getJobs_inserter_f>& insert)> Pbs::getJobs(std::vector<std::string>) { return GetJobs(_func); }
std::function<bool(const std::function<getQueues_inserter_f>& insert)> Pbs::getQueues() { return GetQueues(_func); }
std::function<bool()> Pbs::rescheduleRunningJobInQueue(const std::string& job, bool force) { return RescheduleRunningJobInQueue(_func, job, force); }
std::function<bool()> Pbs::setQueueState(const std::string& name, QueueState state, bool force) { return SetQueueState(_func, name, state, force); }
std::function<bool()> Pbs::resumeJob(const std::string& job, bool force) { return ResumeJob(_func, job, force); }
std::function<bool()> Pbs::suspendJob(const std::string& job, bool force) { return SuspendJob(_func, job, force); }
std::function<bool()> Pbs::deleteJobByUser(const std::string& user, bool force) { return DeleteJobByUser(_func, user, force); }
std::function<bool()> Pbs::deleteJobById(const std::string& job, bool force) { return DeleteJobById(_func, job, force); }
std::function<bool()> Pbs::holdJob(const std::string& job, bool force) { return HoldJob(_func, job, force); }
std::function<bool()> Pbs::releaseJob(const std::string& job, bool force) { return ReleaseJob(_func, job, force); }
std::function<bool()> Pbs::setNodeComment(const std::string& name, bool force, const std::string& comment, bool appendComment) { return SetNodeComment(_func, name, force, comment, appendComment); }
std::function<bool()> Pbs::changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) { return ChangeNodeState(_func, name, state, force, reason, appendReason); }
std::function<bool(std::string&)> Pbs::runJob(const JobOptions& opts) { return RunJob(_func, opts); }
std::function<bool(bool&)> Pbs::detect() { return Detect(_func); }
std::function<bool(BatchInfo&)> Pbs::getBatchInfo() { return GetBatchInfo(_func); }


}
}
}
