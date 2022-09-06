#ifndef __CW_BATCHSYSTEM_LSF_H__
#define __CW_BATCHSYSTEM_LSF_H__

#include "batchsystem/batchsystem.h"

#include <system_error>

namespace cw {
namespace batch {
namespace lsf {

enum class error {
    bkill_failed = 1,
    bkill_u_failed,
    node_change_state_undrain_unsupported_by_lsf,
    bresume_failed,
    bstop_failed,
    queue_state_inactive_unsupported_by_lsf,
    queue_state_draining_unsupported_by_lsf,
    badmin_failed,
    bhosts_failed,
    bqueues_failed,
    bsub_failed,
    bjob_u_all_failed,
    lsid_failed,
	bsub_cannot_parse_job_name,
};

const std::error_category& error_category() noexcept;

std::error_code make_error_code(error e);

/**
 * \brief Concrete implementation of batchsystem for LSF
 * 
 */
class Lsf : public BatchInterface {
private:
	cmd_f _cmd;
public:
	Lsf(cmd_f func);

	static void parseNodes(const std::string& output, std::vector<Node>& nodes);
	static void parseJobs(const std::string& output, std::vector<Job>& jobs);
	static void parseQueues(const std::string& output, std::vector<Queue>& queues);

	void getNodes(std::vector<std::string> filterNodes, std::function<void(std::vector<Node> nodes, std::error_code ec)> cb) override;
	void getJobs(std::vector<std::string> filterJobs, std::function<void(std::vector<Job> jobs, std::error_code ec)> cb) override;
	void getQueues(std::vector<std::string> filterQueues, std::function<void(std::vector<Queue> queues, std::error_code ec)> cb) override;
	void setQueueState(std::string name, QueueState state, bool force, std::function<void(std::error_code ec)> cb) override;
	void deleteJobByUser(std::string user, bool force, std::function<void(std::error_code ec)> cb) override;
	void deleteJobById(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void holdJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void releaseJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void changeNodeState(std::string name, NodeChangeState state, bool force, std::string reason, bool appendReason, std::function<void(std::error_code ec)> cb) override;
	void runJob(JobOptions opts, std::function<void(std::string jobName, std::error_code ec)> cb) override;
	void detect(std::function<void(bool has_batch, std::error_code ec)> cb) override;
	void getBatchInfo(std::function<void(BatchInfo info, std::error_code ec)> cb) override;



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

namespace std
{
  template <>
  struct is_error_code_enum<cw::batch::lsf::error> : true_type {};
}

#endif /* __CW_BATCHSYSTEM_LSF_H__ */
