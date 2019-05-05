#include <iostream>
#include <stdexcept>
#include <exception>
#include <stdio.h>
#include "dbPool.h"

using namespace std;
using namespace sql;

pthread_mutex_t DBPool::lock = PTHREAD_MUTEX_INITIALIZER;

//Singleton: get the single object
DBPool *DBPool::instance_ = new DBPool();

DBPool *DBPool::GetInstance()
{
    return instance_;
}

void DBPool::initPool(std::string url_, std::string user_, std::string password_, int maxSize_)
{
    this->user = user_;
    this->password = password_;
    this->url = url_;
    this->maxSize = maxSize_;
    this->curSize = 0;
    try
    {
        this->driver = sql::mysql::get_driver_instance();
    }
    catch (sql::SQLException &e)
    {
        printf("Get sql driver failed,%s\n",e.what());
    }
    catch (std::runtime_error &e)
    {
        printf("Run error %s\n",e.what());
    }

    this->InitConnection(maxSize / 2);
}

//init conn pool
void DBPool::InitConnection(int initSize)
{
    Connection *conn;
    pthread_mutex_lock(&lock);
    for (int i = 0; i < initSize; i++)
    {
        conn = this->CreateConnection();

        if (conn)
        {
            connList.push_back(conn);
            ++(this->curSize);
        }
        else
        {
            printf("create conn error\n");
        }
    }
    pthread_mutex_unlock(&lock);
}

Connection *DBPool::CreateConnection()
{
    Connection *conn;
    try
    {
        // conn = driver->connect(url, user, password); //create a conn
        conn = driver->connect("tcp://127.0.0.1:3306", "root", ""); //create a conn

        return conn;
    }
    catch (sql::SQLException &e)
    {
        printf("uu%s\n", e.what());
        return NULL;
    }
    catch (std::runtime_error &e)
    {
        perror("run error");
        return NULL;
    }
}

Connection *DBPool::GetConnection()
{
    Connection *conn;

    pthread_mutex_lock(&lock);

    if (connList.size() > 0) //the pool have a conn
    {
        conn = connList.front();
        connList.pop_front(); //move the first conn
        if (conn->isClosed()) //if the conn is closed, delete it and recreate it
        {
            delete conn;
            conn = this->CreateConnection();
        }

        if (conn == NULL)
        {
            --curSize;
        }

        pthread_mutex_unlock(&lock);

        return conn;
    }
    else
    {
        if (curSize < maxSize) //the pool no conn
        {
            conn = this->CreateConnection();
            if (conn)
            {
                ++curSize;
                pthread_mutex_unlock(&lock);
                return conn;
            }
            else
            {
                pthread_mutex_unlock(&lock);
                return NULL;
            }
        }
        else //the conn count > maxSize
        {
            pthread_mutex_unlock(&lock);
            return NULL;
        }
    }
}

//put conn back to pool
void DBPool::ReleaseConnection(Connection *conn)
{
    if (conn)
    {
        pthread_mutex_lock(&lock);
        connList.push_back(conn);
        pthread_mutex_unlock(&lock);
    }
}

void DBPool::DestoryConnPool()
{
    list<Connection *>::iterator iter;
    pthread_mutex_lock(&lock);
    for (iter = connList.begin(); iter != connList.end(); ++iter)
    {
        this->DestoryConnection(*iter);
    }
    curSize = 0;
    connList.clear();
    pthread_mutex_unlock(&lock);
}

void DBPool::DestoryConnection(Connection *conn)
{
    if (conn)
    {
        try
        {
            conn->close();
        }
        catch (sql::SQLException &e)
        {
            perror(e.what());
        }
        catch (std::exception &e)
        {
            perror(e.what());
        }
        delete conn;
    }
}

DBPool::~DBPool()
{
    this->DestoryConnPool();
}
