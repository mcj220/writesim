#include <cstdio>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <gtest/gtest.h>
#include "fs-helpers.h"

namespace {

class FSHelpersTest : public ::testing::Test {
public:
	const std::string JF_NAME = "./t";
	const std::string JF_FIRSTNAME = JF_NAME + ".first";
	const std::string JF_NEWNAME = JF_NAME + ".new";

protected:
	void SetUp() override
	{
		jf = new FSHelpers::JournalledFile(JF_NAME);
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
		::remove(JF_FIRSTNAME.c_str());
		::remove(JF_NEWNAME.c_str());
	}

	off_t getFileSize(const std::string &path)
	{
		struct stat statbuf = {0};
		stat(path.c_str(), &statbuf);
		return statbuf.st_size;
	}

	void assert_delete_journal();
	FSHelpers::JournalledFile *jf;
};

TEST_F(FSHelpersTest, testWriteFile) {
	jf->writeFile(1024);
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_FIRSTNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_NEWNAME));
}

TEST_F(FSHelpersTest, testReplayJournal) {
	FSHelpers::writeFile_(JF_NAME, 1);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_FIRSTNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_NEWNAME));
	ASSERT_EQ(getFileSize(JF_NAME), 1);
	cleanupFiles();

	FSHelpers::writeFile_(JF_FIRSTNAME, 2);
	jf->replayJournal();
	ASSERT_FALSE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_FIRSTNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_NEWNAME));
	cleanupFiles();

	FSHelpers::writeFile_(JF_NEWNAME, 3);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_FIRSTNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_NEWNAME));
	ASSERT_EQ(getFileSize(JF_NAME), 3);
	cleanupFiles();

	FSHelpers::writeFile_(JF_NAME, 1);
	FSHelpers::writeFile_(JF_FIRSTNAME, 2);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_FIRSTNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_NEWNAME));
	ASSERT_EQ(getFileSize(JF_NAME), 1);
	cleanupFiles();

	FSHelpers::writeFile_(JF_NAME, 1);
	FSHelpers::writeFile_(JF_NEWNAME, 2);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_EQ(getFileSize(JF_NAME), 1);
	ASSERT_FALSE(FSHelpers::exists(JF_FIRSTNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_NEWNAME));
	cleanupFiles();

	FSHelpers::writeFile_(JF_NAME, 1);
	FSHelpers::writeFile_(JF_NEWNAME, 2);
	FSHelpers::writeFile_(JF_FIRSTNAME, 3);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_EQ(getFileSize(JF_NAME), 1);
	ASSERT_FALSE(FSHelpers::exists(JF_FIRSTNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_NEWNAME));
	cleanupFiles();

#if 0
	// Invalid state
	FSHelpers::writeFile_(JF_NEWNAME, 2);
	FSHelpers::writeFile_(JF_FIRSTNAME, 3);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_EQ(getFileSize(JF_NAME), 2);
	ASSERT_FALSE(FSHelpers::exists(JF_FIRSTNAME));
	ASSERT_FALSE(FSHelpers::exists(JF_NEWNAME));
	cleanupFiles();
#endif
}

} // anon
