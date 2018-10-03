#include "fs-helpers.h"

int main(int argc, char *argv[])
{
	FSHelpers::JournalledFile jf("test");
	jf.writeFile(66);
	return 0;
}
