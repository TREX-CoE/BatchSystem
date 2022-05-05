#ifndef __CW_BATCHSYSTEM_LSF_H__
#define __CW_BATCHSYSTEM_LSF_H__

#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {

/**
 * \brief Batchsystem interface implementation for LSF
 * 
 */
namespace lsf {

/**
 * \brief Initialize batchsystem interface with LSF implementation
 * 
 * \param[out] inf Interface object to set
 * \param _func Function to call shell commands
 */
void create_batch(BatchSystem& inf, std::function<cmd_execute_f> _func);
}
}
}

#endif /* __CW_BATCHSYSTEM_LSF_H__ */
