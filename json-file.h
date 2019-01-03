#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace FSHelpers {

std::ostream &operator<<(std::ostream &os, const JournalState state);

template<typename F> std::unique_ptr<File> create(const std::string &path) { return std::make_unique<F>(path); }

static inline bool checkJSON(const std::string &filename)
{
	return ::system(std::string(std::string{"jq '.' >/dev/null 2>&1 <"} + filename).c_str()) == 0;
}

class JSONFile: public File {
public:
	explicit JSONFile(const std::string &path): File(path) {}

private:
	bool checkJSON() const { return FSHelpers::checkJSON(getPath()); }

	void doCommit() override { /* empty */ }
	void doWriteFile(off_t len) override { 
		writeFile_(getPath(), len);
	}
	void doDeleteJournal() override { ::remove(getPath().c_str()); }

	JournalState doGetJournalState() const override {
		JournalState result = JournalState::NONE;
		if (hasData() && !checkJSON()) {
			result = JournalState::INCOMPLETE;
		}
		return result;
	}
};

} // namespace FSHelpers
