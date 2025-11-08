#include "kv_store.hpp"
#include <libpq-fe.h>

KVStore::KVStore(DbPool& pool, size_t cache_size)
  : pool_(pool), cache_(cache_size) {}

bool KVStore::put(const std::string& key, const std::string& value) {
  PGconn* c = pool_.acquire();
  const char* p[2] = { key.c_str(), value.c_str() };
  PGresult* r = PQexecParams(c,
    "INSERT INTO kv(key,value) VALUES($1,$2) "
    "ON CONFLICT(key) DO UPDATE SET value=EXCLUDED.value",
    2, nullptr, p, nullptr, nullptr, 0);

  bool ok = PQresultStatus(r) == PGRES_COMMAND_OK;
  PQclear(r);
  pool_.release(c);

  if (ok) cache_.put(key, value);
  return ok;
}

std::optional<std::string> KVStore::get(const std::string& key) {
  if (auto v = cache_.get(key)) return v;

  PGconn* c = pool_.acquire();
  const char* p[1] = { key.c_str() };
  PGresult* r = PQexecParams(c,
    "SELECT value FROM kv WHERE key=$1",
    1, nullptr, p, nullptr, nullptr, 0);

  if (!r || PQresultStatus(r) != PGRES_TUPLES_OK || PQntuples(r) == 0) {
    PQclear(r);
    pool_.release(c);
    return std::nullopt;
  }

  std::string result = PQgetvalue(r, 0, 0);
  PQclear(r);
  pool_.release(c);

  cache_.put(key, result);
  return result;
}

bool KVStore::erase(const std::string& key) {
  PGconn* c = pool_.acquire();
  const char* p[1] = { key.c_str() };
  PGresult* r = PQexecParams(c,
    "DELETE FROM kv WHERE key=$1",
    1, nullptr, p, nullptr, nullptr, 0);

  bool ok = PQresultStatus(r) == PGRES_COMMAND_OK;
  PQclear(r);
  pool_.release(c);

  if (ok) cache_.erase(key);
  return ok;
}
