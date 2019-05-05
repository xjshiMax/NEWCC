cmake_minimum_required(VERSION 2.8)
#add_definitions("-Wall -std=c++11 -pthread") # <= 新增的编译选项
include_directories("/root/package/mysqlcnn/include/jdbc")
#include_directories("/usr/local/include")
#include_directories("/root/package/jsoncpp-master/include")
include_directories("common/ConcurrentQueue.hpp")
#include_directories("common/nlp/nlp.h")

 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -Wno-unused-variable -pthread -lmysqlcppconn -lstdc++ -lcurl -lcrypto -lmsc -lrt")
link_directories("/usr/local/lib")
link_directories("/usr/local/lib64")
link_directories("/root/package/mysqlcnn/lib64")
add_executable(
    txbeta main.cpp
    esl/esl.c
    esl/esl_json.c
    esl/esl_threadmutex.c
    esl/esl_buffer.c
    esl/esl_event.c
    esl/esl_config.c
    process_event.cpp
    common/speech/common.c
    common/speech/token.cpp
    common/speech/ttscurl.c
    common/codeHelper.cpp
    common/structdef.cpp
    common/DBOperator.cpp
    database/config/inirw.cpp
    database/dbPool.cpp
    

   )
