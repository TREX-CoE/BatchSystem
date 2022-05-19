#include "batchsystem/lsfBatch.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <functional>

#include "batchsystem/internal/runCommand.h"
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

}

namespace cw {
namespace batch {
namespace lsf {

LsfBatch::LsfBatch(std::function<cmd_execute_f> func): _func(func) {}

void LsfBatch::parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert) {
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

bool LsfBatch::getNodes(const std::vector<std::string>& filterNodes, std::function<getNodes_inserter_f> insert) const {
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

bool LsfBatch::detect(bool& detected) {
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

void LsfBatch::parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert) {
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

bool LsfBatch::getJobs(std::function<getJobs_inserter_f> insert) const {
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

void LsfBatch::parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert) {
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

bool LsfBatch::getQueues(std::function<getQueues_inserter_f> insert) const {
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

bool LsfBatch::setQueueState(const std::string& name, QueueState state, bool) const {
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
	if (ret == -1) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		return true;
	}
}

bool LsfBatch::changeNodeState(const std::string& name, NodeChangeState state, bool, const std::string&, bool) const {
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
	if (ret == -1) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		return true;
	}

}

bool LsfBatch::releaseJob(const std::string& job, bool) const {
	CmdOptions opts{"bresume", {job}};

	std::string out;
	int ret = _func(out, opts);
	if (ret == -1) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		return true;
	}
}
bool LsfBatch::holdJob(const std::string& job, bool) const {
	CmdOptions opts{"bstop", {job}};

	std::string out;
	int ret = _func(out, opts);
	if (ret == -1) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		return true;
	}
}
bool LsfBatch::deleteJobByUser(const std::string& user, bool) const {
	CmdOptions opts{"bkill", {"-u", user}};

	std::string out;
	int ret = _func(out, opts);
	if (ret == -1) {
		return false;
	} else if (ret > 0) {
		throw CommandFailed("Command failed", opts, ret);
	} else {
		return true;
	}
}
bool LsfBatch::deleteJobById(const std::string& job, bool) const {
	CmdOptions opts{"bkill", {job}};

	std::string out;
	int ret = _func(out, opts);
	if (ret == -1) {
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
