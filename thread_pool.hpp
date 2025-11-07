#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <stdexcept>
#include <libpq-fe.h>

class Thread_pool{
    public:
        Thread_pool(const std::string &connection_info, int size);
        ~Thread_pool();

        PGconn* acquire();
        void release_conn(PGconn* conn);
    private:
        std::mutex mu;
        std::condition_variable cd;
        std::queue<PGconn*> idle_;
        int total_{0};
};