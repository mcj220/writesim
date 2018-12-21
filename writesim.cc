#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

#include <libgen.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fs-helpers.h"
#include "journalled-file.h"

#define MAX_FILESIZE 262144
#define DEFAULT_REWRITE_PERCENT 10
#define DEFAULT_NUM_FILES 10
#define MBYTE 1048576ULL
#define GBYTE 1073741824ULL

int num_files = DEFAULT_NUM_FILES;
off_t max_filesize = MAX_FILESIZE;
char *stemname = NULL;
int num_mbytes = -1;
int rewrite_percent = DEFAULT_REWRITE_PERCENT;

enum {
	ARG_POS_BASENAME = 1,
	ARG_POS_NUM_MBYTES,
	ARG_POS_TARGET_REWRITE_PERCENT,
	ARG_POS_NUM_FILES,
	ARG_POS_MAX_FILESIZE,
	MAX_ARG_POS
};

namespace {
	static void usage(int argc, char *argv[])
	{
		fprintf(stderr, "Usage: %s <stemname> <num_mbytes> [<target_rewrite_percent> <num_files> <max_filesize>]\n", basename(argv[0]));
		exit(EXIT_FAILURE);
	}

	static std::string make_fname(const std::string &stem, int i)
	{
		std::stringstream ss;
		ss << stem << '_' << i;
		return ss.str();
	}

	static void syncall()
	{
		char *n = NULL;
		int fd;

		asprintf(&n, "%s_%d", stemname, 0);
		fd = open(n, 0);
		assert(fd != -1);
		::syncfs(fd);
		::close(fd);
		free(n); n = NULL;
	}

	static void write_initial_set(int nfiles)
	{
		int64_t data_written = 0;
		for(int i=0; i < nfiles; ++i) {
			off_t filesz = rand() % max_filesize;

			auto jf = FSHelpers::create<FSHelpers::JournalledFile>(make_fname(stemname, i));
			jf->writeFile(filesz);

			data_written += filesz;
		}
		std::cout << "Wrote initial " << (float) data_written / MBYTE << "MB" << std::endl;
	}
} // namespace

int main(int argc, char *argv[])
{
	if (argc < 3) {
		usage(argc, argv);
	}

	stemname = argv[ARG_POS_BASENAME];
	num_mbytes = atoi(argv[ARG_POS_NUM_MBYTES]);

	if (argc > ARG_POS_TARGET_REWRITE_PERCENT) {
		rewrite_percent = atoi(argv[3]);
	}

	if (argc > ARG_POS_NUM_FILES) {
		num_files = atoi(argv[ARG_POS_NUM_FILES]);
	}

	if (argc > ARG_POS_MAX_FILESIZE) {
		max_filesize = atoi(argv[ARG_POS_MAX_FILESIZE]);
	}

	struct timespec starttime;
	clock_gettime(CLOCK_REALTIME, &starttime);
	//srand(starttime.tv_nsec);
	srand(0xaa55);

	write_initial_set(num_files);
	for(int data_written=0, cycle=0; data_written < (int64_t) num_mbytes * MBYTE; ) {
		for(int i=0; i < num_files * rewrite_percent / 100; ++i) {
			off_t filesz = rand() % max_filesize;

			auto jf = FSHelpers::create<FSHelpers::JournalledFile>(make_fname(stemname, rand() % num_files));
			jf->writeFile(filesz);

			data_written += filesz;
		}
		std::cout << "Wrote " << (float) data_written / MBYTE << "MB" << std::endl;
		std::cout << "Completed " << cycle++ << " cycles" << std::endl;
	}

	struct timespec endtime;
	syncall();

	clock_gettime(CLOCK_REALTIME, &endtime);
	printf("Throughput: %.2lf Mbytes/s\n", (double)(num_mbytes * 1024) / (endtime.tv_sec - starttime.tv_sec));

	return 0;
}
