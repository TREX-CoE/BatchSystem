#include "batchsystem/batchsystem.h"
#include "batchsystem/factory.h"
#include "batchsystem/pbs.h"
#include "batchsystem/slurm.h"
#include "batchsystem/lsf.h"

namespace cw {
namespace batch {



bool detect(const System& system, std::function<cmd_execute_f> _func) {
    switch (system) {
        case System::Pbs:
            return pbs::detect(_func);
        case System::Slurm:
            return slurm::detect(_func);
        case System::Lsf:
            return lsf::detect(_func);
        default:
            throw std::runtime_error("invalid system enum");
    } 
}

void create_batch(BatchSystem& inf, const System& system, std::function<cmd_execute_f> _func) {
    switch (system) {
        case System::Pbs:
            pbs::create_batch(inf, _func);
            break;
        case System::Slurm:
            slurm::create_batch(inf, _func);
            break;
        case System::Lsf:
            lsf::create_batch(inf, _func);
            break;
        default:
            throw std::runtime_error("invalid system enum");
    } 
}

}
}