#include "batchsystem/batchsystem.h"
#include "batchsystem/factory.h"
#include "batchsystem/pbs.h"
#include "batchsystem/slurm.h"
#include "batchsystem/lsf.h"
#include "batchsystem/error.h"

#include <system_error>

namespace cw {
namespace batch {

std::unique_ptr<BatchInterface> create_batch(const System& system, cmd_f _func) {
    switch (system) {
        case System::Pbs: return std::unique_ptr<BatchInterface>{new pbs::Pbs(_func)};
        case System::Slurm: return std::unique_ptr<BatchInterface>{new slurm::Slurm(_func)};
        case System::Lsf: return std::unique_ptr<BatchInterface>{new lsf::Lsf(_func)};
        default: throw std::system_error(error_type::system_out_of_enum);
    } 
}

}
}