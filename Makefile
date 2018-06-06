# Copyright AllSeen Alliance. All rights reserved.
#
#    Permission to use, copy, modify, and/or distribute this software for any
#    purpose with or without fee is hereby granted, provided that the above
#    copyright notice and this permission notice appear in all copies.
#
#    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

# Modified by V2 Systems, LLC April 2017

ALLJOYN_DIST := $(AJ_ROOT)/build/$(OS)/$(CPU)/debug/dist/cpp

OBJ_DIR := obj/Debug

BIN_DIR := bin/Debug

ALLJOYN_CPPLIB := $(ALLJOYN_DIST)/lib
COMMON_INC := $(AJ_ROOT)/common/inc

ifeq ($(SECURE), secure)
	DEFINE := -DSECURE
else
	DEFINE=
endif

CXXFLAGS = -Wall -pipe -std=c++11 -fno-rtti -fno-exceptions -Wno-long-long -Wno-deprecated -g -DQCC_OS_LINUX -DQCC_OS_GROUP_POSIX -DQCC_DBG -m32 -DROUTER $(DEFINE)

LIBS = -lstdc++ -lcrypto -lpthread -lalljoyn -lrt -lajrouter -lm 

default: custEMS
custEMS: main.o assetMgr.o seInterface.o emsUtils.o
	mkdir -p $(BIN_DIR)
	$(CXX) -o $(BIN_DIR)/custEMS $(OBJ_DIR)/main.o $(OBJ_DIR)/assetMgr.o $(OBJ_DIR)/seInterface.o $(OBJ_DIR)/emsUtils.o -L$(ALLJOYN_CPPLIB) $(LIBS)

main.o: main.cc $(ALLJOYN_LIB)
	mkdir -p $(OBJ_DIR)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ main.cc

assetMgr.o: assetMgr.cc assetMgr.h $(ALLJOYN_LIB)
	mkdir -p $(OBJ_DIR)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ assetMgr.cc

seInterface.o: seInterface.cc seInterface.h $(ALLJOYN_LIB)
	mkdir -p $(OBJ_DIR)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ seInterface.cc

emsUtils.o: emsUtils.cc emsUtils.h $(ALLJOYN_LIB)
	mkdir -p $(OBJ_DIR)
	$(CXX) -c $(CXXFLAGS) -I$(ALLJOYN_DIST)/inc -o $(OBJ_DIR)/$@ emsUtils.cc

all_clean: clean
	rmdir $(OBJ_DIR)
	rmdir $(BIN_DIR)

clean:
	rm -f $(BIN_DIR)/custEMS
	rm -f $(OBJ_DIR)/main.o 
	rm -f $(OBJ_DIR)/assetMgr.o
	rm -f $(OBJ_DIR)/seInterface.o
	rm -f $(OBJ_DIR)/emsUtils.o

