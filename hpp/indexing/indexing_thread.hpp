#include <unordered_set>
#include <string>
#include <mutex>
#include <semaphore>
#include <deque>
#include <shared_mutex>
#include <unordered_map>

void parse_and_index(std::unordered_map<std::string, int>& words,
					std::shared_mutex& words_mutex,
					std::unordered_set<std::string>& links_to_process,
					std::unordered_set<std::string>& processed_links,
					std::mutex& links_mutex, 
					std::counting_semaphore<500'000>& links_sem, 
					std::deque<std::string>& html,
					std::deque<std::string>& html_url,
					std::counting_semaphore<1000>& html_sem,
					std::mutex& html_mutex);