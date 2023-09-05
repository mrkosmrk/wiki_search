#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <unordered_set>
#include <mutex>
#include <semaphore>
#include <deque>
#include <shared_mutex>
#include <unordered_map>

class html_element {
public:
	html_element(std::string& html, unsigned long long index, html_element* parent);

	void parse();


private:
	bool is_closing_tag(unsigned long long postition);
	std::pair<unsigned long long, unsigned long long > find_url_in_a_tag(unsigned long long position);

	std::string& html;
	unsigned long long index;

	std::string text;
	std::string tag;
	std::unordered_set<std::string>* links;
	unsigned long long lenght;
	html_element* parent;
	unsigned long long words_count;
	std::unordered_map<std::string, unsigned>* word_freq;

	friend void parse_and_index(std::unordered_map<std::string, int>& words,
								std::shared_mutex& words_mutex,
								std::unordered_set<std::string>& links_to_process,
								std::unordered_set<std::string>& processed_links,
								std::mutex& links_mutex,
								std::counting_semaphore<500'000>& links_sem,
								std::deque<std::string>& html,
								std::deque<std::string>& html_url,
								std::counting_semaphore<1000>& html_sem,
								std::mutex& html_mutex);
};
