#ifndef __CW_BASE_BATCH_SYSTEM_CATCHER__
#define __CW_BASE_BATCH_SYSTEM_CATCHER__

#include <string>
#include <vector>
#include <ctime>
#include <map>
#include <stdexcept>
#include <limits>
#include <cstdint>
#include <functional>

#include "policyOptional.h"

namespace cw {

/**
 * \brief Batchsystem interface
 * 
 * Generic batchsystem interface to allow querying / sending commands to different batchsystems.
 * 
 * \section usage Usage
 * 
 * \code
 * // define batchinterface variable
 * BatchInterface batch;
 * 
 * // implement / pass function to call shell commands
 * int cmd_executer(std::stringstream& out, const std::string& cmd, const std::vector<std::string>& args) {
 * 	  // ... execute cmd with args, write stdout to out and return exit code
 * }
 * 
 * // initialize interface for pbs
 * cw::batch::pbs::create_batch(batch, cmd_executer);
 * 
 * // use batch interface (NOTE: need to check for function if used implementation unknown)
 * if (batch.holdJob) batch.holdJob("job1", false); // hold job
 * \endcode
 */
namespace batch {

using cw::policy_optional::string_optional;
using cw::policy_optional::max_optional;

/**
 * \brief Node state enum
 * 
 * Enum representing node state from various batchsystems as a set of unified states.
 * 
 */
enum class NodeState {
	Unknown, //!< Unknown / unhandled or invalid node state
	Offline, //!< Node is offline
	Online, //!< Node is online and ready to run jobs
	Disabled, //!< Node is disabled from scheduling jobs on
	Various, //!< Node/Nodes/Nodegroup have different states
	Powersave, //!< Node is in powersave mode
	Reserved, //!< Node is reservered
	Maintainence, //!< Node is disabled for maintainence
	Failed, //!< Node is a failed / error state
};

/**
 * \brief Events to change Node state
 * 
 * Enum containing all events to change Node state.
 */
enum class NodeChangeState {
	Resume, //!< Remove Offline state
	Drain, //!< Mark node to disable jobs being scheduled on
	Undrain, //!< Mark node to allow jobs being scheduled on
};

/**
 * \brief Node informations
 * 
 * Struct containing all node informations parsed from batchsystem.
 * 
 * \warning
 * Fields can be empty / not supported by certain batchsystem implementation.
 * Check field with \ref is_empty first
 */
struct Node {
	string_optional name; //!< Shortened (hostname only) node name
	string_optional nameFull; //!< Full node name
	string_optional comment; //!< User settable comment for node
	string_optional reason; //!< Reason why node is in state (set by user/batchsystem)
	NodeState state {NodeState::Unknown}; //!< Common / unified node state enum
	string_optional rawState; //!< Raw node state (unmodified)
	std::map<std::string, uint32_t> cpusOfJobs; //!< Mapping for jobs on node with the number of cpus allocated by each one
	max_optional<uint32_t> cpus; //!< Number of cpus available
	max_optional<uint32_t> cpusReserved; //!< Number of cpus reserved
	max_optional<uint32_t> cpusUsed; //!< Number of cpus used
};

/**
 * \brief Queue state enum
 * 
 * Enum representing queue state from various batchsystems as a set of unified states.
 * 
 */
enum class QueueState {
	Unknown, //!< Unknown / unhandled or invalid queue state
	Open, //!< Open queue, jobs can be scheduled and run
	Closed, //!< Closed queue, jobs can neither be scheduled nor run
	Inactive, //!< Queue is inactive, jobs can be scheduled but will not run (yet)
	Draining, //!< Queue currently draining, jobs cannot be scheduled but existing still run
};

/**
 * \brief Queue informations
 * 
 * Struct containing all queue informations parsed from batchsystem.
 */
struct Queue {
	string_optional name; //!< Name / identifier of queue
	max_optional<uint32_t> priority; //!< Priority of queue
	QueueState state {QueueState::Unknown}; //!< Common / unified queue state enum
	string_optional rawState; //!< Raw queue state (unmodified)
	max_optional<uint32_t> jobsTotal; //!< Total number of jobs in queue
	max_optional<uint32_t> jobsPending; //!< Number of jobs scheduled for execution in queue
	max_optional<uint32_t> jobsRunning; //!< Number of jobs running in queue
	max_optional<uint32_t> jobsSuspended; //!< Number of previously running jobs that are suspended
	max_optional<uint32_t> cpus; //!< Total number of available cpus in queue
	max_optional<uint32_t> nodesTotal; //!< Total number of nodes in queue
	max_optional<std::time_t> modifyTime; //!< Last time the queue was modified
	std::vector<std::string> nodes; //!< All nodes by name in queue 
	max_optional<uint32_t> nodesMax; //!< Maximum number of nodes per job
	max_optional<uint32_t> nodesMin; //!< Minimum number of nodes per job
	max_optional<std::time_t> wallTimeMax; //!< Maximum walltime per job
	max_optional<uint64_t> memPerCpuMax; //!< Maximum memory per cpu per job in bytes
};

/**
 * \brief Job state enum
 * 
 * Enum representing job state from various batchsystems as a set of unified states.
 * 
 */
enum class JobState {
	Unknown, //!< Unknown / unhandled or invalid job state
	Running, //!< Job is currently running
	Queued, //!< Job is scheduled for running
	Requeued, //!< Job is scheduled after being run already
	Terminating, //!< Job is currently terminating
	Finished, //!< Job already finished
	Cancelled, //!< Job has been cancelled
	Failed, //!< Job failed to run
	Moved, //!< Job is been moved to another node
	Suspend, //!< Job has been paused from running
	Zombie, //!< Job failed in a possibly unstoppable zombie state
};

/**
 * \brief Convert job state enum to string.
 * 
 * \param state Job state enum
 * \return Enum string representation
 */
const char* to_cstr(const JobState& state);

/**
 * \brief Convert queue state enum to string.
 * 
 * \param state Queue state enum
 * \return Enum string representation
 */
const char* to_cstr(const QueueState& state);

/**
 * \brief Convert node state enum to string.
 * 
 * \param state Node state enum
 * \return Enum string representation
 */
const char* to_cstr(const NodeState& state);

/**
 * \brief Batchsystem informations
 * 
 * Struct containing all batchsystem meta informations
 */
struct BatchInfo {
	string_optional name; //!< Name of batchsystem
	string_optional version; //!< Version string of batchsystem
	std::map<std::string, std::string> info; //!< Key-Value pairs for features / config of batchsystem and additional informations
};

/**
 * \brief Job run options
 * 
 * Struct containing all options to run job via BatchSystem.runJob.
 */
struct JobOptions {
	string_optional path; //!< Path to job script to execute
	max_optional<uint32_t> numberNodes; //!< Number of nodes requested
	max_optional<uint32_t> numberNodesMax; //!< Maximum number of nodes requested
	max_optional<uint32_t> numberTasks; //!< Number of tasks requested
	max_optional<uint32_t> numberGpus; //!< Number of gpus requested
};

/**
 * \brief Job informations
 * 
 * Struct containing all job informations parsed from batchsystem.
 */
struct Job {
	string_optional id; //!< Job Id used by batchsystem 
	string_optional idCompact; //!< shortened Job Id
	string_optional name; //!< Name of job
	string_optional owner; //!< Owner of job
	string_optional user; //!< Username of user that triggered job
	max_optional<uint32_t> userId; //!< User Id of user that triggered job
	string_optional group; //!< Groupname of user that triggered job
	max_optional<uint32_t> groupId; //!< Group Id of user that triggered job
	string_optional queue; //!< Name of queue job is currently in
	max_optional<uint32_t> priority; //!< Priority of job
	string_optional execHost; //!< Hostname where job is executed
	string_optional submitHost; //!< Hostname where job has been submitted from
	string_optional server; //!< Server managing job
	string_optional reason; //!< Reason why job is in its current state
	string_optional comment; //!< User set comment for job
	string_optional commentSystem; //!< System set comment for job
	string_optional commentAdmin; //!< Admin set comment for job
	string_optional workdir; //!< Workdirectory used for executing job
	max_optional<int32_t> exitCode; //!< Exitcode of job if finished
	max_optional<int32_t> exitSignal; //!< Signal Id of job if finished
	max_optional<std::time_t> submitTime; //!< Time the job has been submitted
	max_optional<std::time_t> startTime; //!< Time the job was started
	max_optional<std::time_t> endTime; //!< Time the job finished
	max_optional<std::time_t> queueTime; //!< Time the job has been queued for execution
	max_optional<std::time_t> wallTimeRequested; //!< Walltime requested by job in seconds
	max_optional<std::time_t> wallTimeUsed; //!< Walltime used by job in seconds
	max_optional<std::time_t> cpuTimeUsed; //!< Cputime used by job in seconds
	max_optional<uint64_t> memUsed; //!< Memory used by job in bytes
	max_optional<uint64_t> vmemUsed; //!< Virtual memory used by job in bytes
	JobState state {JobState::Unknown}; //!< Common / unified job state enum
	string_optional rawState; //!< Raw job state (unmodified)
	max_optional<uint32_t> cpusPerNode; //!< Number of cpus used per node
	max_optional<uint32_t> nodesRequested; //!< Number of nodes requested
	max_optional<uint32_t> nodes; //!< Number of nodes allocated for job
	max_optional<uint32_t> cpusRequested; //!< Number of cpus requested
 	max_optional<uint32_t> cpus; //!< Number of cpus allocated for job
	max_optional<uint32_t> niceRequested; //!< Nice level requested by job
	string_optional otherRequested; //!< Other /custom required string field
	std::map<std::string, std::string> variableList; //!< Environment variables used for job execution
	string_optional submitArgs; //!< Arguments passed while submitting job
	std::map<std::string, std::string> requestedGeneral; //!< General key-value pairs for requested custom resources
	std::map<std::string, uint32_t> cpusFromNode; //!< Mapping of nodenames and number of cpus from each node used by the job
};

/**
 * \brief Exception for failed command execution
 * 
 * Exception supposed to be thrown from within BatchSystem implementation if a shell command failed.
 */
class CommandFailed : public std::runtime_error {
private:	
	int retno;
public:
	/**
	 * \brief Construct a new CommandFailed exception
	 * 
	 * \param msg Custom error message
	 * \param cmd Command name that failed
	 * \param ret Return code of failed command
	 */
	CommandFailed(const std::string& msg, const std::string& cmd, int ret);

