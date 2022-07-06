#include "batchsystem/error.h"

#include <string>
#include <exception>
#include <system_error>
#include <cassert>

namespace {

using namespace cw::batch;

const char* to_cstr(batch_error type) {
  switch (type) {
      case batch_error::pbsnodes_x_xml_parse_error: return "error while parsing pbsnodes -x xml output";
      case batch_error::qselect_u_failed: return "qselect -u failed";
      case batch_error::qdel_failed: return "qdel failed";
      case batch_error::sacct_X_P_format_ALL_failed: return "sacct -X -P --format ALL failed";
      case batch_error::scontrol_show_job_all_failed: return "scontrol show job --all failed";
      case batch_error::scontrol_version_failed: return "scontrol --version failed";
      case batch_error::scontrol_show_config_failed: return "scontrol show config failed";
      case batch_error::bkill_u_failed: return "bkill -u failed";
      case batch_error::node_change_state_undrain_unsupported_by_lsf: return "node state change to undrain unsupported by lsf";
      case batch_error::node_change_state_out_of_enum: return "node state change invalid enum value";
      case batch_error::bresume_failed: return "bresume failed";
      case batch_error::bstop_failed: return "bstop failed";
      case batch_error::queue_state_unknown_not_supported: return "queue state unknown not supported";
      case batch_error::queue_state_inactive_unsupported_by_lsf: return "queue state change to inactive unsupported by lsf";
      case batch_error::queue_state_draining_unsupported_by_lsf: return "queue state change to draining unsupported by lsf";
      case batch_error::queue_state_out_of_enum: return "queue state invalid enum value";
      case batch_error::badmin_failed: return "badmin failed";
      case batch_error::bhosts_failed: return "bhosts failed";
      case batch_error::bqueues_failed: return "bqueues failed";
      case batch_error::bsub_failed: return "bsub failed";
      case batch_error::bjob_u_all_failed: return "bjob -u all failed";
      case batch_error::lsid_failed: return "lsid failed";
      case batch_error::qstat_f_x_xml_parse_error: return "error while parsing qstat -f -x xml output";
      case batch_error::pbsnodes_change_node_state_failed: return "pbsnodes node state change failed";
      case batch_error::pbsnodes_set_comment_failed: return "pbsnodes set comment failed";
      case batch_error::qrls_failed: return "qrls failed";
      case batch_error::qhold_failed: return "qhold failed";
      case batch_error::qsig_s_suspend_failed: return "qsig -s suspend failed";
      case batch_error::qsig_s_resume_failed: return "qsig -s resume failed";
      case batch_error::qmgr_c_set_queue_failed: return "qmgr -c set queue failed";
      case batch_error::qrerun_failed: return "qrerun failed";
      case batch_error::pbsnodes_x_failed: return "pbsnodes -x failed";
      case batch_error::qstat_Qf_failed: return "qstat -Qf failed";
      case batch_error::qsub_failed: return "qsub failed";
      case batch_error::qstat_f_x_failed: return "qstat -f -x failed";
      case batch_error::pbsnodes_version_failed: return "pbsnodes --version failed";
      case batch_error::scontrol_update_NodeName_State_failed: return "scontrol update NodeName State failed";
      case batch_error::scontrol_update_NodeName_Comment_failed: return "scontrol update NodeName Comment failed";
      case batch_error::scontrol_release_failed: return "scontrol release failed";
      case batch_error::scontrol_hold_failed: return "scontrol hold failed";
      case batch_error::scancel_failed: return "scancel failed";
      case batch_error::scancel_u_failed: return "scancel -u failed";
      case batch_error::scontrol_suspend_failed: return "scontrol suspend failed";
      case batch_error::scontrol_resume_failed: return "scontrol resume failed";
      case batch_error::scontrol_update_PartitionName_State_failed: return "scontrol update PartitionName State failed";
      case batch_error::scontrol_requeuehold_failed: return "scontrol requeuehold failed";
      case batch_error::scontrol_requeue_failed: return "scontrol requeue failed";
      case batch_error::scontrol_show_node_failed: return "scontrol show node failed";
      case batch_error::scontrol_show_partition_all_failed: return "scontrol show partition --all failed";
      case batch_error::sbatch_failed: return "sbatch failed";
      case batch_error::slurm_job_mode_out_of_enum: return "slurm job mode invalid enum value";
      case batch_error::system_out_of_enum: return "batchsystem system invalid enum value";
      default: return "(unrecognized error)";
  }
}

const char* to_cstr(batch_condition type) {
  switch (type) {
      case batch_condition::command_failed: return "failed batchsystem command";
      case batch_condition::invalid_argument: return "invalid argument given";
      default: return "(unrecognized error)";
  }
}



struct BatchsystemErrCategory : std::error_category
{

  const char* name() const noexcept {
    return "batchsystem";
  }
 
  std::string message(int ev) const {
    return to_cstr(static_cast<batch_error>(ev));
  }

