#ifndef BOOST_PROXY_ERROR
#define BOOST_PROXY_ERROR

#include <string>
#include <exception>
#include <system_error>


namespace cw {
namespace batch {

enum class error {
    node_change_state_out_of_enum = 1,
    queue_state_unknown_not_supported,
    queue_state_out_of_enum,
    system_out_of_enum,
};


enum class batch_condition {
    command_failed = 1,
    invalid_argument,
};

const std::error_category& batchsystem_category() noexcept;

std::error_code make_error_code(error e);

std::error_condition make_error_condition(batch_condition e);

}
}

namespace std
{
  template <>
  struct is_error_code_enum<cw::batch::error> : true_type {};

  template <>
  struct is_error_condition_enum<cw::batch::batch_condition> : true_type {};
}

#endif /* BOOST_PROXY_ERROR */

