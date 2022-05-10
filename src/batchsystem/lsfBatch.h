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
class LsfBatch {
private:
	std::function<cmd_execute_f> _func;
public:
	LsfBatch(std::function<cmd_execute_f> func);

	static void parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert);
	static CmdOptions getNodesCmd();
	void getNodes(std::function<getNodes_inserter_f> insert) const;
	void getNodes(const std::vector<std::string>& filterNodes, std::function<getNodes_inserter_f> insert) const;
	static void parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert);
	static CmdOptions getJobsCmd();
	void getJobs(std::function<getJobs_inserter_f> insert) const;
	static void parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert);
	static CmdOptions getQueuesCmd();
	void getQueues(std::function<getQueues_inserter_f> insert) const;
	void setQueueState(const std::string& name, QueueState state, bool) const;
	void changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) const;
	void releaseJob(const std::string& job, bool) const;
	void holdJob(const std::string& job, bool) const;
	void deleteJobByUser(const std::string& user, bool) const;
	void deleteJobById(const std::string& job, bool) const;
};

}
}
}

#endif /* __CW_BATCHSYSTEM_LSF_BATCH_H__ */
