#ifndef __CW_BATCHSYSTEM_SLURM_H__
#define __CW_BATCHSYSTEM_SLURM_H__

#include "batchsystem/batchsystem.h"

#include <system_error>

namespace cw {
namespace batch {
namespace slurm {

enum class error {
    sacct_X_P_format_ALL_failed = 1,
    scontrol_show_job_all_failed,
    scontrol_version_failed,
    scontrol_show_config_failed,
    scontrol_update_NodeName_State_failed,
    scontrol_update_NodeName_Comment_failed,
    scontrol_release_failed,
    scontrol_hold_failed,
    scancel_failed,
    scancel_u_failed,
    scontrol_suspend_failed,
    scontrol_resume_failed,
    scontrol_update_PartitionName_State_failed,
    scontrol_requeuehold_failed,
    scontrol_requeue_failed,
    scontrol_show_node_failed,
    scontrol_show_partition_all_failed,
    sbatch_failed,
    slurm_job_mode_out_of_enum,
};


const std::error_category& error_category() noexcept;

std::error_code make_error_code(error e);

/**
 * \brief Concrete implementation of batchsystem for Slurm
 * 
 */
class Slurm : public BatchInterface {
public:
	enum class job_mode {
		unchecked, //!< auto detect whether sacct is supported or fallback to scontrol
		sacct, //!< use sacct
		scontrol, //!< use scontrol
	};
private:
	cmd_f _cmd;
	/**
	 * @brief Which command to use for getting job info
	 * 
	 * Determines which command to use in \ref getJobs.
	 * 
	 */
	job_mode _mode;
public:
	static const std::string DefaultReason;

	/**
	 * \brief Initialize batchsystem interface with Slurm implementation
	 * 
	 * \param func Function to call shell commands
	 */
	Slurm(cmd_f func);

	static void parseQueues(const std::string& output, std::vector<Queue>& queues);
	
	/**
	 * \brief Set to use sacct or scontrol
	 * 
	 * \param mode Wether to use sacct instead of scontrol
	 */
	void setJobMode(Slurm::job_mode mode);

	/**
	 * \brief Get wether sacct is used.
	 */
	Slurm::job_mode getJobMode() const;

	/**
	 * \brief Convert generic Key-Value mapping to job struct with correct datatypes
	 * 
	 * Can convert info from \ref parseSacct or \ref parseShowJob
	 * 
	 * \param j Mapping to convert
	 * \param[out] job Job to write values to
	 */
	static void jobMapToStruct(const std::map<std::string, std::string>& j, Job& job);

	/**
	 * \brief Parse output of sacct
	 * 
	 * \param output Sacct output to parse
	 * \param insert Function to pass Map Key-Value-Pairs for each job parsed
	 */
	static void parseSacct(const std::string& output, std::function<bool(std::map<std::string, std::string>)> insert);

	/**
	 * \brief Parse output of scontrol show jobs
	 * 
	 * \param output Scontrol output to parse
	 * \param insert Function to pass Map Key-Value-Pairs for each job parsed
	 */
	static void parseShowJob(const std::string& output, std::function<bool(std::map<std::string, std::string>)> insert);

	static void parseNodes(const std::string& output, std::vector<Node>& nodes);

	static void parseJobsLegacy(const std::string& output, std::vector<Job>& jobs);
	static void parseJobsSacct(const std::string& output, std::vector<Job>& jobs);

	void getNodes(std::vector<std::string> filterNodes, std::function<void(std::vector<Node> nodes, std::error_code ec)> cb) override;

	/**
	 * \brief Get the Jobs object
	 * 
	 * Calls sacct to check wether it is available and queryable (slurmdbd running) and use that instead of scontrol for job infos
	 * if not set via \ref setJobMode before.
	 * 
	 * Uses a filter to only get active jobs.
	 */
	void getJobs(std::vector<std::string> filterJobs, std::function<void(std::vector<Job> jobs, std::error_code ec)> cb) override;
	void getJobsSacct(std::vector<std::string> filterJobs, std::string stateFilter, std::function<void(std::vector<Job> jobs, std::error_code ec)> cb);
	void getJobsLegacy(std::function<void(std::vector<Job> jobs, std::error_code ec)> cb);

	void getQueues(std::function<void(std::vector<Queue> queues, std::error_code ec)> cb) override;
	void rescheduleRunningJobInQueue(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void setQueueState(std::string name, QueueState state, bool force, std::function<void(std::error_code ec)> cb) override;
	void resumeJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void suspendJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void deleteJobByUser(std::string user, bool force, std::function<void(std::error_code ec)> cb) override;
	void deleteJobById(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void holdJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void releaseJob(std::string job, bool force, std::function<void(std::error_code ec)> cb) override;
	void setNodeComment(std::string name, bool force, std::string comment, bool appendComment, std::function<void(std::error_code ec)> cb) override;
	/**
	 * \brief Change Node state
	 * 
	 * \note Slurm needs a reason for changing a node to a down / draining state, so a default is passed if no reason is given by user.
	 */
	void changeNodeState(std::string name, NodeChangeState state, bool force, std::string reason, bool appendReason, std::function<void(std::error_code ec)> cb) override;
	void runJob(JobOptions opts, std::function<void(std::string jobName, std::error_code ec)> cb) override;
	void detect(std::function<void(bool has_batch, std::error_code ec)> cb) override;
	void getBatchInfo(std::function<void(BatchInfo info, std::error_code ec)> cb) override;

	

	/**
	 * \brief Check if sacct executable is present and can be queried
	 * 
	 * Calls "sacct --helpformat" to check if command can be called and works (slurmdbd running)
	 * 
	 * \param[out] sacctSupported Wether command succeeded with 0 exit code
	 * \param cb Callback to get whether sacct is supported and possible error state
	 */
	void checkSacct(std::function<void(bool has_sacct, std::error_code ec)> cb);

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
};

}
}
}

namespace std
{
  template <>
  struct is_error_code_enum<cw::batch::slurm::error> : true_type {};
}

#endif /* __CW_BATCHSYSTEM_SLURM_H__ */
