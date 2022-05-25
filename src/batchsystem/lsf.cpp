#include "batchsystem/lsf.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <functional>

#include "batchsystem/internal/trim.h"
#include "batchsystem/internal/streamCast.h"
#include "batchsystem/internal/splitString.h"

using namespace cw::batch;
using namespace cw::batch::internal;

namespace {

const CmdOptions optsDetect{"bjobs", {"--version"}};
const CmdOptions optsGetNodes{"bhosts", {}};
const CmdOptions optsGetJobs{"bjobs", {"-u", "all"}};
const CmdOptions optsGetQueues{"bqueues", {}};
const CmdOptions optsVersion{"lsid", {}};

}

namespace cw {
namespace batch {
namespace lsf {

Lsf::Lsf(std::function<cmd_execute_f> func): _func(func) {}

bool Lsf::getNodes(supported) { return true; }
bool Lsf::getQueues(supported) { return true; }
bool Lsf::getJobs(supported) { return true; }
bool Lsf::getBatchInfo(supported) { return true; }
bool Lsf::deleteJobById(supported) { return true; }
bool Lsf::deleteJobByUser(supported) { return true; }
bool Lsf::changeNodeState(supported) { return true; }
bool Lsf::setQueueState(supported) { return true; }
bool Lsf::runJob(supported) { return true; }
bool Lsf::holdJob(supported) { return true; }
bool Lsf::releaseJob(supported) { return true; }

bool Lsf::getBatchInfo(BatchInfo& info) {
	std::string out;
	int ret = _func(out, optsVersion);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", optsVersion, ret);
	} else {
		info.name = std::string("lfs");
		info.version = trim_copy(out);
		return true;
	}
}

bool Lsf::runJob(const JobOptions& opts, std::string& jobName) {
	CmdOptions cmd{"qsub", {}};
	if (opts.numberNodes.has_value()) {
		cmd.args.push_back("-nnodes");
		cmd.args.push_back(std::to_string(opts.numberNodes.get()));
	}
	if (opts.numberTasks.has_value()) {
		cmd.args.push_back("-n");
		cmd.args.push_back(std::to_string(opts.numberTasks.get()));
	}
	cmd.args.push_back(opts.path.get());

	std::string out;
	int ret = _func(out, cmd);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", cmd, ret);
	} else {
		auto start = out.find_first_of("<");
		auto end = out.find_first_of(">");
		
		if (start != std::string::npos && end != std::string::npos && end > start) {
			jobName = out.substr(start+1, end-start);
		} else {
			throw std::runtime_error(std::string("Failed to parse job name from: ")+out);
		}
		return true;
	}
}

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

bool Lsf::getNodes(const std::vector<std::string>& filterNodes, std::function<getNodes_inserter_f> insert) {
	CmdOptions opts = optsGetNodes;
	for (const auto& n : filterNodes) opts.args.push_back(n);

	std::string out;
	int ret = _func(out, opts);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		parseNodes(out, insert);
		return true;
	}
}

bool Lsf::detect(bool& detected) {
	std::string out;
	int ret = _func(out, optsDetect);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		detected = false;
		return true;
	} else {
		detected = true;
		return true;
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

bool Lsf::getJobs(std::function<getJobs_inserter_f> insert) {
	std::string out;
	int ret = _func(out, optsGetJobs);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", optsGetJobs, ret);
	} else {
		parseJobs(out, insert);
		return true;
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

bool Lsf::getQueues(std::function<getQueues_inserter_f> insert) {
	std::string out;
	int ret = _func(out, optsGetQueues);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", optsGetQueues, ret);
	} else {
		parseQueues(out, insert);
		return true;
	}
}

bool Lsf::setQueueState(const std::string& name, QueueState state, bool) {
	CmdOptions opts;

	switch (state) {
		case QueueState::Unknown: throw std::runtime_error("unknown state");
		case QueueState::Open:
			opts = {"badmin", {"qopen", name}};
			break;
		case QueueState::Closed:
			opts = {"badmin", {"qclose", name}};
			break;
		case QueueState::Inactive: throw std::runtime_error("inactive not supported");
		case QueueState::Draining: throw std::runtime_error("draining not supported");
		default: throw std::runtime_error("invalid state");
	}

	std::string out;
	int ret = _func(out, opts);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		return true;
	}
}

bool Lsf::changeNodeState(const std::string& name, NodeChangeState state, bool, const std::string&, bool) {
	CmdOptions opts;
	std::string stateArg;
	switch (state) {
		case NodeChangeState::Resume: 
			opts = {"badmin", {"hopen", name}};
			break;
		case NodeChangeState::Drain:
			opts = {"badmin", {"hclose", name}};
			break;
		case NodeChangeState::Undrain:
			throw std::runtime_error("Undrain not supported for LSF");
		default:
			throw std::runtime_error("invalid state");
	}

	std::string out;
	int ret = _func(out, opts);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		return true;
	}

}

bool Lsf::releaseJob(const std::string& job, bool) {
	CmdOptions opts{"bresume", {job}};

	std::string out;
	int ret = _func(out, opts);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		return true;
	}
}
bool Lsf::holdJob(const std::string& job, bool) {
	CmdOptions opts{"bstop", {job}};

	std::string out;
	int ret = _func(out, opts);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		return true;
	}
}
bool Lsf::deleteJobByUser(const std::string& user, bool) {
	CmdOptions opts{"bkill", {"-u", user}};

	std::string out;
	int ret = _func(out, opts);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		return true;
	}
}
bool Lsf::deleteJobById(const std::string& job, bool) {
	CmdOptions opts{"bkill", {job}};

	std::string out;
	int ret = _func(out, opts);
	if (ret == not_finished) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		return true;
	}
}

}
}
}
