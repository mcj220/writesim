#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <gtest/gtest.h>
#include "fs-helpers.h"
#include "json-file.h"

namespace {

class JSONTest : public ::testing::Test {
public:
	static constexpr const char *JF_EXAMPLE_NAME = "./example.json";
	static constexpr const char *JF_NAME = "./t";

protected:
	void SetUp() override
	{
		jf = new FSHelpers::JSONFile(JF_NAME);
	}

	void TearDown() override
	{
		delete jf;
		cleanupFiles();
	}

public:
	static void cleanupFiles()
	{
		::remove(JF_NAME);
	}

	static off_t getFileSize(const std::string &path)
	{
		struct stat statbuf = {0};
		stat(path.c_str(), &statbuf);
		return statbuf.st_size;
	}

	static void createJF()
	{
		::system(std::string(std::string{"cp "} + JF_EXAMPLE_NAME + ' ' + JF_NAME).c_str());
	}

	static void createBadJF()
	{
		createJF();
		::system(std::string(std::string{"truncate -s -10 "} + JF_NAME).c_str());
	}

	FSHelpers::JSONFile *jf;
};

TEST_F(JSONTest, testWriteFile) {
	jf->writeFile(1024);
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
}

TEST_F(JSONTest, testCheckJSON) {
	createJF();
	ASSERT_TRUE(FSHelpers::checkJSON(JF_NAME));
	createBadJF();
	ASSERT_FALSE(FSHelpers::checkJSON(JF_NAME));
	jf->writeFile(1024);
	ASSERT_TRUE(FSHelpers::checkJSON(JF_NAME));
}

TEST_F(JSONTest, testReplayJournal_Recoverable) {
	FSHelpers::writeFile_(JF_NAME, 11);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_EQ(getFileSize(JF_NAME), 11);
}

TEST_F(JSONTest, testReplayJournal_Unrecoverable) {
	createBadJF();
	jf->replayJournal();
	ASSERT_FALSE(FSHelpers::exists(JF_NAME));
}

#if 0
	FSHelpers::writeFile_(JF_RECOVERABLENAME, 3);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_UNRECOVERABLENAME));
	ASSERT_FALSE(FSHelpers::exists(JF_RECOVERABLENAME));
	ASSERT_EQ(getFileSize(JF_NAME), 3);
	cleanupFiles();

	FSHelpers::writeFile_(JF_NAME, 1);
	FSHelpers::writeFile_(JF_UNRECOVERABLENAME, 2);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_FALSE(FSHelpers::exists(JF_UNRECOVERABLENAME));
	ASSERT_FALSE(FSHelpers::exists(JF_RECOVERABLENAME));
	ASSERT_EQ(getFileSize(JF_NAME), 1);
	cleanupFiles();

	FSHelpers::writeFile_(JF_NAME, 1);
	FSHelpers::writeFile_(JF_RECOVERABLENAME, 2);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_EQ(getFileSize(JF_NAME), 1);
	ASSERT_FALSE(FSHelpers::exists(JF_UNRECOVERABLENAME));
	ASSERT_FALSE(FSHelpers::exists(JF_RECOVERABLENAME));
	cleanupFiles();

	FSHelpers::writeFile_(JF_NAME, 1);
	FSHelpers::writeFile_(JF_RECOVERABLENAME, 2);
	FSHelpers::writeFile_(JF_UNRECOVERABLENAME, 3);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_EQ(getFileSize(JF_NAME), 1);
	ASSERT_FALSE(FSHelpers::exists(JF_UNRECOVERABLENAME));
	ASSERT_FALSE(FSHelpers::exists(JF_RECOVERABLENAME));
	cleanupFiles();
#endif

#if 0
	// Invalid state
	FSHelpers::writeFile_(JF_RECOVERABLENAME, 2);
	FSHelpers::writeFile_(JF_UNRECOVERABLENAME, 3);
	jf->replayJournal();
	ASSERT_TRUE(FSHelpers::exists(JF_NAME));
	ASSERT_EQ(getFileSize(JF_NAME), 2);
	ASSERT_FALSE(FSHelpers::exists(JF_UNRECOVERABLENAME));
	ASSERT_FALSE(FSHelpers::exists(JF_RECOVERABLENAME));
	cleanupFiles();
#endif

} // anon
