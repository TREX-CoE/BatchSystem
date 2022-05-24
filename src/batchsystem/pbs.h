#ifndef __CW_BATCHSYSTEM_PBS_H__
#define __CW_BATCHSYSTEM_PBS_H__

#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {
namespace pbs {

/**
 * \brief Concrete implementation of batchsystem for PBS
 * 
 * Wrapped within generic interface via \ref cw::batch::pbs::create_batch.
 * 
 */
class Pbs : public BatchInterface {
private:
	std::function<cmd_execute_f> _func;
	std::map<CmdOptions, std::string> _cache;
public:
	Pbs(std::function<cmd_execute_f> func);

	static void parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert);
	static void parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert);
	static void parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert);

	bool getJobsByUser(const std::string& user, std::function<bool(std::string)> inserter);

	bool detect(bool& detected) override;
	void resetCache() override;
	bool getNodes(const std::vector<std::string>& filterNodes, std::function<getNodes_inserter_f> insert) override;
	bool getQueues(std::function<getQueues_inserter_f> insert) override;
	bool getJobs(std::function<getJobs_inserter_f> insert) override;
	bool getBatchInfo(BatchInfo& info) override;
	bool deleteJobById(const std::string& job, bool force) override;
	bool deleteJobByUser(const std::string& user, bool force) override;
	bool changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) override;
	bool setQueueState(const std::string& name, QueueState state, bool force) override;
	bool runJob(const JobOptions& opts, std::string& jobName) override;
	bool setNodeComment(const std::string& name, bool, const std::string& comment, bool appendComment) override;
	bool holdJob(const std::string& job, bool force) override;
	bool releaseJob(const std::string& job, bool force) override;
	bool suspendJob(const std::string& job, bool force) override;
	bool resumeJob(const std::string& job, bool force) override;
	bool rescheduleRunningJobInQueue(const std::string& job, bool force) override;

	bool getNodes(supported) override;
	bool getQueues(supported) override;
	bool getJobs(supported) override;
	bool getBatchInfo(supported) override;
	bool deleteJobById(supported) override;
	bool deleteJobByUser(supported) override;
	bool changeNodeState(supported) override;
	bool setQueueState(supported) override;
	bool runJob(supported) override;
	bool setNodeComment(supported) override;
	bool holdJob(supported) override;
	bool releaseJob(supported) override;
	bool suspendJob(supported) override;
	bool resumeJob(supported) override;
	bool rescheduleRunningJobInQueue(supported) override;
};


}
}
}

#endif /* __CW_BATCHSYSTEM_PBS_H__ */
