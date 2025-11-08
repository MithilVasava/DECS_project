#include <iostream>
#include "httplib.h"
#include "thread_pool.hpp"
#include "kv_store.hpp"

int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: ./tenant_kv \"host=127.0.0.1 port=5432 dbname=kvdb user=postgres\" <db_pool> <threads>\n";
    return 1;
  }

  DbPool pool(argv[1], std::stoi(argv[2])); // 1.connection 2.size of db connections
  KVStore kv(pool, 10000); //poolsizes

  httplib::Server server;
  server.new_task_queue = [t = std::stoi(argv[3])] { return new httplib::ThreadPool(t); }; //create thread_pool

  server.Put(R"(/kv/(.*))", [&](const auto& req, auto& res){
    kv.put(req.matches[1], req.body); res.status = 200;
  });

  server.Get(R"(/kv/(.*))", [&](const auto& req, auto& res){
    auto v = kv.get(req.matches[1]);
    if (v) res.set_content(*v, "text/plain");
    else res.status = 404;
  });

  server.Delete(R"(/kv/(.*))", [&](const auto& req, auto& res){
    kv.erase(req.matches[1]); res.status = 200;
  });

  std::cout << "[Server] Running on :8080\n";
  server.listen("0.0.0.0", 8080);
}
