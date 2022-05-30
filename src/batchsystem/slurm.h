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
	std::function<cmd_execute_f> _func;
	cmd_f _f;
	std::map<CmdOptions, std::string> _cache;
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
	Slurm(std::function<cmd_execute_f> func);


	static void parseQueues(const std::string& output, std::function<getQueues_inserter_f> insert);
	
	/**
	 * \brief Check if sacct executable is present and can be queried
	 * 
	 * Calls "sacct --helpformat" to check if command can be called and works (slurmdbd running)
	 * 
	 * \param[out] sacctSupported Wether command succeeded with 0 exit code
	 */
	bool checkSacct(bool& sacctSupported);

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

	static void parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert);

	static void parseJobsLegacy(const std::string& output, std::function<getJobs_inserter_f> insert);

	/**
	 * \brief Explicitly get jobs via scontrol
	 * 
	 * \param insert 
	 */
	bool getJobsLegacy(std::function<getJobs_inserter_f> insert) const;

	static void parseJobsSacct(const std::string& output, std::function<getJobs_inserter_f> insert);

	/**
	 * \brief Explicitly get jobs via acct
	 * 
	 * Uses a filter to only get active jobs.
	 * 
	 * \param insert 
	 */
	bool getJobsSacct(std::function<getJobs_inserter_f> insert) const;

	/**
	 * \brief Explicitly get jobs via acct
	 * 
	 * Allows filtering jobs manually.
	 * 
	 * \param insert 
	 * \param stateFilter Comma separated States to filter
	 * \param filterJobs jobs to filter
	 */
	bool getJobsSacct(std::function<getJobs_inserter_f> insert, const std::string& stateFilter, const std::vector<std::string>& filterJobs) const;

	bool detect(bool& detected) override;
	void resetCache() override;
	bool getNodes(const std::vector<std::string>& filterNodes, std::function<getNodes_inserter_f> insert) override;
	bool getQueues(std::function<getQueues_inserter_f> insert) override;

	/**
	 * \brief Get the Jobs object
	 * 
	 * Calls sacct to check wether it is available and queryable (slurmdbd running) and use that instead of scontrol for job infos
	 * if not set via \ref setJobMode before.
	 * 
	 * \param insert 
	 */
	bool getJobs(std::function<getJobs_inserter_f> insert) override;
	bool getBatchInfo(BatchInfo& info) override;
	bool deleteJobById(const std::string& job, bool force) override;
	bool deleteJobByUser(const std::string& user, bool force) override;

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
	bool changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) override;
	bool setQueueState(const std::string& name, QueueState state, bool force) override;
	bool runJob(const JobOptions& opts, std::string& jobName) override;
	bool setNodeComment(const std::string& name, bool, const std::string& comment, bool appendComment) override;
	bool holdJob(const std::string& job, bool force) override;
	bool releaseJob(const std::string& job, bool force) override;
	bool suspendJob(const std::string& job, bool force) override;
	bool resumeJob(const std::string& job, bool force) override;
	bool rescheduleRunningJobInQueue(const std::string& job, bool force) override;

	std::function<bool(const std::function<getNodes_inserter_f>& insert)> getNodes2(std::vector<std::string> filterNodes);
	std::function<bool(const std::function<getJobs_inserter_f>& insert)> getJobs2(std::vector<std::string> filterJobs);
	std::function<bool(const std::function<getQueues_inserter_f>& insert)> getQueues2();
	std::function<bool()> rescheduleRunningJobInQueue2(const std::string& job, bool force);
	std::function<bool()> setQueueState2(const std::string& name, QueueState state, bool force);
	std::function<bool()> resumeJob2(const std::string& job, bool force);
	std::function<bool()> suspendJob2(const std::string& job, bool force);
	std::function<bool()> deleteJobByUser2(const std::string& user, bool force);
	std::function<bool()> deleteJobById2(const std::string& job, bool force);
	std::function<bool()> holdJob2(const std::string& job, bool force);
	std::function<bool()> releaseJob2(const std::string& job, bool force);
	std::function<bool()> setNodeComment2(const std::string& name, bool force, const std::string& comment, bool appendComment);
	std::function<bool()> changeNodeState2(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason);
	std::function<bool(std::string&)> runJob2(const JobOptions& opts);
	std::function<bool(bool&)> detect2();
	std::function<bool(bool&)> checkSacct2();
	std::function<bool(BatchInfo&)> getBatchInfo2();


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
