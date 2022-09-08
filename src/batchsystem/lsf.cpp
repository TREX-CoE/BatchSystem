#include "batchsystem/lsf.h"
#include "batchsystem/error.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
#include <functional>

#include "batchsystem/internal/trim.h"
#include "batchsystem/internal/streamCast.h"
#include "batchsystem/internal/splitString.h"

namespace {

using namespace cw::batch::lsf;

const char* to_cstr(error type) {
  switch (type) {
      case error::bkill_failed: return "bkill failed";
      case error::bkill_u_failed: return "bkill -u failed";
      case error::node_change_state_undrain_unsupported_by_lsf: return "node state change to undrain unsupported by lsf";
      case error::bresume_failed: return "bresume failed";
      case error::bstop_failed: return "bstop failed";
      case error::queue_state_inactive_unsupported_by_lsf: return "queue state change to inactive unsupported by lsf";
      case error::queue_state_draining_unsupported_by_lsf: return "queue state change to draining unsupported by lsf";
      case error::badmin_failed: return "badmin failed";
      case error::bhosts_failed: return "bhosts failed";
      case error::bqueues_failed: return "bqueues failed";
      case error::bsub_failed: return "bsub failed";
      case error::bjob_u_all_failed: return "bjob -u all failed";
      case error::lsid_failed: return "lsid failed";
	  case error::bsub_cannot_parse_job_name: return "cannot parse job name";
      default: return "(unrecognized error)";
  }
}

struct ErrCategory : std::error_category
{

  const char* name() const noexcept {
    return "cw::batch::lsf";
  }
 
  std::string message(int ev) const {
    return to_cstr(static_cast<error>(ev));
  }

  std::error_condition default_error_condition(int err_value) const noexcept {
      switch (static_cast<error>(err_value)) {
        case error::bkill_failed: return cw::batch::batch_condition::command_failed;
        case error::bkill_u_failed: return cw::batch::batch_condition::command_failed;
        case error::node_change_state_undrain_unsupported_by_lsf: return cw::batch::batch_condition::command_failed;
        case error::bresume_failed: return cw::batch::batch_condition::command_failed;
        case error::bstop_failed: return cw::batch::batch_condition::command_failed;
        case error::queue_state_inactive_unsupported_by_lsf: return cw::batch::batch_condition::command_failed;
        case error::queue_state_draining_unsupported_by_lsf: return cw::batch::batch_condition::command_failed;
        case error::badmin_failed: return cw::batch::batch_condition::command_failed;
        case error::bhosts_failed: return cw::batch::batch_condition::command_failed;
        case error::bqueues_failed: return cw::batch::batch_condition::command_failed;
        case error::bsub_failed: return cw::batch::batch_condition::command_failed;
        case error::bjob_u_all_failed: return cw::batch::batch_condition::command_failed;
        case error::lsid_failed: return cw::batch::batch_condition::command_failed;
		case error::bsub_cannot_parse_job_name: return cw::batch::batch_condition::command_failed;
        default: return cw::batch::batch_condition::command_failed;
      }
  }
};

const ErrCategory error_cat {};

}

