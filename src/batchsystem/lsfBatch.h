#ifndef __CW_BATCHSYSTEM_LSF_BATCH_H__
#define __CW_BATCHSYSTEM_LSF_BATCH_H__

#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {
namespace lsf {

/**
 * \brief Concrete implementation of batchsystem for LSF
 * 
 * Wrapped within generic interface via \ref cw::batch::lsf::create_batch.
 * 
 */
class LsfBatch : public BatchInterface {
private:
	std::function<cmd_execute_f> _func;
public:
	LsfBatch(std::function<cmd_execute_f> func);

	static void parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert);
	static void parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert);
	static void parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert);

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

};

}
}
}

#endif /* __CW_BATCHSYSTEM_LSF_BATCH_H__ */
