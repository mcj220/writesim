#pragma once

#include <cassert>
#include <cstdio>
#include <string>

namespace FSHelpers {

std::ostream &operator<<(std::ostream &os, const JournalState state);

template<typename F> std::unique_ptr<File> create(const std::string &path) { return std::make_unique<F>(path); }

class WALFile: public File {
public:
	explicit WALFile(const std::string &path): File(path) {}

private:
	const std::string firstPath() const { return getPath() + ".first"; }
	const std::string newPath() const { return getPath() + ".new"; }

	void moveFile(const std::string &path);
	const std::string getJournalPath() const { return hasData() ? newPath() : firstPath(); }
	void finalizeJournal() { ::remove(getPath().c_str()); }

	void doCommit() override;
	void doWriteFile(off_t len) override { 
		writeFile_<WRITEFILE_USE_FSYNC|WRITEFILE_USE_FFLUSH>(getJournalPath(), len);
	}
	void doDeleteJournal() override;
	JournalState doGetJournalState() const override;
};

} // namespace FSHelpers
