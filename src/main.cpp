#define CURL_STATICLIB
#include <iostream>
#include <string>
#include <deque>
#include <fstream>
#include <mutex>
#include <semaphore>
#include <unordered_set>
#include <shared_mutex>
#include "curl/curl.h"
#include "sqlite/sqlite3.h"
#include "html_element.hpp"
#include "invalid_parentheses.hpp"
#include "search_engine_db.hpp"
#include "indexing_thread.hpp"


#ifdef _DEBUG
#pragma comment(lib, "curl/libcurl_a_debug.lib")
#else
#pragma comment(lib, "curl/libcurl_a.lib")
#endif

#define SQLITE_THREADSAFE 1


#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "advapi32.lib")

constexpr int thread_num = 2;
std::unordered_set<std::string> links_to_process;
std::unordered_set<std::string> processed_links;
std::unordered_map<std::string, int> words;
std::shared_mutex word_mutex;
std::mutex links_mutex;
std::counting_semaphore<500'000> links_sem(1);
std::deque<std::string> html;
std::deque<std::string> html_url;
std::counting_semaphore<1000> html_sem(0);
std::mutex html_mutex;
std::string curr_html;
int pages_count = 0;

std::thread threads[thread_num];


size_t convert_html_to_string(void* data, size_t size, size_t nmemb, void* clientp) {
    
    size_t realsize = size * nmemb;
    char* data_c = (char*)data;

    curr_html.append(data_c, realsize);

    return realsize;
}

int main(void)
{

    sqlite3* db = search_engine_db::setup_db();
    // catch(...) TODO

    // prepare links
    links_to_process.insert("/wiki/Chuck_McGill");

    for (int i = 0; i < thread_num; ++i) {
        threads[i] = std::thread(parse_and_index, 
                                std::ref(words),
                                std::ref(word_mutex),
                                std::ref(links_to_process), 
                                std::ref(processed_links), 
                                std::ref(links_mutex), 
                                std::ref(links_sem), 
                                std::ref(html), 
                                std::ref(html_url), 
                                std::ref(html_sem), 
                                std::ref(html_mutex));
    }

    while (true) {
        CURL* curl;
        CURLcode res;

        curl_global_init(CURL_GLOBAL_DEFAULT);

        std::string url;
        links_sem.acquire();
        links_mutex.lock();
        url = (*links_to_process.begin());
        links_to_process.erase(url);
        processed_links.insert(url);
        links_mutex.unlock();

        url = "https://en.wikipedia.org" + url;

        std::cout << url << std::endl;

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            /* cache the CA cert bundle in memory for a week */
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &convert_html_to_string);

            curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);

            /* Perform the request, res will get the return code */
            res = curl_easy_perform(curl);
            /* Check for errors */
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
                std::cerr << url << std::endl;
                exit(-1);
            }
            else {
                html_mutex.lock();
                html.push_back(curr_html);
                html_url.push_back(url);
                html_mutex.unlock();
                html_sem.release();
                ++pages_count;
                if (pages_count == 5) {
                    for (int i = 0; i < thread_num; ++i) {
                        html_sem.release();
                    }
                    break;
                }
                curr_html = "";
            }

            /* always cleanup */
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();
    }

    for (int i = 0; i < thread_num; ++i)
        threads[i].join();

    int res = sqlite3_close_v2(db);
    if (res != SQLITE_OK && res != SQLITE_DONE) {
        std::cerr << "Error while closing connection " << res << std::endl;
        exit(-1);
    }

    return 0;
}