namespace cw {
namespace batch {
namespace lsf {

using namespace cw::batch;
using namespace cw::batch::internal;

const std::error_category& error_category() noexcept {
    return error_cat;
}

std::error_code make_error_code(error e) {
  return {static_cast<int>(e), error_cat};
}

Lsf::Lsf(cmd_f func): _cmd(func) {}

const char* Lsf::name() const { return "lsf"; }

bool Lsf::getNodes(supported_t) { return true; }
bool Lsf::getQueues(supported_t) { return true; }
bool Lsf::getJobs(supported_t) { return true; }
bool Lsf::getBatchInfo(supported_t) { return true; }
bool Lsf::deleteJobById(supported_t) { return true; }
bool Lsf::deleteJobByUser(supported_t) { return true; }
bool Lsf::changeNodeState(supported_t) { return true; }
bool Lsf::setQueueState(supported_t) { return true; }
bool Lsf::runJob(supported_t) { return true; }
bool Lsf::holdJob(supported_t) { return true; }
bool Lsf::releaseJob(supported_t) { return true; }



void Lsf::parseNodes(const std::string& output, std::vector<Node>& nodes) {
	std::vector<int>	cellLengths;

	std::stringstream commandResult(output);
	std::stringstream buffer;
	char line[1024];
	bool firstLine=true;

	while(commandResult.good())
	{
		Node node {};
		if(commandResult.getline(line,1023))
		{
			line[1023]=0;

			if(firstLine)
			{
				firstLine=false;

				buffer.clear();
				buffer.str("");

				buffer << line;

				int i=0;
				bool next=true;
				while(buffer.good())
				{
					if(buffer.get()>32)
					{
						if(next)
						{
							if(i>0)
							{
								cellLengths.push_back(i);
							}

							i=0;
							next=false;
						}
					}
					else
					{
						next=true;
					}

					i++;
				}

				continue;
			}

			buffer.clear();
			buffer.str("");

			buffer << line;

			buffer.get(line,cellLengths[0]+1);
			line[1023]=0;
			node.name=trim_copy(line);

			buffer.get(line,cellLengths[1]+1);
			line[1023]=0;
			std::string nodeStatus=trim_copy(line);

			buffer.get(line,cellLengths[2]+1);

			buffer.get(line,cellLengths[3]+1);
			line[1023]=0;
			stream_cast(trim_copy(line), node.cpus.get());

			buffer.get(line,cellLengths[4]+1);

			buffer.get(line,cellLengths[5]+1);
			line[1023]=0;
			stream_cast(trim_copy(line), node.cpusUsed.get());

			buffer.get(line,cellLengths[6]+1);

			buffer.get(line,cellLengths[7]+1);

			buffer.getline(line,1023);
			line[1023]=0;
			stream_cast(trim_copy(line), node.cpusReserved.get());

			if(!buffer.bad() && node.name.get().length()>0)
			{
				if(nodeStatus=="ok")
				{
					node.state = NodeState::Online;
				}
				else if(nodeStatus=="unavail" || nodeStatus=="unreach")
				{
					node.state = NodeState::Offline;
				}
				else if(nodeStatus=="closed" || nodeStatus=="unlicensed")
				{
					node.state = NodeState::Disabled;
				}
				else
				{
					node.state = NodeState::Unknown;
				}

				nodes.push_back(node);
			}
		}
	}
}




void Lsf::parseJobs(const std::string& output, std::vector<Job>& jobs) {
	std::stringstream commandResult(output);

	std::vector<int> cellLengths;
	std::stringstream buffer;
	char line[1024];
	bool firstLine=true;

	while(commandResult.good())
	{
		Job job {};
		if(commandResult.getline(line,1023))
		{
			line[1023]=0;

			if(firstLine)
			{
				firstLine=false;

				buffer.clear();
				buffer.str("");

				buffer << line;

				int i=0;
				bool next=true;
				while(buffer.good())
				{
					if(buffer.get()>32)
					{
						if(next)
						{
							if(i>0)
							{
								cellLengths.push_back(i);
							}

							i=0;
							next=false;
						}
					}
					else
					{
						next=true;
					}

					i++;
				}

				continue;
			}

			buffer.clear();
			buffer.str("");

			buffer << line;

			buffer.get(line,cellLengths[0]+1);
			line[1023]=0;
			job.id=trim_copy(line);

			buffer.get(line,cellLengths[1]+1);
			line[1023]=0;
			job.owner=trim_copy(line);

			buffer.get(line,cellLengths[2]+1);
			line[1023]=0;
			std::string jobStatus=trim_copy(line);

			buffer.get(line,cellLengths[3]+1);
			line[1023]=0;
			job.queue=trim_copy(line);

			buffer.get(line,cellLengths[4]+1);

			buffer.get(line,cellLengths[5]+1);
			line[1023]=0;
			std::string execHost=trim_copy(line);

			buffer.get(line,cellLengths[6]+1);
			line[1023]=0;
			job.name=trim_copy(line);

			buffer.getline(line,1023);
			line[1023]=0;
			stream_cast(trim_copy(line), job.submitTime.get());

			if(!buffer.bad() && job.id.get().length()>0)
			{
				if(jobStatus=="RUN")
				{
					job.state = JobState::Running;
				}
				else if(jobStatus=="PSUP" || jobStatus=="USUSP" || jobStatus=="SSUSP" || jobStatus=="WAIT")
				{
					job.state = JobState::Suspend;
				}
				else if(jobStatus=="PEND")
				{
					job.state = JobState::Queued;
				}
				else if(jobStatus=="DONE")
				{
					job.state = JobState::Terminating;
				}
				else if(jobStatus=="ZOMBIE")
				{
					job.state = JobState::Zombie;
				}
				else
				{
					job.state = JobState::Unknown;
				}

				std::vector<std::string> tmp;
				splitString(execHost, "*", [&tmp,&execHost](size_t start, size_t end){
					tmp.push_back(execHost.substr(start, end));
					return true;
				});

				if(tmp.size()==2)
				{
					stream_cast(tmp[0], job.cpusFromNode[tmp[1]]);
				}
				else if(tmp.size()==1)
				{
					job.cpusFromNode[tmp[0]] = 1;
				}

				jobs.push_back(job);
			}
		}
	}
}

void Lsf::parseQueues(const std::string& output, std::vector<Queue>& queues) {
	std::vector<int>	cellLengths;

	std::stringstream commandResult(output);

	std::stringstream buffer;
	char line[1024];
	bool firstLine=true;

	while(commandResult.good())
	{
		Queue queue {};
		if(commandResult.getline(line,1023))
		{
			line[1023]=0;

			if(firstLine)
			{
				firstLine=false;

				buffer.clear();
				buffer.str("");

				buffer << line;

				int i=0;
				bool next=true;
				while(buffer.good())
				{
					if(buffer.get()>32)
					{
						if(next)
						{
							if(i>0)
							{
								cellLengths.push_back(i);
							}

							i=0;
							next=false;
						}
					}
					else
					{
						next=true;
					}

					i++;
				}

				continue;
			}

			buffer.clear();
			buffer.str("");

			buffer << line;

			buffer.get(line,cellLengths[0]+1);
			line[1023]=0;
			queue.name=trim_copy(line);

			buffer.get(line,cellLengths[1]+1);
			line[1023]=0;
			stream_cast(trim_copy(line), queue.priority.get());

			buffer.get(line,cellLengths[2]+1);
			line[1023]=0;
			std::string queueStatus=trim_copy(line);

			buffer.get(line,cellLengths[3]+1);

			buffer.get(line,cellLengths[4]+1);

			buffer.get(line,cellLengths[5]+1);

			buffer.get(line,cellLengths[6]+1);

			buffer.get(line,cellLengths[7]+1);
			line[1023]=0;
			stream_cast(trim_copy(line), queue.jobsTotal.get());

			buffer.get(line,cellLengths[8]+1);
			line[1023]=0;
			stream_cast(trim_copy(line), queue.jobsPending.get());

			buffer.get(line,cellLengths[9]+1);
			line[1023]=0;
			stream_cast(trim_copy(line), queue.jobsRunning.get());

			buffer.getline(line,1023);
			line[1023]=0;
			stream_cast(trim_copy(line), queue.jobsSuspended.get());

			if(!buffer.bad() && queue.name.get().length()>0)
			{
				if(queueStatus=="Open:Active")
				{
					queue.state = QueueState::Open;
				}
				else if(queueStatus=="Open:Inact")
				{
					queue.state = QueueState::Inactive;
				}
				else if(queueStatus=="Closed:Active")
				{
					queue.state = QueueState::Draining;
				}
				else if(queueStatus=="Closed:Inact")
				{
					queue.state = QueueState::Closed;
				}
				else
				{
					queue.state = QueueState::Unknown;
				}

				queues.push_back(queue);
			}
		}
	}
}

void Lsf::getNodes(std::vector<std::string> filterNodes, std::function<void(std::vector<Node> nodes, std::error_code ec)> cb) {
	std::vector<std::string> args{};
	for (const auto& n : filterNodes) args.push_back(n);
	_cmd({"bhosts", args, {}, cmdopt::capture_stdout}, [cb](auto res){
		std::vector<Node> nodes;
		if (!res.ec && res.exit == 0) Lsf::parseNodes(res.out, nodes);
		cb(nodes, res.exit != 0 ? error::bhosts_failed : res.ec);
	});
}

void Lsf::getJobs(std::vector<std::string> filterJobs, std::function<void(std::vector<Job> jobs, std::error_code ec)> cb) {
	(void)filterJobs;
	_cmd({"bjobs", {"-u", "all"}, {}, cmdopt::capture_stdout}, [cb](auto res){
		std::vector<Job> jobs;
		if (!res.ec && res.exit == 0) Lsf::parseJobs(res.out, jobs);
		cb(jobs, res.exit != 0 ? error::bjob_u_all_failed : res.ec);
	});
}

void Lsf::getQueues(std::vector<std::string> filterQueues, std::function<void(std::vector<Queue> queues, std::error_code ec)> cb) {
	(void)filterQueues;
	_cmd({"bqueues", {}, {}, cmdopt::capture_stdout}, [cb](auto res){
		std::vector<Queue> queues;
		if (!res.ec && res.exit == 0) Lsf::parseQueues(res.out, queues);
		cb(queues, res.exit != 0 ? error::bqueues_failed : res.ec);
	});
}

void Lsf::setQueueState(std::string name, QueueState state, bool force, std::function<void(std::error_code ec)> cb) {
	(void)force;
	std::string subcmd;
	switch (state) {
		case QueueState::Unknown: throw std::system_error(cw::batch::error::queue_state_unknown_not_supported);
		case QueueState::Open:
			subcmd = "qopen";
			break;
		case QueueState::Closed:
			subcmd = "qclose";
			break;
		case QueueState::Inactive: throw std::system_error(error::queue_state_inactive_unsupported_by_lsf);
		case QueueState::Draining: throw std::system_error(error::queue_state_draining_unsupported_by_lsf);
		default: throw std::system_error(cw::batch::error::queue_state_out_of_enum);
	}
	_cmd({"badmin", {subcmd, name}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::badmin_failed : res.ec);
	});
}

void Lsf::deleteJobByUser(std::string user, bool force, std::function<void(std::error_code ec)> cb) {
	(void)force;
	_cmd({"bkill", {"-u", user}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::bkill_u_failed : res.ec);
	});
}

void Lsf::deleteJobById(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	(void)force;
	_cmd({"bkill", {job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::bkill_failed : res.ec);
	});
}

void Lsf::holdJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	(void)force;
	_cmd({"bstop", {job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::bstop_failed : res.ec);
	});
}

