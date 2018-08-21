SRC := writesim.c fs-helpers.c
LDFLAGS := $(LDFLAGS) -pthread

writesim: $(SRC) fs-helpers.h
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $@

.phony: clean
clean:
	$(RM) writesim
