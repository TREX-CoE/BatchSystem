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
#include "batchsystem/internal/singleCmd.h"

using namespace cw::batch;
using namespace cw::batch::internal;

namespace cw {
namespace batch {
namespace lsf {

Lsf::Lsf(cmd_f func): _func(func) {}

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



void Lsf::parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert) {
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

				if (!insert(node)) return;
			}
		}
	}
}




void Lsf::parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert) {
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

				if (!insert(job)) return;
			}
		}
	}
}

void Lsf::parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert) {
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

				if (!insert(queue)) return;
			}
		}
	}
}

class DeleteJobByUser: public SingleCmd {
private:
	std::string user;
	bool force;
public:
	DeleteJobByUser(cmd_f& cmd_, const std::string& user_, bool force_): SingleCmd(cmd_), user(user_), force(force_) {}
    bool operator()() {
        switch (state) {
            case State::Starting:
                cmd(res, {"bkill", {"-u", user}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting(batch_error::bkill_u_failed)) return false; 
                // fall through
            case State::Done:
				return true;
			default: assert(false && "invalid state");
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
				std::string subcmd;
				switch (nodeState) {
					case NodeChangeState::Resume: 
						subcmd = "hopen";
						break;
					case NodeChangeState::Drain:
						subcmd = "hclose";
						break;
					case NodeChangeState::Undrain:
						throw std::system_error(batch_error::node_change_state_undrain_unsupported_by_lsf);
					default:
						throw std::system_error(batch_error::node_change_state_out_of_enum);
				}
                cmd(res, {"badmin", {subcmd, name}, {}, cmdopt::none});
                state=State::Waiting;
			}
			// fall through
            case State::Waiting:
                if (!checkWaiting(batch_error::badmin_failed)) return false; 
                // fall through
            case State::Done:
				return true;
			default: assert(false && "invalid state");
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
                cmd(res, {"bresume", {job}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting(batch_error::bresume_failed)) return false; 
                // fall through
            case State::Done:
				return true;
			default: assert(false && "invalid state");
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
                cmd(res, {"bstop", {job}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting(batch_error::bstop_failed)) return false; 
                // fall through
            case State::Done:
				return true;
			default: assert(false && "invalid state");
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
				// purge forces job to be deleted
                cmd(res, {"bkill", {job}, {}, cmdopt::none});
                state=State::Waiting;
                // fall through
            case State::Waiting:
                if (!checkWaiting(batch_error::qdel_failed)) return false; 
                // fall through
            case State::Done:
				return true;
			default: assert(false && "invalid state");
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
				std::string subcmd;
				switch (queueState) {
					case QueueState::Unknown: throw std::system_error(batch_error::queue_state_unknown_not_supported);
					case QueueState::Open:
						subcmd = "qopen";
						break;
					case QueueState::Closed:
						subcmd = "qclose";
						break;
					case QueueState::Inactive: throw std::system_error(batch_error::queue_state_inactive_unsupported_by_lsf);
					case QueueState::Draining: throw std::system_error(batch_error::queue_state_draining_unsupported_by_lsf);
					default: throw std::system_error(batch_error::queue_state_out_of_enum);
				}
				cmd(res, {"badmin", {subcmd, name}, {}, cmdopt::none});
                state=State::Waiting;
			}
			// fall through
            case State::Waiting:
                if (!checkWaiting(batch_error::badmin_failed)) return false; 
                // fall through
            case State::Done: {
				return true;
			}
			default: assert(false && "invalid state");
        }
	}
};

class Detect: public SingleCmd {
public:
	using SingleCmd::SingleCmd;
    bool operator()(bool& detected) {
        switch (state) {
            case State::Starting:
                cmd(res, {"bjobs", {"--version"}, {}, cmdopt::none});
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
			default: assert(false && "invalid state");
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
				std::vector<std::string> args{};
				for (const auto& n : filterNodes) args.push_back(n);
                cmd(res, {"bhosts", args, {}, cmdopt::capture_stdout});
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting(batch_error::bhosts_failed)) return false; 
                // fall through
            case State::Done: 
				Lsf::parseNodes(res.out, insert);
				return true;
			default: assert(false && "invalid state");
        }
    }
};

class GetQueues: public SingleCmd {
public:
	using SingleCmd::SingleCmd;
    bool operator()(const std::function<getQueues_inserter_f>& insert) {
        switch (state) {
            case State::Starting: {
                cmd(res, {"bqueues", {}, {}, cmdopt::capture_stdout});
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting(batch_error::bqueues_failed)) return false; 
                // fall through
            case State::Done: 
				Lsf::parseQueues(res.out, insert);
				return true;
			default: assert(false && "invalid state");
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

                cmd(res, c);
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting(batch_error::bsub_failed)) return false; 
                // fall through
            case State::Done: {
				auto start = res.out.find_first_of("<");
				auto end = res.out.find_first_of(">");
				if (start != std::string::npos && end != std::string::npos && end > start) {
					jobName = res.out.substr(start+1, end-start);
				} else {
					throw std::runtime_error(std::string("Failed to parse job name from: ")+res.out);
				}
				return true;
			}
			default: assert(false && "invalid state");
        }
    }
};

class GetJobs: public SingleCmd {
public:
	using SingleCmd::SingleCmd;

    bool operator()(const std::function<getJobs_inserter_f>& insert) {
        switch (state) {
            case State::Starting: {
                cmd(res, {"bjobs", {"-u", "all"}, {}, cmdopt::capture_stdout});
                state=State::Waiting;
			}
                // fall through
            case State::Waiting:
                if (!checkWaiting(batch_error::bjob_u_all_failed)) return false; 
                // fall through
            case State::Done: 
				Lsf::parseJobs(res.out, insert);
				return true;
			default: assert(false && "invalid state");
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
				cmd(res, {"lsid", {}, {}, cmdopt::capture_stderr});
				state = State::Waiting;
			}
			// fall through
			case State::Waiting:
                if (!checkWaiting(batch_error::lsid_failed)) return false; 
				// fall through
			case State::Done:
				info.name = std::string("lfs");
				info.version = trim_copy(res.err);
				return true;
			default: assert(false && "invalid state");
        }
    }
};

std::function<getNodes_f> Lsf::getNodes(std::vector<std::string> filterNodes) { return GetNodes(_func, filterNodes); }
std::function<getJobs_f> Lsf::getJobs(std::vector<std::string>) { return GetJobs(_func); }
std::function<getQueues_f> Lsf::getQueues() { return GetQueues(_func); }
std::function<bool()> Lsf::setQueueState(const std::string& name, QueueState state, bool force) { return SetQueueState(_func, name, state, force); }
std::function<bool()> Lsf::deleteJobByUser(const std::string& user, bool force) { return DeleteJobByUser(_func, user, force); }
std::function<bool()> Lsf::deleteJobById(const std::string& job, bool force) { return DeleteJobById(_func, job, force); }
std::function<bool()> Lsf::holdJob(const std::string& job, bool force) { return HoldJob(_func, job, force); }
std::function<bool()> Lsf::releaseJob(const std::string& job, bool force) { return ReleaseJob(_func, job, force); }
std::function<bool()> Lsf::changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) { return ChangeNodeState(_func, name, state, force, reason, appendReason); }
std::function<runJob_f> Lsf::runJob(const JobOptions& opts) { return RunJob(_func, opts); }
std::function<bool(bool&)> Lsf::detect() { return Detect(_func); }
std::function<bool(BatchInfo&)> Lsf::getBatchInfo() { return GetBatchInfo(_func); }



}
}
}
