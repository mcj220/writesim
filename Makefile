GTEST_DIR := ../googletest/googletest

WRITESIM_SRC := writesim.cc fs-helpers.cc
TEST_SRC := journalled-file-test.cc fs-helpers.cc $(GTEST_DIR)/src/gtest_main.cc

HDR := fs-helpers.h journalled-file.h

TARGETS := writesim test

CXXFLAGS += -std=c++14

.DEFAULT_GOAL := all

.phony: all
all: $(TARGETS)

writesim: $(WRITESIM_SRC) $(HDR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(WRITESIM_SRC) -o $@

test: $(TEST_SRC) $(HDR) libgtest.a
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
