#include "batchsystem/error.h"

#include <string>
#include <exception>
#include <system_error>
#include <cassert>

namespace {

using namespace cw::batch;

const char* to_cstr(error type) {
  switch (type) {
      case error::node_change_state_out_of_enum: return "node state change invalid enum value";
      case error::queue_state_unknown_not_supported: return "queue state unknown not supported";
      case error::queue_state_out_of_enum: return "queue state invalid enum value";
      case error::system_out_of_enum: return "batchsystem system invalid enum value";
      default: return "(unrecognized error)";
  }
}

const char* to_cstr(batch_condition type) {
  switch (type) {
      case cw::batch::batch_condition::command_failed: return "failed batchsystem command";
      case cw::batch::batch_condition::invalid_argument: return "invalid argument given";
      default: return "(unrecognized error)";
  }
}



struct BatchsystemErrCategory : std::error_category
{

  const char* name() const noexcept {
    return "cw::batch";
  }
 
  std::string message(int ev) const {
    return to_cstr(static_cast<error>(ev));
  }

  std::error_condition default_error_condition(int err_value) const noexcept {
      switch (static_cast<error>(err_value)) {
        case error::node_change_state_out_of_enum: return cw::batch::batch_condition::command_failed;
        case error::queue_state_unknown_not_supported: return cw::batch::batch_condition::command_failed;
        case error::queue_state_out_of_enum: return cw::batch::batch_condition::command_failed;
        case error::system_out_of_enum: return cw::batch::batch_condition::command_failed;
        default: assert(false && "unknown error");
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

std::error_code make_error_code(error e) {
  return {static_cast<int>(e), error_cat};
}

std::error_condition make_error_condition(batch_condition e) {
  return {static_cast<int>(e), cond_cat};
}

}
}
