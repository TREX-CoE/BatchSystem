#ifndef BOOST_PROXY_ERROR
#define BOOST_PROXY_ERROR

#include <string>
#include <exception>
#include <system_error>


namespace cw {
namespace batch {

enum class error_type {
    pbsnodes_x_xml_parse_error = 1,
    qselect_u_failed,
    qdel_failed,
    sacct_X_P_format_ALL_failed,
    scontrol_show_job_all_failed,
    scontrol_version_failed,
    scontrol_show_config_failed,
    bkill_u_failed,
    node_change_state_undrain_unsupported_by_lsf,
    node_change_state_out_of_enum,
    bresume_failed,
    bstop_failed,
    queue_state_unknown_not_supported,
    queue_state_inactive_unsupported_by_lsf,
    queue_state_draining_unsupported_by_lsf,
    queue_state_out_of_enum,
    badmin_failed,
    bhosts_failed,
    bqueues_failed,
    bsub_failed,
    bjob_u_all_failed,
    lsid_failed,
    qstat_f_x_xml_parse_error,
    pbsnodes_change_node_state_failed,
    pbsnodes_set_comment_failed,
    qrls_failed,
    qhold_failed,
    qsig_s_suspend_failed,
    qsig_s_resume_failed,
    qmgr_c_set_queue_failed,
    qrerun_failed,
    pbsnodes_x_failed,
    qstat_Qf_failed,
    qsub_failed,
    qstat_f_x_failed,
    pbsnodes_version_failed,
    scontrol_update_NodeName_State_failed,
    scontrol_update_NodeName_Comment_failed,
    scontrol_release_failed,
    scontrol_hold_failed,
    scancel_failed,
    scancel_u_failed,
    scontrol_suspend_failed,
    scontrol_resume_failed,
    scontrol_update_PartitionName_State_failed,
    scontrol_requeuehold_failed,
    scontrol_requeue_failed,
    scontrol_show_node_failed,
    scontrol_show_partition_all_failed,
    sbatch_failed,
    slurm_job_mode_out_of_enum,
    system_out_of_enum,
};

const std::error_category& batchsystem_category() noexcept;

std::error_code make_error_code(error_type e);

}
}

namespace std
{
  template <>
  struct is_error_code_enum<cw::batch::error_type> : true_type {};
}

#endif /* BOOST_PROXY_ERROR */

