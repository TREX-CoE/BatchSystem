#include "batchsystem/error.h"

#include <string>
#include <exception>
#include <system_error>

namespace {

using namespace cw::batch;

const char* to_cstr(error_type type) {
  (void)type;
  return "";

  switch (type) {
      case error_type::pbsnodes_x_xml_parse_error: return "error while parsing pbsnodes -x xml output";
      case error_type::qselect_u_failed: return "qselect -u failed";
      case error_type::qdel_failed: return "qdel failed";
      case error_type::sacct_X_P_format_ALL_failed: return "sacct -X -P --format ALL failed";
      case error_type::scontrol_show_job_all_failed: return "scontrol show job --all failed";
      case error_type::scontrol_version_failed: return "scontrol --version failed";
      case error_type::scontrol_show_config_failed: return "scontrol show config failed";
      case error_type::bkill_u_failed: return "bkill -u failed";
      case error_type::node_change_state_undrain_unsupported_by_lsf: return "node state change to undrain unsupported by lsf";
      case error_type::node_change_state_out_of_enum: return "node state change invalid enum value";
      case error_type::bresume_failed: return "bresume failed";
      case error_type::bstop_failed: return "bstop failed";
      case error_type::queue_state_unknown_not_supported: return "queue state unknown not supported";
      case error_type::queue_state_inactive_unsupported_by_lsf: return "queue state change to inactive unsupported by lsf";
      case error_type::queue_state_draining_unsupported_by_lsf: return "queue state change to draining unsupported by lsf";
      case error_type::queue_state_out_of_enum: return "queue state invalid enum value";
      case error_type::badmin_failed: return "badmin failed";
      case error_type::bhosts_failed: return "bhosts failed";
      case error_type::bqueues_failed: return "bqueues failed";
      case error_type::bsub_failed: return "bsub failed";
      case error_type::bjob_u_all_failed: return "bjob -u all failed";
      case error_type::lsid_failed: return "lsid failed";
      case error_type::qstat_f_x_xml_parse_error: return "error while parsing qstat -f -x xml output";
      case error_type::pbsnodes_change_node_state_failed: return "pbsnodes node state change failed";
      case error_type::pbsnodes_set_comment_failed: return "pbsnodes set comment failed";
      case error_type::qrls_failed: return "qrls failed";
      case error_type::qhold_failed: return "qhold failed";
      case error_type::qsig_s_suspend_failed: return "qsig -s suspend failed";
      case error_type::qsig_s_resume_failed: return "qsig -s resume failed";
      case error_type::qmgr_c_set_queue_failed: return "qmgr -c set queue failed";
      case error_type::qrerun_failed: return "qrerun failed";
      case error_type::pbsnodes_x_failed: return "pbsnodes -x failed";
      case error_type::qstat_Qf_failed: return "qstat -Qf failed";
      case error_type::qsub_failed: return "qsub failed";
      case error_type::qstat_f_x_failed: return "qstat -f -x failed";
      case error_type::pbsnodes_version_failed: return "pbsnodes --version failed";
      case error_type::scontrol_update_NodeName_State_failed: return "scontrol update NodeName State failed";
      case error_type::scontrol_update_NodeName_Comment_failed: return "scontrol update NodeName Comment failed";
      case error_type::scontrol_release_failed: return "scontrol release failed";
      case error_type::scontrol_hold_failed: return "scontrol hold failed";
      case error_type::scancel_failed: return "scancel failed";
      case error_type::scancel_u_failed: return "scancel -u failed";
      case error_type::scontrol_suspend_failed: return "scontrol suspend failed";
      case error_type::scontrol_resume_failed: return "scontrol resume failed";
      case error_type::scontrol_update_PartitionName_State_failed: return "scontrol update PartitionName State failed";
      case error_type::scontrol_requeuehold_failed: return "scontrol requeuehold failed";
      case error_type::scontrol_requeue_failed: return "scontrol requeue failed";
      case error_type::scontrol_show_node_failed: return "scontrol show node failed";
      case error_type::scontrol_show_partition_all_failed: return "scontrol show partition --all failed";
      case error_type::sbatch_failed: return "sbatch failed";
      case error_type::slurm_job_mode_out_of_enum: return "slurm job mode invalid enum value";
      case error_type::system_out_of_enum: return "batchsystem system invalid enum value";
      default: return "(unrecognized error)";
  }
}


struct BatchsystemErrCategory : std::error_category
{
  const char* name() const noexcept override;
  std::string message(int ev) const override;
};
 
const char* BatchsystemErrCategory::name() const noexcept {
  return "batchsystem";
}
 
std::string BatchsystemErrCategory::message(int ev) const {
  return to_cstr(static_cast<error_type>(ev));
}
 
const BatchsystemErrCategory error_cat {};

}

namespace cw {
namespace batch {

const std::error_category& batchsystem_category() noexcept {
    return error_cat;
}

std::error_code make_error_code(error_type e) {
  return {static_cast<int>(e), error_cat};
}

}
}
