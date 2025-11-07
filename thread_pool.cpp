#include "thread_pool.hpp"
#include <iostream>
using namespace std;

void check_connc(PGconn* c){
    if(!c || PQstatus(c)!= CONNECTION_OK){
        throw runtime_error(string("PG connect failed: ")+ (c?PQerrorMessage(c):"null"));
    }
}

Thread_pool::Thread_pool(const string &connection_info, int size){
    for(int i=0;i<size;i++){
        PGconn *con = PQconnectdb(connection_info.c_str());
        check_connc(con);
        
        PGresult *r = PQexec(con, "SET synchronous_commit TO OFF");
        PQclear(r);
        idle_.push(con);
        total_++;
    }
}

Thread_pool::~Thread_pool(){
    while(!idle_.empty()){
        PQfinish(idle_.front());
        idle_.pop();
    }
}

PGconn* Thread_pool::acquire(){
    unique_lock<mutex> lk(mu);
    cd.wait(lk, [&]{return !idle_.empty();});
    auto c = idle_.front();
    idle_.pop();
    return c;
}

void Thread_pool::release_conn(PGconn* conn){
    lock_guard<mutex> g(mu);
    idle_.push(conn);
    cd.notify_one();
}

