#ifndef __CW_BATCHSYSTEM_FACTORY__
#define __CW_BATCHSYSTEM_FACTORY__

#include "batchsystem/batchsystem.h"

#include <array>

namespace cw {
namespace batch {

/**
 * \brief Implemented batchsystems
 * 
 */
enum class System {
	Slurm, //!< slurm
	Pbs, //!< pbs / pbspro / torque
	Lsf, //!< ibm lsf
};

/**
 * \brief All implemented batchsystems
 * 
 * All values of \ref System enum.
 * 
 */
const std::array<System,3> Systems = {System::Slurm, System::Pbs, System::Lsf};


bool detect(const System& system, std::function<cmd_execute_f> _func);
void create_batch(BatchSystem& inf, const System& system, std::function<cmd_execute_f> _func);

}
}

#endif /* __CW_BATCHSYSTEM_FACTORY__ */
