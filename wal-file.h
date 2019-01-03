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
	const std::string tempPath() const { return getPath() + ".first"; }
	const std::string walPath() const { return getPath() + ".new"; }

	void moveFile(const std::string &path);
	const std::string selectBuffer() const { return hasData() ? walPath() : tempPath(); }
	void doCloseJournal() { ::remove(getPath().c_str()); }
	bool isJournalClosed() const { return !hasData(); }

	void doCommit() override;
	void doWriteFile(off_t len) override { 
		writeFile_<WRITEFILE_USE_FSYNC|WRITEFILE_USE_FFLUSH>(selectBuffer(), len);
	}
	void doDeleteJournal() override;
	JournalState doGetJournalState() const override;
};

} // namespace FSHelpers
