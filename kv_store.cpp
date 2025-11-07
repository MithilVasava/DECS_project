#include <libpq-fe.h>
#include "kv_store.hpp"
using namespace std;
kv_store::kv_store(Thread_pool &pool, size_t value) : pool_(pool), cache_(value) {}


bool kv_store::put(const string& key, const string& value){
    PGconn *conn = pool_.acquire();
    const char *params[2] = {key.c_str(), value.c_str()};
    PGresult *res = PQexecParams(conn, "INSERT INTO kv(key,value) VALUES($1,$2) " "ON CONFLICT(key) DO UPDATE SET value=EXCLUDED.value", 2, nullptr, params, nullptr, nullptr, 0);
    bool ok = (PQresultStatus(res)==PGRES_COMMAND_OK);
    PQclear(res);
    pool_.release_conn(conn);
    if (ok){
        cache_.put(key, value);
    }
    return ok;
}

optional<string> kv_store::get(const string& key){
    if(auto v = cache_.get(key)){
        return v;
    }
    PGconn *conn = pool_.acquire();
    const char *params[1] = {key.c_str()};
    PGresult *res = PQexecParams(conn, "SELECT value FROM kv WHERE key=$1", 1, nullptr, params, nullptr, nullptr, 0);
    optional<string> out;
    if(PQresultStatus(res)==PGRES_TUPLES_OK && PQntuples(res)==1){
        out = PQgetvalue(res, 0, 0);
        cache_.put(key, *out);
    }

    PQclear(res);
    pool_.release_conn(conn);

    return out;

}

bool kv_store::erase(const string &key){
    PGconn *conn = pool_.acquire();
    const char *params[1] = {key.c_str()};
    PGresult *res = PQexecParams(conn, "DELETE FROM kv WHERE key=$1", 1, nullptr, params, nullptr, nullptr, 0);
    bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    pool_.release_conn(conn);
    if(ok){
        cache_.erase(key);
    }

    return ok;
 }