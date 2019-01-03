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
#include "wal-file.h"

namespace FSHelpers {

JournalState WALFile::doGetJournalState() const
{
	JournalState result = JournalState::NONE;

	if (exists(tempPath())) {
		assert(!exists(walPath()));
		result = JournalState::INCOMPLETE;
	} else if (exists(walPath())) {
		result = isJournalClosed() ? JournalState::RECOVERABLE : JournalState::INCOMPLETE;
	}

	return result;
}

void WALFile::moveFile(const std::string &path)
{
	::rename(path.c_str(), getPath().c_str());
	char *dir = ::dirname(strdupa(getPath().c_str()));
	int d = ::open(dir, O_DIRECTORY);
	if (d == -1) throw std::runtime_error("Can't open directory");
	if (fsync(d) == -1) throw std::runtime_error("fsync failed");
	::close(d);
}

void WALFile::doDeleteJournal()
{
	::remove(tempPath().c_str());
	::remove(walPath().c_str());
	//fsync of dir ...?
}

void WALFile::doCommit()
{
	assert(isJournalDirty());
	assert(!exists(walPath()) || !exists(tempPath())); 
	doCloseJournal();
	if (doGetJournalState() == JournalState::RECOVERABLE) {
		moveFile(walPath());
	} else {
		moveFile(tempPath());
	}
}

} // namespace FSHelpers
