#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <unistd.h>
#include "httplib.h"

using namespace std;

atomic <bool> stop_flag(false);

struct Stats{
    long long succ=0;
    long long fail=0;
    long long tot_latency=0;
};

/*struct Stats {
    long long success = 0;
    long long fail = 0;
    long long total_latency_ns = 0;
};*/

struct Mode {
    double get_r;
    double put_r;
    double del_r;
};

vector<Mode> MODES = {
    {0.95, 0.05, 0.0},  // cache
    {0.10, 0.80, 0.10}, // db
    {0.70, 0.20, 0.10}  // mixed
};

void worker_thread(string host, int port, int mode, int think_ms, vector<string> &keys, vector<string> &values, Stats &stats){
    httplib::Client cli(host, port);
    cli.set_read_timeout(5, 0); //wait for 5 seconds
    default_random_engine gen(time(NULL));

    uniform_real_distribution<double> dist(0.0, 1.0);
    uniform_int_distribution<int> pick(0, keys.size() - 1);

    while(!stop_flag.load())
    {
        int idx = pick(gen);
        string key = keys[idx];
        string value = values[idx];
        string endpoint = "/kv/" + key;

        double r = dist(gen);
        auto t1 = chrono::high_resolution_clock::now();
        httplib::Result res;
        
        if(r < MODES[mode].get_r){
            res = cli.Get(endpoint.c_str());
        }
        else if(r < (MODES[mode].get_r + MODES[mode].put_r)){
            res = cli.Put(endpoint.c_str(), value, "text/plain");

        }
        else{
            res = cli.Delete(endpoint.c_str());
        }

        auto t2 = chrono::high_resolution_clock::now();

        long long latency = chrono::duration_cast<chrono::nanoseconds>(t2 - t1).count();
        stats.tot_latency += latency;

        if (res && res->status < 500)
            stats.succ++;
        else
            stats.fail++;

        // ---- Think Time ----
        if (think_ms > 0)
            usleep(think_ms * 1000);
     }






}

int main(int argc, char *argv[]){
    if (argc < 6) {
        cerr << "Usage: ./loadgen <host> <port> <threads> <duration> <mode:0/1/2>\n";
        return 1;
    }

    string host = argv[1]; //set params
    int port = atoi(argv[2]);
    int threads = atoi(argv[3]);
    int duration = atoi(argv[4]);
    int mode = atoi(argv[5]);
    int think_ms = argc >= 7 ? atoi(argv[6]) : 0;

    if (mode < 0 || mode > 2) mode = 2; //default 

    const int POOL = 5000;
    vector<string> keys, values;
    keys.reserve(POOL);
    values.reserve(POOL); //first storing the keys, values and then send it to server

    for (int i = 0; i < POOL; i++) {
        keys.push_back("key_" + to_string(i));
        values.push_back("val_" + to_string(i));
    }

    cout << "Running Load Test...\n";
    cout << "Mode: " << mode << "  Threads: " << threads << "  Duration: " << duration << " sec" << "  Think: " << think_ms << " ms\n\n";

    vector<Stats> stats(threads); //each threads measure stats
    vector<thread> workers;

    for (int i = 0; i < threads; i++) {
        workers.emplace_back(worker_thread, host, port, mode, think_ms,
                             ref(keys), ref(values), ref(stats[i]));
    }
    sleep(duration);

    stop_flag.store(true);

    for(auto &t :workers){
        t.join();
    }
    long long total_success = 0, total_fail = 0, total_lat = 0;
    for (auto &s : stats) {
        total_success += s.succ;
        total_fail += s.fail;
        total_lat += s.tot_latency;
    }

    long long total = total_success + total_fail;

    cout << "Total Requests: " << total << "\n";
    cout << "Success:        " << total_success << "\n";
    cout << "Failures:       " << total_fail << "\n";
    cout << "Throughput:     " << (total / (double)duration) << " req/s\n";
    cout << "Avg Latency:    " << (total > 0 ? (total_lat / 1e6 / total) : 0) << " ms\n";

    return 0;

    
}