#pragma once

#include <cstdlib>
#include <cstdio>
#include <ostream>
#include <memory>
#include <string>

#include <unistd.h>

namespace FSHelpers {

static inline bool exists(const std::string &path)
{
	return access(path.c_str(), R_OK|W_OK) == 0;
}

class File {
public:
	virtual void writeFile(off_t len) = 0;
};

} // namespace FSHelpers
