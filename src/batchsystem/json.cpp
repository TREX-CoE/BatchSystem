#include "batchsystem/json.h"

#include <algorithm>
#include <iostream>
#include <cctype>
#include <string>
#include <cstring>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using rapidjson::Value;
using rapidjson::Document;

namespace {

using namespace cw::batch;

std::string jsonToString(const rapidjson::Document& document) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    return buffer.GetString();
}

template <typename T, typename U, typename D, typename A>
void addMemberOptional(D&& document, T&& name, U&& val, A&& allocator) {
        if (std::forward<U>(val).has_value()) std::forward<D>(document).AddMember(std::forward<T>(name), std::forward<U>(val).get(), std::forward<A>(allocator));
}
template <typename T, typename U, typename D, typename A>
void addMember(D&& document, T&& name, U&& val, A&& allocator) {
        std::forward<D>(document).AddMember(std::forward<T>(name), std::forward<U>(val), std::forward<A>(allocator));
}

bool getForce(const rapidjson::Document& document) {
        return document.HasMember("force") && document["force"].IsBool() && document["force"].GetBool();
}

std::string getJob(const rapidjson::Document& document) {
        if (!document.HasMember("job")) throw std::runtime_error("No job given");
        auto& job = document["job"];
        if (!job.IsString()) throw std::runtime_error("Job is not a string");
        return std::string(job.GetString());
}

std::string getNode(const rapidjson::Document& document) {
        if (!document.HasMember("node")) throw std::runtime_error("No node given");
        auto& node = document["node"];
        if (!node.IsString()) throw std::runtime_error("Node is not a string");
        return std::string(node.GetString());
}

std::string getQueue(const rapidjson::Document& document) {
        if (!document.HasMember("queue")) throw std::runtime_error("No queue given");
        auto& queue = document["queue"];
        if (!queue.IsString()) throw std::runtime_error("Queue is not a string");
        return std::string(queue.GetString());
}

std::string getState(const rapidjson::Document& document) {
        if (!document.HasMember("state")) throw std::runtime_error("No state given");
        auto& queue = document["state"];
        if (!queue.IsString()) throw std::runtime_error("State is not a string");
        return std::string(queue.GetString());
}

QueueState convertQueue(const std::string& str) {
        if (str == "open") {
                return QueueState::Open;
        } else if (str == "closed") {
                return QueueState::Closed;
        } else if (str == "inactive") {
                return QueueState::Inactive;
        } else if (str == "draining") {
                return QueueState::Draining;
        } else {
                throw std::runtime_error("Invalid queue state");
        }
}


NodeChangeState convertNodeChange(const std::string& str) {
        if (str == "resume") {
                return NodeChangeState::Resume;
        } else if (str == "drain") {
                return NodeChangeState::Drain;
        } else if (str == "undrain") {
                return NodeChangeState::Undrain;
        } else {
                throw std::runtime_error("Invalid node state");
        }
}


enum class QueueState {
	Unknown, //!< Unknown / unhandled or invalid queue state
	Open, //!< Open queue, jobs can be scheduled and run
	Closed, //!< Closed queue, jobs can neither be scheduled nor run
	Inactive, //!< Queue is inactive, jobs can be scheduled but will not run (yet)
	Draining, //!< Queue currently draining, jobs cannot be scheduled but existing still run
};

}

