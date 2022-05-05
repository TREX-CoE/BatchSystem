#ifndef __CW_PBS_BATCH_H__
#define __CW_PBS_BATCH_H__

#include "batchSystem.h"
#include "batchSystem.h"

namespace cw {
namespace batch {
namespace pbs {

/**
 * \brief Concrete implementation of batchsystem for PBS
 * 
 * Wrapped within generic interface via \ref cw::batch::pbs::create_batch.
 * 
 */
class PbsBatch {
private:
	std::function<cmd_execute_f> _func;
public:
	PbsBatch(std::function<cmd_execute_f> func);
	
	std::string runJob(const JobOptions& opts) const;
	BatchInfo getBatchInfo() const;
	void getNodes(std::function<getNodes_inserter_f> insert) const;
	void getJobs(std::function<getJobs_inserter_f> insert) const;
	void getQueues(std::function<getQueues_inserter_f> insert) const;
	void setQueueState(const std::string& name, QueueState state, bool) const;
	void changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) const;
	void setNodeComment(const std::string& name, bool, const std::string& comment, bool appendComment) const;
	void releaseJob(const std::string& job, bool) const;
	void holdJob(const std::string& job, bool) const;
	void suspendJob(const std::string& job, bool) const;
	void resumeJob(const std::string& job, bool) const;
	void getJobsByUser(const std::string& user, std::function<bool(std::string)> inserter) const;
	void deleteJobByUser(const std::string& user, bool force) const;
	void deleteJobById(const std::string& job, bool force) const;
	void rescheduleRunningJobInQueue(const std::string& name, bool) const;
};


}
}
}

#endif /* __CW_PBS_BATCH_H__ */