	/**
	 * \brief Get return code
	 * 
	 * Get return code of failed command.
	 * 
	 * \return Return code
	 */
	int returncode() const;
};

/**
 * \brief Get next Node struct from batchsystem
 * 
 * Iterator like callback that passes next Node as parameter and allows stopping via return value.
 * 
 * \param node Next Node parsed from batchsystem
 * 
 * \return Wether to stop iterating
 */
typedef bool getNodes_inserter_f(Node node);

/**
 * \brief Get Node structs information from batchsystem
 * 
 * Query batchsystem for node informations.
 * 
 * \param insert Callback to get next Node
 */
typedef void getNodes_f(std::function<getNodes_inserter_f> insert);

/**
 * \brief Get next Job struct from batchsystem
 * 
 * Iterator like callback that passes next Job as parameter and allows stopping via return value.
 * 
 * \param job Next Job parsed from batchsystem
 * 
 * \return Wether to stop iterating 
 */
typedef bool getJobs_inserter_f(Job job);

/**
 * \brief Get Job structs information from batchsystem
 * 
 * Query batchsystem for job informations.
 * 
 * \param insert Callback to get next Job
 */
typedef void getJobs_f(std::function<getJobs_inserter_f> insert);

/**
 * \brief Get next Queue struct from batchsystem
 * 
 * Iterator like callback that passes next Queue as parameter and allows stopping via return value.
 * 
 * \param job Next Queue parsed from batchsystem
 * 
 * \return Wether to stop iterating 
 */
typedef bool getQueues_inserter_f(Queue queue);

/**
 * \brief Get Queue structs information from batchsystem
 * 
 * Query batchsystem for queue informations.
 * 
 * \param insert Callback to get next Queue
 */
typedef void getQueues_f(std::function<getQueues_inserter_f> insert);

/**
 * \brief Get batchsystem information
 * 
 * Get metadata for used batchsystem and version.
 * 
 * \return batchsystem info metadata 
 */
typedef BatchInfo getBatchInfo_f();

/**
 * \brief Delete job by Id
 * 
 * \param job Id of job to delete
 * \param force Force delete
 */
typedef void deleteJobById_f(const std::string& job, bool force);

/**
 * \brief Delete job(s) by user
 * 
 * \param user User of job(s) to delete
 * \param force Force delete
 */
typedef void deleteJobByUser_f(const std::string& user, bool force);

/**
 * \brief Change Node state
 * 
 * \param name Name of node to change
 * \param state Event to trigger to change node state
 * \param force Wether to force node change
 * \param reason Reason of node change
 * \param appendReason Wether reason should be appended instead of overwritten
 */
typedef void changeNodeState_f(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason);

/**
 * \brief Set the QueueState
 * 
 * Change the queue state.
 * 
 * \param name Name of the queue to change state
 * \param state State to change to
 * \param force Wether to force state change
 */
typedef void setQueueState_f(const std::string& name, QueueState state, bool force);

/**
 * \brief Schedule job to run on batchsystem
 * 
 * \param opts Options for job to run
 * \return Job Id of scheduled job 
 */
typedef std::string runJob_f(const JobOptions& opts);

/**
 * \brief Set the node comment
 * 
 * \param name Node to set comment for
 * \param comment Comment string to set
 * \param appendComment Wether to append comment instead of overwritting
 */
typedef void setNodeComment_f(const std::string& name, bool, const std::string& comment, bool appendComment);

/**
 * \brief Prevent a pending job from being run
 * 
 * Reverse of \ref releaseJob_f operation.
 * 
 * \param job Id of job to hold
 * \param force Wether to force hold
 */
typedef void holdJob_f(const std::string& job, bool force);

/**
 * \brief Release a job that has been hold before to start execution
 * 
 * Reverse of \ref holdJob_f operation.
 * 
 * \param job Id of job to release
 * \param force Wether to force release
 */
typedef void releaseJob_f(const std::string& job, bool force);

/**
 * \brief Suspend a running job
 * 
 * Reverse of \ref resumeJob_f operation.
 * 
 * \param job Id of job to suspend
 * \param force Wether to force suspend
 */
typedef void suspendJob_f(const std::string& job, bool force);

/**
 * \brief Resume a job that was suspended before
 * 
 * Reverse of \ref suspendJob_f operation.
 * 
 * \param job Id of job to resume
 * \param force Wether to force resume
 */
typedef void resumeJob_f(const std::string& job, bool force);


/**
 * \brief Requeue a job into a waiting state
 * 
 * Running / supended / finished job is put back into a pending state.
 * 
 * \param job Id of job to requeue
 * \param force Wether to force reque
 */
typedef void rescheduleRunningJobInQueue_f(const std::string& job, bool force);

struct CmdOptions {
	std::string cmd;
	std::vector<std::string> args;
};

/**
 * \brief Callback for batchsystem implementations to call shell command.
 * 
 * \param[out] out Shell output is passed to this stringstream
 * \param cmd Command to run
 * \param args Arguments to pass to command
 */
typedef int cmd_execute_f(std::string& out, const CmdOptions& opts);

/**
 * \brief Struct of batchsystem interface functions
 * 
 * This is the public interface used by the provided pbs / slurm / lsf implementations,
 * that can be also used for custom implementations.
 * 
 * \note
 * Function objects can throw exceptions to signal errors.
 * Provided implementations do and custom implementations should throw a \ref CommandFailed exception,
 * if they call a shell command that fails.
 * 
 * \warning
 * Each function object can be null, meaning not implemented/supported.
 * So null checking is required before calling.
 */
struct BatchSystem
{
	std::function<getNodes_f> getNodes;
	std::function<getJobs_f> getJobs;
	std::function<getQueues_f> getQueues;

	std::function<getBatchInfo_f> getBatchInfo;

	std::function<deleteJobById_f> deleteJobById;
	std::function<deleteJobByUser_f> deleteJobByUser;

	std::function<changeNodeState_f> changeNodeState;
	std::function<setQueueState_f> setQueueState;
	
	std::function<runJob_f> runJob;
	std::function<setNodeComment_f> setNodeComment;
	std::function<holdJob_f> holdJob;
	std::function<releaseJob_f> releaseJob;
	std::function<suspendJob_f> suspendJob;
	std::function<resumeJob_f> resumeJob;
	std::function<rescheduleRunningJobInQueue_f> rescheduleRunningJobInQueue;
};

}
}

#endif /* __CW_BASE_BATCH_SYSTEM_CATCHER__ */
