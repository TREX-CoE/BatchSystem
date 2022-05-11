#ifndef __CW_BATCHSYSTEM_SLURM_H__
#define __CW_BATCHSYSTEM_SLURM_H__

#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {

/**
 * \brief Batchsystem interface implementation for Slurm
 * 
 */
namespace slurm {

/**
 * \brief Detect wether batchsystem exists
 * 
 * \param _func Function to call shell commands
 * \return Wether batchsystem has been found
 */
bool detect(std::function<cmd_execute_f> _func);

/**
 * \brief Initialize batchsystem interface with Slurm implementation (autodetect sacct)
 * 
 * This variant calls sacct to check wether it is available and queryable (slurmdbd running) and use that instead of scontrol for job infos.
 * 
 * \param[out] inf Interface object to set
 * \param _func Function to call shell commands
 */
void create_batch(BatchSystem& inf, std::function<cmd_execute_f> _func);

/**
 * \brief Initialize batchsystem interface with Slurm implementation
 * 
 * This variant manually sets wether to use sacct.
 * 
 * \param[out] inf Interface object to set
 * \param _func Function to call shell commands
 * \param useSacct Wether to use sacct or scontrol for job infos
 */
void create_batch(BatchSystem& inf, std::function<cmd_execute_f> _func, bool useSacct);
}
}
}

#endif /* __CW_BATCHSYSTEM_SLURM_H__ */
