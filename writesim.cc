#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include <libgen.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fs-helpers.h"

#define MAX_FILESIZE 262144
#define DEFAULT_REWRITE_PERCENT 10
#define DEFAULT_NUM_FILES 1024
#define GBYTE 1073741824ULL

int num_files = DEFAULT_NUM_FILES;
off_t max_filesize = MAX_FILESIZE;
char *stemname = NULL;
int num_gbytes = -1;
int rewrite_percent = DEFAULT_REWRITE_PERCENT;

int64_t data_written = 0;

enum {
	ARG_POS_BASENAME = 1,
	ARG_POS_NUM_GBYTES,
	ARG_POS_TARGET_REWRITE_PERCENT,
	ARG_POS_NUM_FILES,
	ARG_POS_MAX_FILESIZE,
	MAX_ARG_POS
};

struct writer_args {
	int lower;
	int higher;
	int i;
};

void usage(int argc, char *argv[])
{
	fprintf(stderr, "Usage: %s <stemname> <num_gbytes> [<target_rewrite_percent> <num_files> <max_filesize>]\n", basename(argv[0]));
	exit(EXIT_FAILURE);
}

void clean_files()
{
	for(int i=0; i<num_files; ++i) {
		char *n = NULL;
		asprintf(&n, "%s_%d", stemname, i);
		unlink(n);
		free(n);
	}
}

int64_t writer_1(void *args)
{
	int64_t result = 0;
	writer_args *range = static_cast<writer_args *>(args);
	assert(range != NULL);

loop:
	if (range->i >= range->higher)
		return result;

	char *n = NULL;
	int j = range->i;
	off_t filesz = rand() % max_filesize;

	if (j > 0 && (rand() % 100) < rewrite_percent) {
		j = range->lower + (rand() % (j-range->lower));
	}

	asprintf(&n, "%s_%d", stemname, j);
	FSHelpers::JournalledFile jf(n);
	jf.writeFile(filesz);
	free(n); n = NULL;

	result += filesz;
	++range->i;
	goto loop;

	return result;
}

void syncfs()
{
	char *n = NULL;
	int fd;

	asprintf(&n, "%s_%d", stemname, 0);
	fd = open(n, 0);
	assert(fd != -1);
	syncfs(fd);
	free(n); n = NULL;
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		usage(argc, argv);
	}

	stemname = argv[ARG_POS_BASENAME];
	num_gbytes = atoi(argv[ARG_POS_NUM_GBYTES]);

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

	for(int cycles=1; ; ++cycles) {
		clean_files();
		struct writer_args args = {0, num_files, 0};
		data_written += writer_1(&args);
		printf("Wrote %llu%% of %d GB\n", data_written*100 / ((int64_t) num_gbytes * GBYTE), num_gbytes);

		if (data_written >= (int64_t) num_gbytes * GBYTE) {
			goto out;
		}
		printf("Completed %d cycles\n", cycles);
	} while (true);

	struct timespec endtime;
out:
	syncfs();

	clock_gettime(CLOCK_REALTIME, &endtime);
	printf("Throughput: %.2lf Mbytes/s\n", (double)(num_gbytes * 1024) / (endtime.tv_sec - starttime.tv_sec));

	return 0;
}
