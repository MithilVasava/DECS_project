#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <libpq-fe.h>

class DbPool {
public:
  DbPool(const std::string& conninfo, int size);
  ~DbPool();
  PGconn* acquire();
  void release(PGconn* c);

private:
  std::mutex mu_;
  std::condition_variable cv_;
  std::queue<PGconn*> idle_;
};
