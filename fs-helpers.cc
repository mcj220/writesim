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

#include "fs-helpers.h"
#include "journalled-file.h"

namespace FSHelpers {

char *get_randbuf()
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

std::ostream &operator<<(std::ostream &os, const JournalState state)
{
	os << (state == JournalState::First ? "first" :
			state == JournalState::New ? "new" :
				state == JournalState::Inconsistent ? "illegal" :
					"none");
	return os;
}

JournalState JournalledFile::getJournalState() const
{
	return exists(firstPath()) ?
		(exists(newPath()) ? JournalState::Inconsistent :
			JournalState::First) :
			exists(newPath()) ? JournalState::New :
				JournalState::None;
}

void JournalledFile::moveFile(const std::string &path)
{
	::rename(path.c_str(), getPath().c_str());
	char *dir = ::dirname(strdupa(getPath().c_str()));
	int d = ::open(dir, O_DIRECTORY);
	if (d == -1) throw std::runtime_error("Can't open directory");
	if (fsync(d) == -1) throw std::runtime_error("fsync failed");
	::close(d);
}

void JournalledFile::printStatus() const
{
	if (isJournalRecoverable()) {
		std::cout << "Write to file " << getPath() <<
			" was interrupted! Keeping recoverable new write..." <<
			std::endl;
	} else if (isJournalDirty()) {
		std::cout << "Write to file " << getPath() <<
			" was interrupted! Deleting unrecoverable " <<
			getJournalState() << " write..." << std::endl;
	}
}

void JournalledFile::deleteJournal()
{
	::remove(firstPath().c_str());
	::remove(newPath().c_str());
	//fsync of dir ...?
}

void JournalledFile::replayJournal()
{
	printStatus();

	if (isJournalRecoverable()) {
		commit();
	}

	deleteJournal();
}

void JournalledFile::commit()
{
	assert(isJournalDirty());
	assert(isJournalConsistent());
	finalizeJournal();
	if (getJournalState() == JournalState::New) {
		moveFile(newPath());
	} else {
		moveFile(firstPath());
	}
}

void JournalledFile::writeFile(off_t len)
{
	replayJournal();
	writeFile_(getJournalPath(), len);
	commit();
}

} // namespace FSHelpers
