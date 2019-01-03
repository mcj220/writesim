#include <cstdio>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <gtest/gtest.h>
#include "fs-helpers.h"
#include "wal-file.h"

namespace {

class FSHelpersTest : public ::testing::Test {
public:
	const std::string JF_NAME = "./t";
	const std::string JF_TEMPNAME = JF_NAME + ".first";
	const std::string JF_JOURNALNAME = JF_NAME + ".new";

protected:
	void SetUp() override
	{
		cleanupFiles();
		jf = new FSHelpers::WALFile(JF_NAME);
	}

	void TearDown() override
	{
		delete jf;
		cleanupFiles();
	}

public:
	void cleanupFiles()
	{
		::remove(JF_NAME.c_str());
		::remove(JF_TEMPNAME.c_str());
		::remove(JF_JOURNALNAME.c_str());
	}

	off_t getFileSize(const std::string &path)
	{
		struct stat statbuf = {0};
		stat(path.c_str(), &statbuf);
		return statbuf.st_size;
	}

	FSHelpers::WALFile *jf;
};

TEST_F(FSHelpersTest, testWriteFile) {
	jf->writeFile(1024);
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_TEMPNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_JOURNALNAME));
}

TEST_F(FSHelpersTest, testReplayJournal_CleanJournal) {
	FSHelpers::writeFile_(JF_NAME, 11);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_TEMPNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_JOURNALNAME));
	ASSERT_EQ(getFileSize(JF_NAME), 11);
}

TEST_F(FSHelpersTest, testReplayJournal_TempClosed) {
	FSHelpers::writeFile_(JF_TEMPNAME, 12);
	jf->replayJournal();
	ASSERT_FALSE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_TEMPNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_JOURNALNAME));
}

TEST_F(FSHelpersTest, testReplayJournal_JournalClosed) {
	FSHelpers::writeFile_(JF_JOURNALNAME, 13);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_TEMPNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_JOURNALNAME));
	ASSERT_EQ(getFileSize(JF_NAME), 13);
}

TEST_F(FSHelpersTest, testReplayJournal_TempOpen) {
	FSHelpers::writeFile_(JF_NAME, 11);
	FSHelpers::writeFile_(JF_TEMPNAME, 12);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_TEMPNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_JOURNALNAME));
	ASSERT_EQ(getFileSize(JF_NAME), 11);
}

TEST_F(FSHelpersTest, testReplayJournal_JournalOpen) {
	FSHelpers::writeFile_(JF_NAME, 11);
	FSHelpers::writeFile_(JF_JOURNALNAME, 12);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_EQ(getFileSize(JF_NAME), 11);
	ASSERT_FALSE(FSHelpers::exists(JF_TEMPNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_JOURNALNAME));
}

} // anon
