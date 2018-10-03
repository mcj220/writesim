#!/bin/bash -e

DEV='mmcblk0'
DIR="$1"

analyze_type() {
	local type="$1"
	echo "=== $type ==="
	blkparse -D $DIR $DEV -a $type -f">>%d %n %S %C\n" | awk '/^>>W/ { tot++; if ($2 <= 32) slc++; if ($3 % 32) nona++ } END { printf "total writes: %d\nsmall writes: %d (%.0f%)\nnon-aligned writes: %d (%.0f%)\n",tot,slc,100*slc/tot,nona,100*nona/tot }'
}

analyze_type "queue"
analyze_type "issue"

#blkparse -D $DIR $DEV -a queue -f">>%d %n %S %C\n" | awk '/^>>W/ { tot++; if ($4 == "xfsaild/'$DEV'") journ++ } END { printf "total queued: %d\njournalling: %d (%.0f%)\n",tot,journ,100*journ/tot }'
