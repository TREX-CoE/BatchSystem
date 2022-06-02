#ifndef __CW_BATCHSYSTEM_BATCHSYSTEM__
#define __CW_BATCHSYSTEM_BATCHSYSTEM__

/**
 * \mainpage Batchsystem
 * 
 * Batchsystem is a generic wrapper for different batchsystem implementations like PBS / Slurm or LSF
 * to provide a uniform interface to query batchsystem infos and trigger actions.
 * 
 * \note
 * For the complete API documentation go to this namespace:
 * \ref cw::batch
 */

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
 * \brief Special values for exit code
 */
enum exitcode_marker {
	not_finished = -1 //!< Sentinel value of command exit code integer (\ref Result) to signal that asynchronous command has not finished yet.
};

/**
 * \brief Result object of shell command
 * 
 * Store result of a running shell command in \ref cmd_f.
 */
struct Result {
    int exit = not_finished; //!< exit code of command (\ref not_finished if not done yet)
    std::string out; //!< captured stdout
    std::string err; //!< captured stderr
};

namespace cmdopt {
	/**
	 * \brief Command run options
	 * 
	 * Bitmask to set options for executing shell commands.
	 * 
	 */
	enum cmdopt {
		none = 0, //!< dont capture any output
		capture_stdout = (1 << 0), //!< request capture of stdout
		capture_stderr = (1 << 1), //!< request capture of stderr
		capture_stdout_stderr = (capture_stdout | capture_stderr), //!< request capture of stdout and stderr
	};
}

/**
 * \brief Shell command and arguments
 * 
 * Contains args and options to run command in \ref cmd_f.
 */
struct Cmd {
    std::string cmd; //!< command to run
    std::vector<std::string> args; //!< arguments to pass to command
    std::map<std::string, std::string> envs; //!< envvars to pass to command
    cmdopt::cmdopt opts; //!< options to adapt way command is run
};

/**
 * \brief Callback for batchsystem implementations to call shell command.
 * 
 * \param[out] res Store outputs and exit codes in result object
 * \param cmd Command, arguments and options to run command
 */
using cmd_f = std::function<void(Result& res, const Cmd& cmd)>;


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
public:
	using std::runtime_error::runtime_error;
};

/**
 * \brief Exception to signal not implemented virtual method.
 * 
 * Used in \ref BatchInterface for optional methods.
 * 
 */
class NotImplemented : public std::logic_error
{
public:
	/**
	 * \brief Construct a new NotImplemented exception
	 * 
	 * \param msg String literal for not implemented function name (e.g. pass `__func__` macro)
	 */
    NotImplemented(const char* msg);
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
 * \brief Tag to check whether method is supported / implemented.
 * 
 * Special empty struct value tag to check whether a method in \ref BatchInterface is supported by implementation / derived class.
 */
struct supported_t {};

/**
 * \brief Value for sentinel type \ref supported_t
 * 
 */
constexpr static supported_t supported{};

/**
 * \brief Async functor for \ref BatchInterface::runJob
 * 
 * \param[out] jobName Job Id of scheduled job 
 * \return true if done
 */
typedef bool runJob_f(std::string& jobName);

/**
 * \brief Async functor for \ref BatchInterface::getNodes
 * 
 * \param insert Callback to get next Node
 */
typedef bool getNodes_f(const std::function<getNodes_inserter_f>& insert);

/**
 * \brief Async functor for \ref BatchInterface::getJobs
 * 
 * \param insert Callback to get next Job
 */
typedef bool getJobs_f(const std::function<getJobs_inserter_f>& insert);

/**
 * \brief Async functor for \ref BatchInterface::getQueues
 * 
 * \param insert Callback to get next Queue
 */
typedef bool getQueues_f(const std::function<getQueues_inserter_f>& insert);

/**
 * \brief Async functor for \ref BatchInterface::getBatchInfo
 * 
 * \param[out] info batchsystem info metadata 
 * \return true if done
 */
typedef bool getBatchInfo_f(BatchInfo& info);

/**
 * \brief Async functor for \ref BatchInterface::detect
 * 
 * \param[out] detected Set whether batch system is detected 
 * \return true if done
 */
typedef bool detect_f(bool& detected);


/**
 * \brief Struct of batchsystem interface functions
 * 
 * This is the public interface used by the provided pbs / slurm / lsf implementations,
 * that can be also used for custom implementations.
 * 
 * \par Implementation exceptions
 * Methods can throw exceptions to signal errors.
 * Provided implementations do and custom implementations should throw a \ref CommandFailed exception,
 * if they call a shell command that fails.
 * 
 * \par Optional methods exceptions
 * Optional methods of interface (not pure virtual methods) throw \ref NotImplemented if not overriden.
 * 
 * \par Optional methods supported check
 * Each optional method has a separate overload with only one \ref supported_t tag argument
 * to check wether method is supported to check without triggering action. These methods return false if not overriden.
 * 
 * \par Non-blocking support
 * Every async method returns a functor that stores its internal execution state / progress and returns a bool for non-blocking execution.
 * Until a method has not finished it returns false.
 * This way methods can be run blocking like this `autof = batch.method(); while(!f());` or non-blocking e.g. by polling the return value in an event loop.
 */
class BatchInterface {
public:
	virtual ~BatchInterface();
	
