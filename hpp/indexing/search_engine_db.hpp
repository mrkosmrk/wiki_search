#include "sqlite/sqlite3.h"
#include <string>
#include <atomic>
#include <vector>

class search_engine_db {
public:

	search_engine_db();

	std::pair<int, int> insert_word_url_tf(const std::string& word, const std::string& url, double tf, int add_word, int add_url);

	static sqlite3* setup_db();

	~search_engine_db();

private:
	int execute(const char* query, const char* types, ...);

	sqlite3* db;
	static std::atomic<int> word_id, page_id;
	static bool ready;
};