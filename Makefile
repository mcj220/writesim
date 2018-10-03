GTEST_DIR := ../googletest/googletest

WRITESIM_SRC := writesim.cc
WRITE1_SRC := write1.cc
TEST_SRC := fs-helpers-test.cc $(GTEST_DIR)/src/gtest_main.cc

TARGETS := writesim write1 test

.DEFAULT_GOAL := all

.phony: all
all: $(TARGETS)

writesim: $(WRITESIM_SRC) fs-helpers.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(WRITESIM_SRC) -o $@

write1: $(WRITE1_SRC) fs-helpers.h
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(WRITE1_SRC) -o $@

test: $(TEST_SRC) fs-helpers.h libgtest.a
	$(CXX) -isystem $(GTEST_DIR)/include -pthread $(TEST_SRC) libgtest.a -o $@

libgtest.a:
	$(CXX) -isystem $(GTEST_DIR)/include -I$(GTEST_DIR) -pthread -c $(GTEST_DIR)/src/gtest-all.cc
	$(AR) -rv libgtest.a gtest-all.o

.phony: clean
clean:
	$(RM) $(TARGETS)

.phony: cleanall
cleanall: clean
	$(RM) gtest-all.o libgtest.a