	/**
	 * \brief Check to detect batch interface.
	 */
	virtual std::function<detect_f> detect() = 0;

	/**
	 * \brief Get Node structs information from batchsystem
	 * 
	 * Query batchsystem for node informations.
	 * 
	 * \note if node in filterNodes is missing no execption is thrown, the batchsystem tries return info for the other nodes queried but some implementations will not get info for any node 
	 *
	 * \param filterNodes Query only selected nodes or all if empty
	 */
	virtual std::function<getNodes_f> getNodes(std::vector<std::string> filterNodes);
	virtual bool getNodes(supported_t);

	/**
	 * \brief Get Queue structs information from batchsystem
	 * 
	 * Query batchsystem for queue informations.
	 * 
	 * \param insert Callback to get next Queue
	 */
	virtual std::function<getQueues_f> getQueues();
	virtual bool getQueues(supported_t);

	/**
	 * \brief Get Job structs information from batchsystem
	 * 
	 * Query batchsystem for job informations.
	 * 
	 * \param filterJobs Filter out only certain jobs or all if empty
	 */
	virtual std::function<getJobs_f> getJobs(std::vector<std::string> filterJobs);
	virtual bool getJobs(supported_t);

	/**
	 * \brief Get batchsystem information
	 * 
	 * Get metadata for used batchsystem and version.
	 */
	virtual std::function<getBatchInfo_f> getBatchInfo();
	virtual bool getBatchInfo(supported_t);

	/**
	 * \brief Delete job by Id
	 * 
	 * \param job Id of job to delete
	 * \param force Force delete
	 */
	virtual std::function<bool()> deleteJobById(const std::string& job, bool force);
	virtual bool deleteJobById(supported_t);

	/**
	 * \brief Delete job(s) by user
	 * 
	 * \param user User of job(s) to delete
	 * \param force Force delete
	 */
	virtual std::function<bool()> deleteJobByUser(const std::string& user, bool force);
	virtual bool deleteJobByUser(supported_t);

	/**
	 * \brief Change Node state
	 * 
	 * \param name Name of node to change
	 * \param state Event to trigger to change node state
	 * \param force Wether to force node change
	 * \param reason Reason of node change
	 * \param appendReason Wether reason should be appended instead of overwritten
	 */
	virtual std::function<bool()> changeNodeState(const std::string& name, NodeChangeState state, bool force, const std::string& reason, bool appendReason);
	virtual bool changeNodeState(supported_t);

	/**
	 * \brief Set the QueueState
	 * 
	 * Change the queue state.
	 * 
	 * \param name Name of the queue to change state
	 * \param state State to change to
	 * \param force Wether to force state change
	 */
	virtual std::function<bool()> setQueueState(const std::string& name, QueueState state, bool force);
	virtual bool setQueueState(supported_t);

	/**
	 * \brief Schedule job to run on batchsystem
	 * 
	 * \param opts Options for job to run
	 */
	virtual std::function<runJob_f> runJob(const JobOptions& opts);
	virtual bool runJob(supported_t);

	/**
	 * \brief Set the node comment
	 * 
	 * \param name Node to set comment for
	 * \param comment Comment string to set
	 * \param force Force set comment
	 * \param appendComment Wether to append comment instead of overwritting
	 */
	virtual std::function<bool()> setNodeComment(const std::string& name, bool force, const std::string& comment, bool appendComment);
	virtual bool setNodeComment(supported_t);

	/**
	 * \brief Prevent a pending job from being run
	 * 
	 * Reverse of \ref releaseJob operation.
	 * 
	 * \param job Id of job to hold
	 * \param force Wether to force hold
	 */
	virtual std::function<bool()> holdJob(const std::string& job, bool force);
	virtual bool holdJob(supported_t);


	/**
	 * \brief Release a job that has been hold before to start execution
	 * 
	 * Reverse of \ref holdJob operation.
	 * 
	 * \param job Id of job to release
	 * \param force Wether to force release
	 */
	virtual std::function<bool()> releaseJob(const std::string& job, bool force);
	virtual bool releaseJob(supported_t);

	/**
	 * \brief Suspend a running job
	 * 
	 * Reverse of \ref resumeJob operation.
	 * 
	 * \param job Id of job to suspend
	 * \param force Wether to force suspend
	 */
	virtual std::function<bool()> suspendJob(const std::string& job, bool force);
	virtual bool suspendJob(supported_t);

	/**
	 * \brief Resume a job that was suspended before
	 * 
	 * Reverse of \ref suspendJob operation.
	 * 
	 * \param job Id of job to resume
	 * \param force Wether to force resume
	 */
	virtual std::function<bool()> resumeJob(const std::string& job, bool force);
	virtual bool resumeJob(supported_t);


	/**
	 * \brief Requeue a job into a waiting state
	 * 
	 * Running / supended / finished job is put back into a pending state.
	 * 
	 * \param job Id of job to requeue
	 * \param force Wether to force reque
	 */
	virtual std::function<bool()> rescheduleRunningJobInQueue(const std::string& job, bool force);
	virtual bool rescheduleRunningJobInQueue(supported_t);
};

}
}

#endif /* __CW_BATCHSYSTEM_BATCHSYSTEM__ */
