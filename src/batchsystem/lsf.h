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
	cmd_f _func;
public:
	Lsf(cmd_f func);

	static void parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert);
	static void parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert);
	static void parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert);

	std::function<bool(const std::function<getNodes_inserter_f>& insert)> getNodes(std::vector<std::string> filterNodes) override;
	std::function<bool(const std::function<getJobs_inserter_f>& insert)> getJobs(std::vector<std::string> filterJobs) override;
	std::function<bool(const std::function<getQueues_inserter_f>& insert)> getQueues() override;
	std::function<bool()> setQueueState(const std::string& name, QueueState state, bool force) override;
	std::function<bool()> deleteJobByUser(const std::string& user, bool force) override;
	std::function<bool()> deleteJobById(const std::string& job, bool force) override;
	std::function<bool()> holdJob(const std::string& job, bool force) override;
	std::function<bool()> releaseJob(const std::string& job, bool force) override;
	std::function<bool()> changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) override;
	std::function<bool(std::string& jobName)> runJob(const JobOptions& opts) override;
	std::function<bool(bool& detected)> detect() override;
	std::function<bool(BatchInfo& info)> getBatchInfo() override;


	bool getNodes(supported_t) override;
	bool getQueues(supported_t) override;
	bool getJobs(supported_t) override;
	bool getBatchInfo(supported_t) override;
	bool deleteJobById(supported_t) override;
	bool deleteJobByUser(supported_t) override;
	bool changeNodeState(supported_t) override;
	bool setQueueState(supported_t) override;
	bool runJob(supported_t) override;
	bool holdJob(supported_t) override;
	bool releaseJob(supported_t) override;
};

}
}
}

#endif /* __CW_BATCHSYSTEM_LSF_H__ */
