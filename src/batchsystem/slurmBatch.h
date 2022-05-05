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
class SlurmBatch {
	std::function<cmd_execute_f> _func;
	bool _useSacct;
public:
	static const std::string DefaultReason;
	SlurmBatch(std::function<cmd_execute_f> func);
	SlurmBatch(std::function<cmd_execute_f> func, bool useSacct);

	/**
	 * \brief Check if sacct executable is present and can be queried
	 * 
	 * Calls "sacct --helpformat" to check if command can be called and works (slurmdbd running)
	 * 
	 * \return Wether command succeeded with 0 exit code
	 */
	bool checkSacct() const;

	/**
	 * \brief Set to use sacct or scontrol
	 * 
	 * \param useSacct Wether to use sacct instead of scontrol
	 */
	void useSacct(bool useSacct);

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

	std::string runJob(const JobOptions& opts) const;
	BatchInfo getBatchInfo() const;
	void getNodes(std::function<getNodes_inserter_f> insert) const;
	void getJobs(std::function<getJobs_inserter_f> insert) const;

	/**
	 * \brief Explicitly get jobs via scontrol
	 * 
	 * \param insert 
	 */
	void getJobsLegacy(std::function<getJobs_inserter_f> insert) const;

	/**
	 * \brief Explicitly get jobs via acct
	 * 
	 * Uses a filter to only get active jobs.
	 * 
	 * \param insert 
	 */
	void getJobsSacct(std::function<getJobs_inserter_f> insert) const;

	/**
	 * \brief Explicitly get jobs via acct
	 * 
	 * Allows filtering jobs manually.
	 * 
	 * \param insert 
	 * \param stateFilter Comma separated States to filter
	 */
	void getJobsSacct(std::function<getJobs_inserter_f> insert, const std::string& stateFilter) const;

	void getQueues(std::function<getQueues_inserter_f> insert) const;
	void setNodeComment(const std::string& name, bool, const std::string& comment, bool) const;

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
	void changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason) const;
	void releaseJob(const std::string& job, bool) const;
	void holdJob(const std::string& job, bool) const;
	void deleteJobById(const std::string& job, bool) const;
	void deleteJobByUser(const std::string& user, bool) const;
	void suspendJob(const std::string& job, bool) const;
	void resumeJob(const std::string& job, bool) const;
	void setQueueState(const std::string& name, QueueState state, bool) const;
	void rescheduleRunningJobInQueue(const std::string& job, bool hold) const;
};

}
}
}

#endif /* __CW_BATCHSYSTEM_SLURM_BATCH_H__ */
