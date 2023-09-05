#include "indexing_thread.hpp"
#include "search_engine_db.hpp"
#include "html_element.hpp"
#include <deque>
#include <iostream>
#include <utility>

void parse_and_index(std::unordered_map<std::string, int>& words,
					std::shared_mutex& words_mutex,
					std::unordered_set<std::string>& links_to_process,
					std::unordered_set<std::string>& processed_links,
					std::mutex& links_mutex,
					std::counting_semaphore<500'000>& links_sem,
					std::deque<std::string>& html,
					std::deque<std::string>& html_url,
					std::counting_semaphore<1000>& html_sem,
					std::mutex& html_mutex) {
	
	search_engine_db db;

	while (true) {
		html_sem.acquire();
		html_mutex.lock();
		if (html.empty()) {
			html_mutex.unlock();
			std::cout << "prazno" << std::endl;
			break;
		}
		std::cout << "prosao" << std::endl;
		std::string html_page = std::move(html.back());
		std::string html_page_url = std::move(html_url.back());
		html.pop_back();
		html_url.pop_back();
		html_mutex.unlock();
		
		html_element parser(html_page, 0, nullptr);
		parser.parse();

		std::cout << "isparsira" << std::endl;
		
		int add_page = -1;
		for (auto &x : *parser.word_freq) {
			bool add_word = 0;
			words_mutex.lock_shared();
			add_word = words[x.first];
			words_mutex.unlock_shared();
			if (!add_word) {
				words_mutex.lock();
				add_word = words[x.first];
				if (add_word)
					words_mutex.unlock();
			}
			std::pair<int, int> ind = db.insert_word_url_tf(x.first, html_page_url, ((double)x.second) / parser.words_count, add_word, add_page);
			if (!add_word) {
				words[x.first] = ind.first;
				words_mutex.unlock();
			}
			add_page = ind.second;
		}

		std::cout << "dodao reci" << std::endl;
		
		links_mutex.lock();
		for (auto& x : *parser.links) {
			if (processed_links.find(x) == processed_links.end())
				links_to_process.insert(x);
			links_sem.release();
		}
		links_mutex.unlock();
		std::cout << "krug" << std::endl;
	}
}
