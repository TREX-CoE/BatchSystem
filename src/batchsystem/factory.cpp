#include "batchsystem/batchsystem.h"
#include "batchsystem/factory.h"
#include "batchsystem/pbs.h"
#include "batchsystem/slurm.h"
#include "batchsystem/lsf.h"

namespace cw {
namespace batch {

std::unique_ptr<BatchInterface> create_batch(const System& system, std::function<cmd_execute_f> _func) {
    switch (system) {
        case System::Pbs: return std::unique_ptr<BatchInterface>{new pbs::Pbs(_func)};
        case System::Slurm: return std::unique_ptr<BatchInterface>{new slurm::Slurm(_func)};
        case System::Lsf: return std::unique_ptr<BatchInterface>{new lsf::Lsf(_func)};
        default:
            throw std::runtime_error("invalid system enum");
    } 
}

}
}