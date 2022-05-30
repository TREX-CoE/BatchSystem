#ifndef __CW_BATCHSYSTEM_SLURM_H__
#define __CW_BATCHSYSTEM_SLURM_H__

#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {
namespace slurm {

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
	cmd_f _func;
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


	static void parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert);
	
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

	static void parseNodes(const std::string& output, const std::function<getNodes_inserter_f>& insert);

	static void parseJobsLegacy(const std::string& output, std::function<getJobs_inserter_f> insert);
	static void parseJobsSacct(const std::string& output, std::function<getJobs_inserter_f> insert);

	std::function<bool(const std::function<getNodes_inserter_f>& insert)> getNodes(std::vector<std::string> filterNodes) override;

	/**
	 * \brief Get the Jobs object
	 * 
	 * Calls sacct to check wether it is available and queryable (slurmdbd running) and use that instead of scontrol for job infos
	 * if not set via \ref setJobMode before.
	 * 
	 * Uses a filter to only get active jobs.
	 * 
	 * \param insert 
	 * \param filterJobs jobs to filter
	 */
	std::function<bool(const std::function<getJobs_inserter_f>& insert)> getJobs(std::vector<std::string> filterJobs) override;
	std::function<bool(const std::function<getQueues_inserter_f>& insert)> getQueues() override;
	std::function<bool()> rescheduleRunningJobInQueue(const std::string& job, bool force) override;
	std::function<bool()> setQueueState(const std::string& name, QueueState state, bool force) override;
	std::function<bool()> resumeJob(const std::string& job, bool force) override;
	std::function<bool()> suspendJob(const std::string& job, bool force) override;
	std::function<bool()> deleteJobByUser(const std::string& user, bool force) override;
	std::function<bool()> deleteJobById(const std::string& job, bool force) override;
	std::function<bool()> holdJob(const std::string& job, bool force) override;
	std::function<bool()> releaseJob(const std::string& job, bool force) override;
	std::function<bool()> setNodeComment(const std::string& name, bool force, const std::string& comment, bool appendComment) override;
	/**
	 * \brief Change Node state
	 * 
	 * \note Slurm needs a reason for changing a node to a down / draining state, so a default is passed if no reason is given by user.
	 * 
	 * \param name 
	 * \param state 
	 * \param force 
	 * \param reason 
	 * \param appendReason 
	 */
	std::function<bool()> changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) override;
	std::function<bool(std::string&)> runJob(const JobOptions& opts) override;
	std::function<bool(bool&)> detect() override;
	std::function<bool(BatchInfo&)> getBatchInfo() override;

	/**
	 * \brief Check if sacct executable is present and can be queried
	 * 
	 * Calls "sacct --helpformat" to check if command can be called and works (slurmdbd running)
	 * 
	 * \param[out] sacctSupported Wether command succeeded with 0 exit code
	 */
	std::function<bool(bool&)> checkSacct();


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

#endif /* __CW_BATCHSYSTEM_SLURM_H__ */
