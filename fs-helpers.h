#pragma once

//#define USE_FALLOCATE
#define USE_FSYNC

extern int renameAndSync(const char *oldpath, const char *newpath);
extern void writeFile_(const char *path, size_t len);
extern int ensureFileConsistency(const char *path);
extern void writeFile(const char *path, size_t len);
