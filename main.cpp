#include <iostream>
#include "httplib.h"
#include "kv_store.hpp"
#include "thread_pool.hpp"
using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] 
                  << " <connection_string> <db_pool_size> <thread_pool_size>\n";
        return 1;
    }

    std::string conninfo = argv[1];
    int db_pool_size = std::stoi(argv[2]);
    int http_threads = std::stoi(argv[3]);

    Thread_pool pool(conninfo, db_pool_size);
    {
        PGconn* c = pool.acquire();
        PGresult* r = PQexec(c,
            "CREATE TABLE IF NOT EXISTS kv ("
            " key TEXT PRIMARY KEY,"
            " value TEXT );"
        );
        if (PQresultStatus(r) != PGRES_COMMAND_OK) {
            std::cerr << "[DB ERROR] Failed to create kv table: "
                      << PQerrorMessage(c) << std::endl;
            PQclear(r);
            return 1;
        }
        PQclear(r);
        pool.release_conn(c);
        std::cout << "[DB] kv table verified/created.\n";
    }
    kv_store kv(pool, 100000); 

    httplib::Server svr;

  
    svr.new_task_queue = [http_threads] {
        return new httplib::ThreadPool(http_threads);
    };


    svr.Put(R"(/kv/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string key = req.matches[1];
        if (kv.put(key, req.body)) res.status = 200;
        else { res.status = 500; res.set_content("DB error", "text/plain"); }
    });

   
    svr.Get(R"(/kv/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string key = req.matches[1];
        auto v = kv.get(key);
        if (v) res.set_content(*v, "text/plain");
        else res.status = 404;
    });

    svr.Delete(R"(/kv/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string key = req.matches[1];
        if (kv.erase(key)) res.status = 200;
        else res.status = 404;
    });

    std::cout << "[Server] Running with " << http_threads 
              << " HTTP worker threads\n";

    svr.listen("0.0.0.0", 8080);
    return 0;
}
