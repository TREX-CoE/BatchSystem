#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

namespace cw {
namespace batch {
namespace internal {

static inline std::stringstream runCommand(std::function<cmd_execute_f> _func, const std::string& cmd, const std::vector<std::string>& args) {
	std::stringstream commandResult;
	int ret = _func(commandResult, cmd, args);
	if (ret != 0) throw CommandFailed("Command failed", cmd, ret);
	return commandResult;
}

}	
}
}
