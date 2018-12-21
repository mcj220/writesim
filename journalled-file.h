#pragma once

#include <cassert>
#include <cstdio>
#include <string>

namespace FSHelpers {

static const off_t BUFLEN = 1024*1024;

enum {
	USE_FFLUSH=0x1,
	USE_FSYNC=0x2,
};

char *get_randbuf();

template<int F=USE_FSYNC|USE_FFLUSH> static const void writeFile_(const std::string &path, off_t len)
{
	FILE *f = ::fopen(path.c_str(), "w");
	assert(f != NULL);
	while (len > 0) {
		off_t towrite = len > BUFLEN ? BUFLEN : len;
		int r = ::fwrite(get_randbuf(), towrite, 1, f);
		assert(r == 1);
		len -= towrite;
	}
	if (F & USE_FFLUSH) {
		::fflush(f);
	}
	if (F & USE_FSYNC) {
		::fsync(fileno(f));
	}
	::fclose(f);
}

enum class JournalState {
	None,
	First,
	New,
	Inconsistent
};

std::ostream &operator<<(std::ostream &os, const JournalState state);

class JournalledFile;

template<typename F> std::unique_ptr<File> create(const std::string &path) { return std::make_unique<F>(path); }

class JournalledFile: public File {
public:
	explicit JournalledFile(const std::string &path): m_path(path) {}

private:

	const std::string &getPath() const { return m_path; }
	const std::string firstPath() const { return getPath() + ".first"; }
	const std::string newPath() const { return getPath() + ".new"; }
	JournalState getJournalState() const;

	bool hasData() const { return exists(getPath()); }
	bool isJournalClosed() const { return !hasData(); }
	bool isJournalDirty() const { return getJournalState() != JournalState::None; }
	bool isJournalConsistent() const { return getJournalState() != JournalState::Inconsistent; }
	bool isJournalRecoverable() const { return isJournalClosed() && getJournalState() == JournalState::New; }

	void moveFile(const std::string &path);
	void printStatus() const;
	void deleteJournal();
	const std::string getJournalPath() const { return hasData() ? newPath() : firstPath(); }
	void finalizeJournal() { ::remove(getPath().c_str()); }
	void commit();

public:
	void writeFile(off_t len);
	void replayJournal();

private:
	const std::string m_path;
};

} // namespace FSHelpers
