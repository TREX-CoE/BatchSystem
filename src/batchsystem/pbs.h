#ifndef __CW_BATCHSYSTEM_PBS_H__
#define __CW_BATCHSYSTEM_PBS_H__

#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {
namespace pbs {

/**
 * \brief Concrete implementation of batchsystem for PBS
 * 
 */
class Pbs : public BatchInterface {
private:
	cmd_f _func;
public:
	Pbs(cmd_f func);

	static void parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert);
	static void parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert);
	static void parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert);


	bool getNodes(supported_t) override;
	bool getQueues(supported_t) override;
	bool getJobs(supported_t) override;
	bool getBatchInfo(supported_t) override;
	bool deleteJobById(supported_t) override;
	bool deleteJobByUser(supported_t) override;
	bool changeNodeState(supported_t) override;
	bool setQueueState(supported_t) override;
	bool runJob(supported_t) override;
	bool setNodeComment(supported_t) override;
	bool holdJob(supported_t) override;
	bool releaseJob(supported_t) override;
	bool suspendJob(supported_t) override;
	bool resumeJob(supported_t) override;
	bool rescheduleRunningJobInQueue(supported_t) override;

	std::function<getNodes_f> getNodes(std::vector<std::string> filterNodes) override;
	std::function<getJobs_f> getJobs(std::vector<std::string> filterJobs) override;
	std::function<getQueues_f> getQueues() override;
	std::function<bool()> rescheduleRunningJobInQueue(const std::string& job, bool force) override;
	std::function<bool()> setQueueState(const std::string& name, QueueState state, bool force) override;
	std::function<bool()> resumeJob(const std::string& job, bool force) override;
	std::function<bool()> suspendJob(const std::string& job, bool force) override;
	std::function<bool()> deleteJobByUser(const std::string& user, bool force) override;
	std::function<bool()> deleteJobById(const std::string& job, bool force) override;
	std::function<bool()> holdJob(const std::string& job, bool force) override;
	std::function<bool()> releaseJob(const std::string& job, bool force) override;
	std::function<bool()> setNodeComment(const std::string& name, bool force, const std::string& comment, bool appendComment) override;
	std::function<bool()> changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) override;
	std::function<runJob_f> runJob(const JobOptions& opts) override;
	std::function<bool(bool&)> detect() override;
	std::function<bool(BatchInfo&)> getBatchInfo() override;

};


}
}
}

#endif /* __CW_BATCHSYSTEM_PBS_H__ */