namespace cw {
namespace batch {
namespace json {

void serialize(const Job& job, rapidjson::Document& document) {
        Document::AllocatorType& allocator = document.GetAllocator();
        document.SetObject();
        addMember(document, "id", job.id.get(), allocator);
        addMemberOptional(document, "idCompact", job.idCompact, allocator);
        addMember(document, "name", job.name.get(), allocator);
        addMemberOptional(document, "owner", job.owner, allocator);
        addMemberOptional(document, "user", job.user, allocator);
        addMemberOptional(document, "userId", job.userId, allocator);
        addMemberOptional(document, "group", job.group, allocator);
        addMemberOptional(document, "groupId", job.groupId, allocator);
        addMember(document, "queue", job.queue.get(), allocator);
        addMemberOptional(document, "priority", job.priority, allocator);
        addMemberOptional(document, "execHost", job.execHost, allocator);
        addMemberOptional(document, "submitHost", job.submitHost, allocator);
        addMemberOptional(document, "server", job.server, allocator);

        {
                Value state(to_cstr(job.state), allocator);
                addMember(document, "state", state, allocator);
        }

        addMemberOptional(document, "rawState", job.rawState, allocator);
        addMemberOptional(document, "workdir", job.workdir, allocator);
        addMemberOptional(document, "exitCode", job.exitCode, allocator);
        addMemberOptional(document, "exitSignal", job.exitSignal, allocator);
        addMemberOptional(document, "submitArgs", job.submitArgs, allocator);

        {
                Value comment;
                comment.SetObject();
                addMemberOptional(comment, "reason", job.reason, allocator);
                addMemberOptional(comment, "user", job.comment, allocator);
                addMemberOptional(comment, "admin", job.commentAdmin, allocator);
                addMemberOptional(comment, "system", job.commentSystem, allocator);
                addMember(document, "comment", comment, allocator);
        }

        {
                Value time;
                time.SetObject();
                addMemberOptional(time, "submit", job.submitTime, allocator);
                addMemberOptional(time, "start", job.startTime, allocator);
                addMemberOptional(time, "end", job.endTime, allocator);
                addMemberOptional(time, "queue", job.queueTime, allocator);
                addMember(document, "time", time, allocator);
        }

        {
                Value used;
                used.SetObject();
                addMemberOptional(used, "cpusPerNode", job.cpusPerNode, allocator);
                addMemberOptional(used, "cpus", job.cpus, allocator);
                addMemberOptional(used, "nodes", job.nodes, allocator);
                addMemberOptional(used, "wallTime", job.wallTimeUsed, allocator);
                addMemberOptional(used, "cpuTime", job.cpuTimeUsed, allocator);
                addMemberOptional(used, "mem", job.memUsed, allocator);
                addMemberOptional(used, "vmem", job.vmemUsed, allocator);
                addMember(document, "used", used, allocator);
        }

        {
                Value requested;
                requested.SetObject();
                addMemberOptional(requested, "other", job.otherRequested, allocator);
                addMemberOptional(requested, "nice", job.niceRequested, allocator);
                addMemberOptional(requested, "cpus", job.cpusRequested, allocator);
                addMemberOptional(requested, "nodes", job.nodesRequested, allocator);
                addMemberOptional(requested, "wallTime", job.wallTimeRequested, allocator);

                Value requestedGeneral;
                requestedGeneral.SetObject();
                for (const auto& p : job.requestedGeneral) {
                        Value key(p.first.c_str(), allocator);
                        Value val(p.second.c_str(), allocator);
                        requestedGeneral.AddMember(key, val, allocator);
                }

                addMember(requested, "general", requestedGeneral, allocator);
                addMember(document, "requested", requested, allocator);
        }

        {
                Value cpusFromNode;
                cpusFromNode.SetObject();
                for (const auto& p : job.cpusFromNode) {
                        Value key(p.first.c_str(), allocator);
                        cpusFromNode.AddMember(key, p.second, allocator);
                }
                addMember(document, "cpusFromNode", cpusFromNode, allocator);
        }

        {
                Value variableList;
                variableList.SetObject();
                for (const auto& p : job.variableList) {
                        Value key(p.first.c_str(), allocator);
                        Value val(p.second.c_str(), allocator);
                        variableList.AddMember(key, val, allocator);
                }
                addMember(document, "variableList", variableList, allocator);
        }

}
std::string serialize(const Job& job) {
        Document d;
        serialize(job, d);
        return jsonToString(d);
}

void serialize(const Node& node, rapidjson::Document& document) {
        Document::AllocatorType& allocator = document.GetAllocator();
        document.SetObject();
        addMember(document, "name", node.name.get(), allocator);
        addMemberOptional(document, "nameFull", node.nameFull, allocator);
        
        {
                Value state(to_cstr(node.state), allocator);
                addMember(document, "state", state, allocator);
        }

        addMember(document, "rawState", node.rawState.get(), allocator);
        addMemberOptional(document, "comment", node.comment, allocator);
        addMemberOptional(document, "reason", node.reason, allocator);
        addMemberOptional(document, "cpus", node.cpus, allocator);
        addMemberOptional(document, "cpusReserved", node.cpusReserved, allocator);
        addMemberOptional(document, "cpusUsed", node.cpusUsed, allocator);

        {
                Value cpusOfJobs;
                cpusOfJobs.SetObject();
                for (const auto& p : node.cpusOfJobs) {
                        Value key(p.first.c_str(), allocator);
                        cpusOfJobs.AddMember(key, p.second, allocator);
                }
                addMember(document, "cpusOfJobs", cpusOfJobs, allocator);
        }
}
std::string serialize(const Node& node) {
        Document d;
        serialize(node, d);
        return jsonToString(d);
}

void serialize(const Queue& queue, rapidjson::Document& document) {
        Document::AllocatorType& allocator = document.GetAllocator();
        document.SetObject();
        addMember(document, "name", queue.name.get(), allocator);
        addMember(document, "priority", queue.priority.get(), allocator);
        Value state(to_cstr(queue.state), allocator);
        addMember(document, "state", state, allocator);
        addMember(document, "rawState", queue.rawState.get(), allocator);

        {
                Value jobs;
                jobs.SetObject();
                addMemberOptional(jobs, "total", queue.jobsTotal, allocator);
                addMemberOptional(jobs, "pending", queue.jobsPending, allocator);
                addMemberOptional(jobs, "running", queue.jobsRunning, allocator);
                addMemberOptional(jobs, "suspended", queue.jobsSuspended, allocator);
                addMember(document, "jobs", jobs, allocator);
        }

        addMemberOptional(document, "cpus", queue.cpus, allocator);
        addMemberOptional(document, "nodesTotal", queue.nodesTotal, allocator);
        addMemberOptional(document, "nodesMin", queue.nodesMin, allocator);
        addMemberOptional(document, "nodesMax", queue.nodesMax, allocator);
        addMemberOptional(document, "wallTimeMax", queue.wallTimeMax, allocator);
        addMemberOptional(document, "memPerCpuMax", queue.memPerCpuMax, allocator);
        addMemberOptional(document, "modifyTime", queue.modifyTime, allocator);

        {
                Value nodes;
                nodes.SetArray();
                for (const auto& node : queue.nodes) {
                        Value val(node.c_str(), allocator);
                        nodes.PushBack(val, allocator);
                }
                addMember(document, "nodes", nodes, allocator);
        }
}
std::string serialize(const Queue& queue) {
        Document d;
        serialize(queue, d);
        return jsonToString(d);
}

void serialize(const BatchInfo& batchInfo, rapidjson::Document& document) {
        Document::AllocatorType& allocator = document.GetAllocator();
        document.SetObject();
        addMemberOptional(document, "name", batchInfo.name, allocator);
        addMemberOptional(document, "version", batchInfo.version, allocator);
        {
                Value info;
                info.SetObject();
                for (const auto& p : batchInfo.info) {
                        Value key(p.first.c_str(), allocator);
                        Value val(p.second.c_str(), allocator);
                        info.AddMember(key, val, allocator);
                }
                addMember(document, "info", info, allocator);
        }
}

std::string serialize(const BatchInfo& batchInfo) {
        Document d;
        serialize(batchInfo, d);
        return jsonToString(d);
}

std::function<bool(BatchInterface&)> command(rapidjson::Document& document, BatchInterface& batch) {
        if (!document.IsObject()) throw std::runtime_error("Is not an object");
        if (!document.HasMember("command")) throw std::runtime_error("No command selected");
        auto& command = document["command"];
        if (!command.IsString()) throw std::runtime_error("Command is not a string");
        auto cmd = command.GetString();
        if (strcmp(cmd,"deleteJobById")==0) {
                if (!batch.deleteJobById(supported)) goto unsupported;
                return [force=getForce(document), jobName=getJob(document)](BatchInterface& inf){ return inf.deleteJobById(jobName, force); };
        } else if (strcmp(cmd,"deleteJobByUser")==0) {
                if (!batch.deleteJobByUser(supported)) goto unsupported;
                if (!document.HasMember("user")) throw std::runtime_error("No user given");
                auto& user = document["user"];
                if (!user.IsString()) throw std::runtime_error("User is not a string");
                return [force=getForce(document), userName=std::string(user.GetString())](BatchInterface& inf){ return inf.deleteJobByUser(userName, force); };

        } else if (strcmp(cmd,"changeNodeState")==0) {
                if (!batch.changeNodeState(supported)) goto unsupported;
                std::string reasonText;
                if (document.HasMember("reason")) {
                        auto& reason = document["reason"];
                        if (!reason.IsString()) throw std::runtime_error("Reason is not a string");
                        reasonText = reason.GetString();
                }
                bool appendReason = document.HasMember("append") && document["append"].IsBool() && document["append"].GetBool();
                return [appendReason, nodeName=getNode(document), force=getForce(document), nodeState=convertNodeChange(getState(document)), reasonText](BatchInterface& inf){ return inf.changeNodeState(nodeName, nodeState, force, reasonText, appendReason); };
        } else if (strcmp(cmd,"setQueueState")==0) {
                if (!batch.setQueueState(supported)) goto unsupported;
                return [queueName=getQueue(document), force=getForce(document), queueState=convertQueue(getState(document))](BatchInterface& inf){ return inf.setQueueState(queueName, queueState, force); };
        } else if (strcmp(cmd,"runJob")==0) {
                if (!batch.runJob(supported)) goto unsupported;
                JobOptions opts;
                if (document.HasMember("nodes")) {
                        auto& nodes = document["nodes"];
                        if (!nodes.IsInt()) throw std::runtime_error("Nodes is not an int");
                        int nnodes = nodes.GetInt();
                        if (nnodes < 1) throw std::runtime_error("Nodes has to be atleast 1");
                        opts.numberNodes = static_cast<uint32_t>(nnodes);
                }
                // TODO add rest
                // handle jobname out

                return [opts](BatchInterface& inf){ std::string jobName; return inf.runJob(opts, jobName); };
        } else if (strcmp(cmd,"setNodeComment")==0) {
                if (!batch.setNodeComment(supported)) goto unsupported;

                if (!document.HasMember("comment")) throw std::runtime_error("No comment given");
                auto& comment = document["comment"];
                if (!comment.IsString()) throw std::runtime_error("Comment is not a string");

                bool appendComment = document.HasMember("append") && document["append"].IsBool() && document["append"].GetBool();
                return [force=getForce(document), nodeName=getNode(document), appendComment, commentText=std::string(comment.GetString())](BatchInterface& inf){ return inf.setNodeComment(nodeName, force, commentText, appendComment); };
        } else if (strcmp(cmd,"holdJob")==0) {
                if (!batch.holdJob(supported)) goto unsupported;
                return [force=getForce(document), jobName=getJob(document)](BatchInterface& inf){ return inf.holdJob(jobName, force); };
        } else if (strcmp(cmd,"releaseJob")==0) {
                if (!batch.releaseJob(supported)) goto unsupported;
                return [force=getForce(document), jobName=getJob(document)](BatchInterface& inf){ return inf.releaseJob(jobName, force); };
        } else if (strcmp(cmd,"suspendJob")==0) {
                if (!batch.suspendJob(supported)) goto unsupported;
                return [force=getForce(document), jobName=getJob(document)](BatchInterface& inf){ return inf.suspendJob(jobName, force); };
        } else if (strcmp(cmd,"resumeJob")==0) {
                if (!batch.resumeJob(supported)) goto unsupported;
                return [force=getForce(document), jobName=getJob(document)](BatchInterface& inf){ return inf.resumeJob(jobName, force); };
        } else if (strcmp(cmd,"rescheduleRunningJobInQueue")==0) {
                if (!batch.rescheduleRunningJobInQueue(supported)) goto unsupported;
                return [force=getForce(document), jobName=getJob(document)](BatchInterface& inf){ return inf.rescheduleRunningJobInQueue(jobName, force); };
        } else {
                throw std::runtime_error("Unknown command string");
        }

        unsupported:

        throw std::runtime_error("Unsupported command by batch interface");
}

}
}
}