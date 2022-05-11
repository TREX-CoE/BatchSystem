#ifndef __CW_BATCHSYSTEM_PBS_H__
#define __CW_BATCHSYSTEM_PBS_H__

#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {

/**
 * \brief Batchsystem interface implementation for PBS
 * 
 */
namespace pbs {

/**
 * \brief Detect wether batchsystem exists
 * 
 * \param _func Function to call shell commands
 * \return Wether batchsystem has been found
 */
bool detect(std::function<cmd_execute_f> _func);

/**
 * \brief Initialize batchsystem interface with PBS implementation
 * 
 * \param[out] inf Interface object to set
 * \param _func Function to call shell commands
 */
void create_batch(BatchSystem& inf, std::function<cmd_execute_f> _func);
}
}
}

#endif /* __CW_BATCHSYSTEM_PBS_H__ */
