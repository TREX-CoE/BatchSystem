#include "batchsystem/batchsystem.h"

namespace cw {
namespace batch {
namespace internal {

class SingleCmd {
protected:
    cmd_f& cmd;
    Result res;
    enum class State {
        Starting,
        Waiting,
        Done,
    };
    State state = State::Starting;

	bool checkWaiting(std::error_code e) {
		if (res.exit==not_finished) {
			return false;
		} else if (res.exit!=0) {
			throw std::system_error(e);
		}
		state=State::Done;
		return true;
	}
public:
	SingleCmd(cmd_f& cmd_): cmd(cmd_) {}
};

}	
}
}