void Lsf::releaseJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) {
	(void)force;
	_cmd({"bresume", {job}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::bresume_failed : res.ec);
	});
}

void Lsf::changeNodeState(std::string name, NodeChangeState state, bool force, std::string reason, bool appendReason, std::function<void(std::error_code ec)> cb) {
	(void)force;
	(void)reason;
	(void)appendReason;
	std::string subcmd;
	switch (state) {
		case NodeChangeState::Resume: 
			subcmd = "hopen";
			break;
		case NodeChangeState::Drain:
			subcmd = "hclose";
			break;
		case NodeChangeState::Undrain:
			throw std::system_error(error::node_change_state_undrain_unsupported_by_lsf);
		default:
			throw std::system_error(cw::batch::error::node_change_state_out_of_enum);
	}
	_cmd({"badmin", {subcmd, name}, {}, cmdopt::none}, [cb](auto res){
		cb(res.exit != 0 ? error::badmin_failed : res.ec);
	});
}

void Lsf::runJob(JobOptions opts, std::function<void(std::string jobName, std::error_code ec)> cb) {
	Cmd c{"bsub", {}, {}, cmdopt::capture_stdout};
	if (opts.numberNodes.has_value()) {
		c.args.push_back("-nnodes");
		c.args.push_back(std::to_string(opts.numberNodes.get()));
	}
	if (opts.numberTasks.has_value()) {
		c.args.push_back("-n");
		c.args.push_back(std::to_string(opts.numberTasks.get()));
	}
	c.args.push_back(opts.path.get());

	_cmd(c, [cb](auto res){
		if (!res.ec && res.exit==0) {
			auto start = res.out.find_first_of("<");
			auto end = res.out.find_first_of(">");
			if (start != std::string::npos && end != std::string::npos && end > start) {
				cb(res.out.substr(start+1, end-start), {});
				return;
			} else {
				cb("", error::bsub_cannot_parse_job_name);
				return;
			}
		}
		cb("", error::bsub_failed);
	});
}

void Lsf::detect(std::function<void(bool has_batch, std::error_code ec)> cb) {
	_cmd({"bjobs", {"--version"}, {}, cmdopt::none}, [cb](auto res){
		cb(!res.ec && res.exit==0, {});
	});
}

void Lsf::getBatchInfo(std::function<void(BatchInfo, std::error_code ec)> cb) {
	_cmd({"lsid", {}, {}, cmdopt::capture_stderr}, [cb](auto res){
		BatchInfo info;
		
		if (res.exit == 0 && !res.ec) {
			info.name = std::string("lfs");
			info.version = trim_copy(res.err);
		}

		cb(info, res.exit != 0 ? error::lsid_failed : res.ec);
	});
}

}
}
}
