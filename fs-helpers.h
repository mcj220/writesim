#pragma once

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <iostream>
#include <string>
#include <streambuf>

#include <unistd.h>

namespace FSHelpers {

static const off_t BUFLEN = 1024*1024;

enum {
	WRITEFILE_USE_NONE=0x0,
	WRITEFILE_USE_FFLUSH=0x1,
	WRITEFILE_USE_FSYNC=0x2,
};

static inline const std::string &get_randbuf()
{
	static std::string randbuf;

	if (randbuf.empty()) {
		randbuf.reserve(BUFLEN);
		std::ifstream urand("/dev/urandom", ::std::ios::binary);
		assert(urand);
		for(off_t i=0; i<BUFLEN; ++i) {
			randbuf.push_back(urand.get() % ('Z' - 'A' + 1) + 'A');
		}
	}

	return randbuf;
}

template<int F=WRITEFILE_USE_NONE> static const void writeFile_(const std::string &path, off_t len)
{
	FILE *f = ::fopen(path.c_str(), "w");
	assert(f != NULL);
	assert(len >= 10);

	// Wrap as JSON object
	int r = ::fwrite("{\"d\":\"", 6, 1, f);
	assert(r == 1);
	len -= 9;

	while (len > 0) {
		off_t towrite = len > BUFLEN ? BUFLEN : len;
		r = ::fwrite(get_randbuf().c_str(), towrite, 1, f);
		assert(r == 1);
		len -= towrite;
	}

	// JSON epilogue
	r = ::fwrite("\"}\n", 3, 1, f);
	assert(r == 1);

	if (F & WRITEFILE_USE_FFLUSH) {
		::fflush(f);
	}
	if (F & WRITEFILE_USE_FSYNC) {
		::fsync(fileno(f));
	}
	::fclose(f);
}

enum class JournalState {
	NONE,
	INCOMPLETE,
	RECOVERABLE
};

static inline std::ostream &operator<<(std::ostream &os, const JournalState state)
{
	os << (state == JournalState::INCOMPLETE ? "first" :
			state == JournalState::RECOVERABLE ? "new" : "none");
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

		if (isJournalDirty()) {
			if (isJournalRecoverable()) {
				doCommit();
			} else {
				doDeleteJournal();
			}
		}
	}

private:
	virtual void doCommit() =0;
	virtual void doWriteFile(off_t len) =0;
	virtual void doDeleteJournal() =0;
	virtual JournalState doGetJournalState() const =0;

protected:
	const std::string &getPath() const { return m_path; }
	bool hasData() const { return exists(getPath()); }
	bool isJournalDirty() const { return doGetJournalState() != JournalState::NONE; }
	bool isJournalRecoverable() const { return doGetJournalState() == JournalState::RECOVERABLE; }

public:
	virtual void writeFile(off_t len) {
		if (isJournalDirty()) {
			doDeleteJournal();
		}
		doWriteFile(len);
		doCommit();
	}

private:
	const std::string m_path;
};

} // namespace FSHelpers