  std::error_condition default_error_condition(int err_value) const noexcept {
      switch (static_cast<batch_error>(err_value)) {
        case batch_error::pbsnodes_x_xml_parse_error: return batch_condition::command_failed;
        case batch_error::qselect_u_failed: return batch_condition::command_failed;
        case batch_error::qdel_failed: return batch_condition::command_failed;
        case batch_error::sacct_X_P_format_ALL_failed: return batch_condition::command_failed;
        case batch_error::scontrol_show_job_all_failed: return batch_condition::command_failed;
        case batch_error::scontrol_version_failed: return batch_condition::command_failed;
        case batch_error::scontrol_show_config_failed: return batch_condition::command_failed;
        case batch_error::bkill_u_failed: return batch_condition::command_failed;
        case batch_error::node_change_state_undrain_unsupported_by_lsf: return batch_condition::command_failed;
        case batch_error::node_change_state_out_of_enum: return batch_condition::command_failed;
        case batch_error::bresume_failed: return batch_condition::command_failed;
        case batch_error::bstop_failed: return batch_condition::command_failed;
        case batch_error::queue_state_unknown_not_supported: return batch_condition::command_failed;
        case batch_error::queue_state_inactive_unsupported_by_lsf: return batch_condition::command_failed;
        case batch_error::queue_state_draining_unsupported_by_lsf: return batch_condition::command_failed;
        case batch_error::queue_state_out_of_enum: return batch_condition::command_failed;
        case batch_error::badmin_failed: return batch_condition::command_failed;
        case batch_error::bhosts_failed: return batch_condition::command_failed;
        case batch_error::bqueues_failed: return batch_condition::command_failed;
        case batch_error::bsub_failed: return batch_condition::command_failed;
        case batch_error::bjob_u_all_failed: return batch_condition::command_failed;
        case batch_error::lsid_failed: return batch_condition::command_failed;
        case batch_error::qstat_f_x_xml_parse_error: return batch_condition::command_failed;
        case batch_error::pbsnodes_change_node_state_failed: return batch_condition::command_failed;
        case batch_error::pbsnodes_set_comment_failed: return batch_condition::command_failed;
        case batch_error::qrls_failed: return batch_condition::command_failed;
        case batch_error::qhold_failed: return batch_condition::command_failed;
        case batch_error::qsig_s_suspend_failed: return batch_condition::command_failed;
        case batch_error::qsig_s_resume_failed: return batch_condition::command_failed;
        case batch_error::qmgr_c_set_queue_failed: return batch_condition::command_failed;
        case batch_error::qrerun_failed: return batch_condition::command_failed;
        case batch_error::pbsnodes_x_failed: return batch_condition::command_failed;
        case batch_error::qstat_Qf_failed: return batch_condition::command_failed;
        case batch_error::qsub_failed: return batch_condition::command_failed;
        case batch_error::qstat_f_x_failed: return batch_condition::command_failed;
        case batch_error::pbsnodes_version_failed: return batch_condition::command_failed;
        case batch_error::scontrol_update_NodeName_State_failed: return batch_condition::command_failed;
        case batch_error::scontrol_update_NodeName_Comment_failed: return batch_condition::command_failed;
        case batch_error::scontrol_release_failed: return batch_condition::command_failed;
        case batch_error::scontrol_hold_failed: return batch_condition::command_failed;
        case batch_error::scancel_failed: return batch_condition::command_failed;
        case batch_error::scancel_u_failed: return batch_condition::command_failed;
        case batch_error::scontrol_suspend_failed: return batch_condition::command_failed;
        case batch_error::scontrol_resume_failed: return batch_condition::command_failed;
        case batch_error::scontrol_update_PartitionName_State_failed: return batch_condition::command_failed;
        case batch_error::scontrol_requeuehold_failed: return batch_condition::command_failed;
        case batch_error::scontrol_requeue_failed: return batch_condition::command_failed;
        case batch_error::scontrol_show_node_failed: return batch_condition::command_failed;
        case batch_error::scontrol_show_partition_all_failed: return batch_condition::command_failed;
        case batch_error::sbatch_failed: return batch_condition::command_failed;
        case batch_error::slurm_job_mode_out_of_enum: return batch_condition::command_failed;
        case batch_error::system_out_of_enum: return batch_condition::command_failed;
        default: assert(false && "unknown batch_error");
      }
  }
};

struct BatchsystemCondCategory : std::error_category
{

  const char* name() const noexcept {
    return "batchsystem_generic";
  }
 
  std::string message(int ev) const {
    return to_cstr(static_cast<batch_condition>(ev));
  }
};

 
const BatchsystemErrCategory error_cat {};
const BatchsystemCondCategory cond_cat {};

}

namespace cw {
namespace batch {

const std::error_category& batchsystem_generic() noexcept {
    return cond_cat;
}

const std::error_category& batchsystem_category() noexcept {
    return error_cat;
}

std::error_code make_error_code(batch_error e) {
  return {static_cast<int>(e), error_cat};
}

std::error_condition make_error_condition(batch_condition e) {
  return {static_cast<int>(e), cond_cat};
}

}
}
