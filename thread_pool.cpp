#include "thread_pool.hpp"
#include <iostream>

DbPool::DbPool(const std::string& conninfo, int size) {
  for (int i = 0; i < size; i++) {
    PGconn* c = PQconnectdb(conninfo.c_str());
    if (!c || PQstatus(c) != CONNECTION_OK) {
      throw std::runtime_error("PG connect failed: " + std::string(PQerrorMessage(c)));
    }
    PGresult* r = PQexec(c, "SET synchronous_commit = OFF"); // Basically we set sync off for the speed
    PQclear(r);
    idle_.push(c);
  }
  std::cerr << "[DB] Connection pool initialized (" << size << " connections)\n";
}

DbPool::~DbPool() {
  while (!idle_.empty()) { PQfinish(idle_.front()); idle_.pop(); }
}

PGconn* DbPool::acquire() { //acquire the connection if the we have an extra connection
  std::unique_lock<std::mutex> lk(mu_);
  cv_.wait(lk, [&]{ return !idle_.empty(); });
  PGconn* c = idle_.front();
  idle_.pop();
  return c;
}

void DbPool::release(PGconn* c) { //release the connection and wake up waiting request if one
  std::lock_guard<std::mutex> g(mu_);
  idle_.push(c);
  cv_.notify_one();
}
