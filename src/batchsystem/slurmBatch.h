#ifndef __CW_BATCHSYSTEM_SLURM_BATCH_H__
#define __CW_BATCHSYSTEM_SLURM_BATCH_H__

#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {
namespace slurm {

/**
 * \brief Concrete implementation of batchsystem for Slurm
 * 
 * Wrapped within generic interface via \ref cw::batch::slurm::create_batch.
 * 
 */
class SlurmBatch : public BatchInterface {
	std::function<cmd_execute_f> _func;
	std::map<CmdOptions, std::string> _cache;
	enum class job_mode {
		unchecked,
		sacct,
		scontrol,
	};
	job_mode _mode;
public:
	static const std::string DefaultReason;

	/**
	 * \brief Initialize batchsystem interface with Slurm implementation (autodetect sacct)
	 * 
	 * \param[out] inf Interface object to set
	 * \param _func Function to call shell commands
	 */
	SlurmBatch(std::function<cmd_execute_f> func);


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
	void setJobMode(job_mode mode);

	/**
	 * @brief Get wether sacct is used.
	 */
	job_mode getJobMode() const;

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

	static void parseNodes(const std::string& output, std::function<getNodes_inserter_f> insert);

	static void parseJobs(const std::string& output, std::function<getJobs_inserter_f> insert);

	static void parseJobsLegacy(const std::string& output, std::function<getJobs_inserter_f> insert);

	/**
	 * \brief Explicitly get jobs via scontrol
	 * 
	 * \param insert 
	 */
	bool getJobsLegacy(std::function<getJobs_inserter_f> insert);

	static void parseJobsSacct(const std::string& output, std::function<getJobs_inserter_f> insert);

	/**
	 * \brief Explicitly get jobs via acct
	 * 
	 * Uses a filter to only get active jobs.
	 * 
	 * \param insert 
	 */
	bool getJobsSacct(std::function<getJobs_inserter_f> insert);

	/**
	 * \brief Explicitly get jobs via acct
	 * 
	 * Allows filtering jobs manually.
	 * 
	 * \param insert 
	 * \param stateFilter Comma separated States to filter
	 * \param filterJobs jobs to filter
	 */
	bool getJobsSacct(std::function<getJobs_inserter_f> insert, const std::string& stateFilter);
	bool getJobsSacct(std::function<getJobs_inserter_f> insert, const std::string& stateFilter, const std::vector<std::string>& filterJobs);

	bool detect(bool& detected) override;
	void resetCache() override;
	bool getNodes(const std::vector<std::string>& filterNodes, std::function<getNodes_inserter_f> insert) override;
	bool getQueues(std::function<getQueues_inserter_f> insert) override;

	/**
	 * @brief Get the Jobs object
	 * 
	 * Calls sacct to check wether it is available and queryable (slurmdbd running) and use that instead of scontrol for job infos
	 * if not set via \ref useSacct before.
	 * 
	 * @param insert 
	 * @return true 
	 * @return false 
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
};

}
}
}

#endif /* __CW_BATCHSYSTEM_SLURM_BATCH_H__ */
