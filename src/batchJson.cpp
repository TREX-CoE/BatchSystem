#include "batchJson.h"

#include <algorithm>
#include <cctype>
#include <string>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using rapidjson::Value;
using rapidjson::Document;

namespace {

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

}
}
}