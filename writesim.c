#define _GNU_SOURCE

#include <assert.h>
#include <libgen.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "fs-helpers.h"

#define MAX_FILESIZE 262144
#define DEFAULT_REWRITE_PERCENT 10
#define DEFAULT_NUM_FILES 1024
#define GBYTE 1073741824ULL

int num_files = DEFAULT_NUM_FILES;
size_t max_filesize = MAX_FILESIZE;
char *stemname = NULL;
int num_gbytes = -1;
int rewrite_percent = DEFAULT_REWRITE_PERCENT;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
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

void *writer(void *args)
{
	struct writer_args *range = args;
	assert(range != NULL);

	for(int i=range->lower; i<range->higher; ++i) {
		char *n = NULL;
		int j = i;
		size_t filesz = rand() % max_filesize;

		if (i > 0 && (rand() % 100) < rewrite_percent) {
			j = range->lower + (rand() % (i-range->lower));
		}

		asprintf(&n, "%s_%d", stemname, j);
		writeFile(n, filesz);
		free(n); n = NULL;

		pthread_mutex_lock(&lock);
		data_written += filesz;
		pthread_mutex_unlock(&lock);
	}

	return NULL;
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

	struct timespec time_n0, starttime;
	clock_gettime(CLOCK_REALTIME, &starttime);
	srand(starttime.tv_nsec);
	memcpy(&time_n0, &starttime, sizeof(struct timespec));

	for(int cycles=1; ; ++cycles) {
		clean_files();
		struct writer_args args = {0, num_files};
		pthread_t tid;
		pthread_create(&tid, NULL, writer, &args);
#if 0
		while (true) {
			struct timespec time_n1;
			clock_gettime(CLOCK_REALTIME, &time_n1);
			if (time_n1.tv_sec > time_n0.tv_sec) {
				memcpy(&time_n0, &time_n1, sizeof(struct timespec));
				printf("Wrote %llu%% of %d GB\n", data_written*100 / ((int64_t) num_gbytes * GBYTE), num_gbytes);
			}

			if (data_written >= (int64_t) num_gbytes * GBYTE) {
				goto out;
			}
		}
#endif
		pthread_join(tid, NULL);
		printf("Completed %d cycles\n", cycles);
	} while (true);

	struct timespec endtime;
out:
	clock_gettime(CLOCK_REALTIME, &endtime);
	printf("Throughput: %.2lf Mbytes/s\n", (double)(num_gbytes * 1024) / (endtime.tv_sec - starttime.tv_sec));

	return 0;
}
