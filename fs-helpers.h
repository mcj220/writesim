#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>

//#define USE_FALLOCATE
#define USE_FILE_FSYNC
#define USE_DIR_FSYNC

namespace FSHelpers {

static const off_t BUFLEN = 1024*1024;

static inline char *get_randbuf()
{
	static char *randbuf = NULL;

	if (randbuf == NULL) {
		randbuf = new char[BUFLEN];
		int urand = open("/dev/urandom", 0, O_RDONLY);
		assert(urand != -1);
		off_t off = 0;
		do {
			off += read(urand, &randbuf[off], BUFLEN - off);
		} while (off < BUFLEN);
		close(urand);
	}

	return randbuf;
}

static inline const void writeFile_(const std::string &path, off_t len)
{
	FILE *f = fopen(path.c_str(), "w");
	assert(f != NULL);
#ifdef USE_FALLOCATE
	fallocate(fileno(f), 0, 0, len);
#endif
	while (len > 0) {
		off_t towrite = len > BUFLEN ? BUFLEN : len;
		int r = fwrite(get_randbuf(), towrite, 1, f);
		assert(r == 1);
		len -= towrite;
	}
	fflush(f);
#ifdef USE_FILE_FSYNC
	fsync(fileno(f));
#endif
	fclose(f);
}

static inline bool exists(const std::string &path)
{
	return access(path.c_str(), R_OK|W_OK) == 0;
}

enum class JournalState {
	None,
	First,
	New
};

static inline std::ostream &operator<<(std::ostream &os, const JournalState state)
{
	os << (state == JournalState::First ? "first" : state == JournalState::New ? "new" : "none");
	return os;
}

class JournalledFile {
public:
	JournalledFile(const std::string &path): m_path(path) {}

private:
	const std::string &getPath() const { return m_path; }
	const std::string firstPath() const { return getPath() + ".first"; }
	const std::string newPath() const { return getPath() + ".new"; }

	JournalState getJournalState() const
	{
		return exists(firstPath()) ?
			JournalState::First :
			exists(newPath()) ? JournalState::New : JournalState::None;
	}

	bool hasData() const { return exists(getPath()); }
	bool isClosedJournal() const { return !hasData(); }
	bool isDirty() const { return getJournalState() == JournalState::First || getJournalState() == JournalState::New; }
	bool isRecoverable() const { return isClosedJournal() && getJournalState() == JournalState::New; }

private:
	void move(const std::string &path)
	{
		::rename(path.c_str(), getPath().c_str());
#ifdef USE_DIR_FSYNC
		char *dir = dirname(strdupa(getPath().c_str()));
		int d = open(dir, O_DIRECTORY);
		if (d == -1) throw std::runtime_error("Can't open directory");
		if (fsync(d) == -1) throw std::runtime_error("fsync failed");
		::close(d);
#endif
	}

	void printStatus() const
	{
		if (isRecoverable()) {
			std::cout << "Write to file " << getPath() <<
				" was interrupted! Keeping recoverable new write..." <<
				std::endl;
		} else if (isDirty()) {
			std::cout << "Write to file " << getPath() <<
				" was interrupted! Deleting unrecoverable " <<
				getJournalState() << " write..." << std::endl;
		}
	}

	void deleteJournal()
	{
		::remove(firstPath().c_str());
		::remove(newPath().c_str());
		//fsync of dir ...?
	}

public:
	const std::string getJournalPath() const { return hasData() ? newPath() : firstPath(); }
	void closeJournal() { ::remove(getPath().c_str()); }

	void replayJournal()
	{
		printStatus();

		if (isRecoverable()) {
			commit();
		}

		deleteJournal();
	}

	void commit()
	{
		if (!isDirty()) throw std::runtime_error("Nothing to commit");
		if (getJournalState() == JournalState::New) {
			move(newPath());
		} else {
			move(firstPath());
		}
	}

	void writeFile(off_t len)
	{
		replayJournal();
		writeFile_(getJournalPath(), len);
		closeJournal();
		commit();
	}

private:
	const std::string m_path;
};

} // namespace FSHelpers
