#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

namespace cw {
namespace batch {
namespace internal {

static inline std::string runCommand(std::function<cmd_execute_f> _func, const CmdOptions& opts) {
	std::string out;
	int ret = _func(out, opts);
	if (ret != 0) throw CommandFailed("Command failed", opts, ret);
	return out;
}

}	
}
}
