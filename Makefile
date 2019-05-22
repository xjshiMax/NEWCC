
CC=gcc
CFLAGS=  -std=c++11 -g -Wall -Wno-unused-variable -pthread -lmysqlcppconn -lstdc++ -lcurl -lcrypto  -lrt -lm
INCLUDES  =  -I./common/ -I./cppconn/ -I./ -I./mysqlcnn/include/jdbc/
INCLUDES +=  -I./esl -I./websocketpp -I/home/workstation/boost_1_47_0/
INCLUDES += -I./base/output/include/
LIBS = -L./mysqlcnn/lib64/ 
LIBS += -L./base/jsoncpp/lib/ -ljson-cpp
LIBS += -L./base/output/lib/linux/ -lseabase
LIBS += -L./base/sqlite3/lib/linux/ -lsqlite3
LIBS += -L/home/workstation/boost_1_47_0/stage/lib -lboost_system
SRC =   main.cpp ./base/inifile/inifile.cpp \
	   WS_ApServer.cpp DNManager.cpp IVRmanager.cpp ACDqueue.cpp CalloutManager.cpp \
	   esl/esl.c esl/esl_json.c esl/esl_threadmutex.c esl/esl_buffer.c esl/esl_event.c esl/esl_config.c \
	   common/speech/common.c common/speech/token.cpp common/speech/ttscurl.c common/codeHelper.cpp common/structdef.cpp \
	   common/DBOperator.cpp \
	   database/config/inirw.cpp database/dbPool.cpp \
	   
OUTPUT_PATH=./output/
target: c

c:$(SRC)
	$(CC) -o $@ $^ $(INCLUDES) $(CFLAGS) $(LIBS)
	if [ ! -d "./output/" ]; then  mkdir $(OUTPUT_PATH); fi

	cp c output/
	cp ./mysqlcnn/lib64/* output/
	cp /home/workstation/boost_1_47_0/stage/lib/libboost_system.so.1.47.0 output/
	cp database.conf output/
	cp ./base/output/lib/linux/*.so output/
	cp database.conf output/

clean:
	rm -f c *.o *.obj
