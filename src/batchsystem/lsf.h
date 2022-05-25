#ifndef __CW_BATCHSYSTEM_LSF_H__
#define __CW_BATCHSYSTEM_LSF_H__

#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {
namespace lsf {

/**
 * \brief Concrete implementation of batchsystem for LSF
 * 
 */
class Lsf : public BatchInterface {
private:
	std::function<cmd_execute_f> _func;
public:
	Lsf(std::function<cmd_execute_f> func);

	static void parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert);
	static void parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert);
	static void parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert);

	bool detect(bool& detected) override;
	bool getNodes(const std::vector<std::string>& filterNodes, std::function<getNodes_inserter_f> insert) override;
	bool getQueues(std::function<getQueues_inserter_f> insert) override;
	bool getJobs(std::function<getJobs_inserter_f> insert) override;
	bool getBatchInfo(BatchInfo& info) override;
	bool deleteJobById(const std::string& job, bool force) override;
	bool deleteJobByUser(const std::string& user, bool force) override;
	bool changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) override;
	bool setQueueState(const std::string& name, QueueState state, bool force) override;
	bool runJob(const JobOptions& opts, std::string& jobName) override;
	bool holdJob(const std::string& job, bool force) override;
	bool releaseJob(const std::string& job, bool force) override;

	bool getNodes(supported) override;
	bool getQueues(supported) override;
	bool getJobs(supported) override;
	bool getBatchInfo(supported) override;
	bool deleteJobById(supported) override;
	bool deleteJobByUser(supported) override;
	bool changeNodeState(supported) override;
	bool setQueueState(supported) override;
	bool runJob(supported) override;
	bool holdJob(supported) override;
	bool releaseJob(supported) override;
};

}
}
}

#endif /* __CW_BATCHSYSTEM_LSF_H__ */
