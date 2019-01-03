#pragma once

#include <cstdlib>
#include <memory>
#include <string>
#include <unistd.h>

namespace FSHelpers {

static const off_t BUFLEN = 1024*1024;

enum {
	WRITEFILE_USE_NONE=0x0,
	WRITEFILE_USE_FFLUSH=0x1,
	WRITEFILE_USE_FSYNC=0x2,
};

static inline char *get_randbuf()
{
	static char *randbuf = NULL;

	if (randbuf == NULL) {
		randbuf = new char[BUFLEN];
		int urand = ::open("/dev/urandom", 0, O_RDONLY);
		assert(urand != -1);
		off_t off = 0;
		do {
			off += ::read(urand, &randbuf[off], BUFLEN - off);
		} while (off < BUFLEN);
		::close(urand);
	}

	return randbuf;
}

template<int F=WRITEFILE_USE_NONE> static const void writeFile_(const std::string &path, off_t len)
{
	FILE *f = ::fopen(path.c_str(), "w");
	assert(f != NULL);
	while (len > 0) {
		off_t towrite = len > BUFLEN ? BUFLEN : len;
		int r = ::fwrite(get_randbuf(), towrite, 1, f);
		assert(r == 1);
		len -= towrite;
	}
	if (F & WRITEFILE_USE_FFLUSH) {
		::fflush(f);
	}
	if (F & WRITEFILE_USE_FSYNC) {
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

static inline std::ostream &operator<<(std::ostream &os, const JournalState state)
{
	os << (state == JournalState::First ? "first" :
			state == JournalState::New ? "new" :
				state == JournalState::Inconsistent ? "illegal" :
					"none");
	return os;
}

static inline bool exists(const std::string &path)
{
	return access(path.c_str(), R_OK|W_OK) == 0;
}

class File {
public:
	explicit File(const std::string &path): m_path(path) {}

	void printStatus() const
	{
		if (isJournalRecoverable()) {
			std::cout << "Write to file " << getPath() <<
				" was interrupted! Keeping recoverable new write..." <<
				std::endl;
		} else if (isJournalDirty()) {
			std::cout << "Write to file " << getPath() <<
				" was interrupted! Deleting unrecoverable " <<
				doGetJournalState() << " write..." << std::endl;
		}
	}

	void replayJournal()
	{
		printStatus();

		if (isJournalRecoverable()) {
			doCommit();
		}

		doDeleteJournal();
	}

private:
	virtual void doCommit() =0;
	virtual void doWriteFile(off_t len) =0;
	virtual void doDeleteJournal() =0;
	virtual JournalState doGetJournalState() const =0;

protected:
	const std::string &getPath() const { return m_path; }
	bool hasData() const { return exists(getPath()); }
	bool isJournalClosed() const { return !hasData(); }
	bool isJournalDirty() const { return doGetJournalState() != JournalState::None; }
	bool isJournalConsistent() const { return doGetJournalState() != JournalState::Inconsistent; }
	bool isJournalRecoverable() const { return isJournalClosed() && doGetJournalState() == JournalState::New; }

public:
	virtual void writeFile(off_t len) {
		replayJournal();
		doWriteFile(len);
		doCommit();
	}

private:
	const std::string m_path;
};

} // namespace FSHelpers
