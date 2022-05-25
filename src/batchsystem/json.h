#ifndef __CW_BATCHSYSTEM_JSON_H__
#define __CW_BATCHSYSTEM_JSON_H__

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "batchsystem/batchsystem.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>

namespace cw {
namespace batch {

/**
 * \brief Json serialization functionality
 * 
 * Functions to serialize data structures of batchsystem library.
 * 
 */
namespace json {

/**
 * \brief Serialize batchsystem job info to json object
 * 
 * \param job job info to serialize
 * \param[out] document Json object to serialize into
 */
void serialize(const Job& job, rapidjson::Document& document);

/**
 * \brief Serialize batchsystem job info to json string
 * 
 * \param job job info to serialize
 * \return Json string representation
 */
std::string serialize(const Job& job);

/**
 * \brief Serialize batchsystem node info to json object
 * 
 * \param node node info to serialize
 * \param[out] document Json object to serialize into
 */
void serialize(const Node& node, rapidjson::Document& document);

/**
 * \brief Serialize batchsystem node info to json string
 * 
 * \param node node info to serialize
 * \return Json string representation
 */
std::string serialize(const Node& node);

/**
 * \brief Serialize batchsystem queue info to json object
 * 
 * \param queue queue info to serialize
 * \param[out] document Json object to serialize into
 */
void serialize(const Queue& queue, rapidjson::Document& document);

/**
 * \brief Serialize batchsystem queue info to json string
 * 
 * \param queue queue info to serialize
 * \return Json string representation
 */
std::string serialize(const Queue& queue);

/**
 * \brief Serialize batchsystem metadata to json object
 * 
 * \param batchInfo batchsystem metadata to serialize
 * \param[out] document Json object to serialize into
 */
void serialize(const BatchInfo& batchInfo, rapidjson::Document& document);

/**
 * \brief Serialize batchsystem metadata to json string
 * 
 * \param batchInfo batchsystem metadata to serialize
 * \return Json string representation
 */
std::string serialize(const BatchInfo& batchInfo);

std::function<bool(BatchInterface&)> command(rapidjson::Document& document, BatchInterface& batch);

}
}
}

#endif /* __CW_BATCHSYSTEM_JSON_H__ */
