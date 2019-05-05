/*
    time: 2018.08.31
    author: mahongyu@infosun.cn
    notice: MySql Connector C++
*/

#ifndef _DB_POOL_H_
#define _DB_POOL_H_
 
#include <iostream>
#include <mysql_connection.h>  
#include <mysql_driver.h>  
#include <cppconn/exception.h>  
#include <cppconn/driver.h>  
#include <cppconn/connection.h>  
#include <cppconn/resultset.h>  
#include <cppconn/prepared_statement.h>  
#include <cppconn/statement.h>  
#include <pthread.h>  
#include <list>  
 
using namespace std;  
using namespace sql; 
 
class DBPool
{
public:
    // Singleton 
    static DBPool* GetInstance();
 
    //init pool
    void initPool(std::string url_, std::string user_, std::string password_, int maxSize_);
 
    //get a conn from pool
    Connection* GetConnection();
 
    //put the conn back to pool
    void ReleaseConnection(Connection *conn);

     ~DBPool();
 
private:
    DBPool(){}
 
    //init DB pool
    void InitConnection(int initSize);
    
    // create a connection
    Connection* CreateConnection(); 
 
    //destory connection
    void DestoryConnection(Connection *conn);
 
    //destory db pool
    void DestoryConnPool();
 
private:
    string user;
    string password;
    string url;
    int maxSize;
    int curSize;
 
    Driver*             driver;     //sql driver (the sql will free it)
    list<Connection*>   connList;   //create conn list
 
    //thread lock mutex
    static pthread_mutex_t lock;  
    static DBPool* instance_;   
};
 
#endif
