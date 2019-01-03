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
	return exists(firstPath()) ?
		(exists(newPath()) ? JournalState::Inconsistent :
			JournalState::First) :
			exists(newPath()) ? JournalState::New :
				JournalState::None;
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
	::remove(firstPath().c_str());
	::remove(newPath().c_str());
	//fsync of dir ...?
}

void WALFile::doCommit()
{
	assert(isJournalDirty());
	assert(isJournalConsistent());
	finalizeJournal();
	if (doGetJournalState() == JournalState::New) {
		moveFile(newPath());
	} else {
		moveFile(firstPath());
	}
}

} // namespace FSHelpers
