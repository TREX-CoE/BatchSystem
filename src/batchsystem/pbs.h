#ifndef __CW_BATCHSYSTEM_PBS_H__
#define __CW_BATCHSYSTEM_PBS_H__

#include "batchsystem/batchsystem.h"

#include <system_error>

namespace cw {
namespace batch {
namespace pbs {

enum class error {
    pbsnodes_x_xml_parse_error = 1,
    qselect_u_failed,
    qdel_failed,
    qstat_f_x_xml_parse_error,
    pbsnodes_change_node_state_failed,
    pbsnodes_set_comment_failed,
    qrls_failed,
    qhold_failed,
    qsig_s_suspend_failed,
    qsig_s_resume_failed,
    qmgr_c_set_queue_failed,
    qrerun_failed,
    pbsnodes_x_failed,
    qstat_Qf_failed,
    qsub_failed,
    qstat_f_x_failed,
    pbsnodes_version_failed,
};

const std::error_category& error_category() noexcept;

std::error_code make_error_code(error e);

/**
 * \brief Concrete implementation of batchsystem for PBS
 * 
 */
class Pbs : public BatchInterface {
private:
	cmd_f _cmd;
public:
	Pbs(cmd_f func);

	static void parseNodes(const std::string& output, std::vector<Node>& nodes);
	static void parseJobs(const std::string& output, std::vector<Job>& jobs);
	static void parseQueues(const std::string& output, std::vector<Queue>& queues);


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


	void getJobsByUser(std::string user, std::function<void(std::vector<std::string> jobs, std::error_code ec)> cb);
	void deleteJobsById(std::vector<std::string> jobs, bool force, std::function<void(std::error_code ec)> cb);

	void getNodes(std::vector<std::string> filterNodes, std::function<void(std::vector<Node> nodes, std::error_code ec)> cb) override;
	void getJobs(std::vector<std::string> filterJobs, std::function<void(std::vector<Job> jobs, std::error_code ec)> cb) override;
	void getQueues(std::vector<std::string> filterQueues, std::function<void(std::vector<Queue> queues, std::error_code ec)> cb) override;
	void rescheduleRunningJobInQueue(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void setQueueState(std::string name, QueueState state, bool force, std::function<void(std::error_code ec)> cb) override;
	void resumeJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void suspendJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void deleteJobByUser(std::string user, bool force, std::function<void(std::error_code ec)> cb) override;
	void deleteJobById(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void holdJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void releaseJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void setNodeComment(std::string name, bool force, std::string comment, bool appendComment, std::function<void(std::error_code ec)> cb) override;
	void changeNodeState(std::string name, NodeChangeState state, bool force, std::string reason, bool appendReason, std::function<void(std::error_code ec)> cb) override;
	void runJob(JobOptions opts, std::function<void(std::string jobName, std::error_code ec)> cb) override;
	void detect(std::function<void(bool has_batch, std::error_code ec)> cb) override;
	void getBatchInfo(std::function<void(BatchInfo info, std::error_code ec)> cb) override;

};


}
}
}


namespace std
{
  template <>
  struct is_error_code_enum<cw::batch::pbs::error> : true_type {};
}

#endif /* __CW_BATCHSYSTEM_PBS_H__ */
