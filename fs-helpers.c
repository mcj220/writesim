#include <assert.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fs-helpers.h"

#define BUFLEN (1024*1024)

static char *get_randbuf()
{
	static char *randbuf = NULL;

	if (randbuf == NULL) {
		randbuf = (char *) malloc(BUFLEN);
		int urand = open("/dev/urandom", 0, O_RDONLY);
		assert(urand != -1);
		size_t off = 0;
		do {
			off += read(urand, &randbuf[off], BUFLEN - off);
		} while (off < BUFLEN);
		close(urand);
	}

	return randbuf;
}

int renameAndSync(const char *oldpath, const char *newpath)
{
	int r = rename(oldpath, newpath);
#ifdef USE_DIR_FSYNC
	if (r != -1) {
		int d = open(dirname((char *) newpath), O_DIRECTORY);
		assert(d != -1);
		int x = fsync(d);
		assert(x != -1);
		close(d);
	}
#endif

	return r;
}

void writeFile_(const char *path, size_t len)
{
	FILE *f = fopen(path, "w");
	assert(f != NULL);
#ifdef USE_FALLOCATE
	fallocate(fileno(f), 0, 0, len);
#endif

	while (len > 0) {
		size_t towrite = len > BUFLEN ? BUFLEN : len;
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

int ensureFileConsistency(const char *path)
{
	char *newpath, *firstpath;
	asprintf(&newpath, "%s.new", path);
	asprintf(&firstpath, "%s.first", path);

	int exists = access(path, R_OK|W_OK) == 0;

	if (access(firstpath, R_OK|W_OK) == 0) {
		printf("Write to file %s was interrupted! Deleting uncommitted first write...\n", path);
		unlink(firstpath); // No fsync!!
		return exists;
	}

	if (access(newpath, R_OK|W_OK) == 0) {
		if (exists) {
			printf("Write to file %s was interrupted! Deleting uncommitted new write...\n", path);
			unlink(newpath); // No fsync!!
			return 1;
		}
		printf("Write to file %s was interrupted! Keeping committed new write...\n", path);
		renameAndSync(newpath, path);
		return 1;
	}

	return exists;
}

void writeFile(const char *path, size_t len)
{
	if (ensureFileConsistency(path)) {
		char *newpath;
		asprintf(&newpath, "%s.new", path);
		writeFile_(newpath, len);
		unlink(path);
		renameAndSync(newpath, path);
	} else {
		char *firstpath;
		asprintf(&firstpath, "%s.first", path);
		writeFile_(firstpath, len);
		renameAndSync(firstpath, path);
	}
}
