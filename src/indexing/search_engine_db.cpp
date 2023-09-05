#include "search_engine_db.hpp"
#include "database_err.hpp"
#include <iostream>

std::atomic<int> search_engine_db::word_id = { 0 };
std::atomic<int> search_engine_db::page_id = { 0 };
bool search_engine_db::ready = false;

search_engine_db::search_engine_db() {
	int res = sqlite3_open_v2("search_engine.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr);
	if (res != SQLITE_OK && res != SQLITE_DONE) {
		throw database_err(database_err::CONNECTING_ERR);
	}
	std::cout << "dobro otvorena veza" << std::endl;
}

sqlite3* search_engine_db::setup_db() {
	if (!ready) {
		ready = true;

		sqlite3* db;
		int res = sqlite3_open_v2("search_engine.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
		if (res != SQLITE_OK && res != SQLITE_DONE) {
			throw database_err(database_err::CONNECTING_ERR);
		}

		const char* words_table = "DROP TABLE IF EXISTS words;\
							   CREATE TABLE words(\
									id INTEGER PRIMARY KEY,\
									word TEXT NOT NULL UNIQUE,\
									idf REAL NOT NULL DEFAULT 0);";
		res = sqlite3_exec(db, words_table, nullptr, nullptr, nullptr);
		if (res != SQLITE_OK && res != SQLITE_DONE) {
			throw database_err(database_err::CREATE_TABLE_ERR);
		}

		const char* pages_table = "DROP TABLE IF EXISTS pages;\
							   CREATE TABLE pages(\
								   id INTEGER PRIMARY KEY,\
								   url TEXT NOT NULL UNIQUE);";
		res = sqlite3_exec(db, pages_table, nullptr, nullptr, nullptr);
		if (res != SQLITE_OK && res != SQLITE_DONE) {
			throw database_err(database_err::CREATE_TABLE_ERR);
		}

		const char* tfs_table = "DROP TABLE IF EXISTS tfs;\
						    CREATE TABLE tfs(\
								  word_id INTEGER NOT NULL,\
								  page_id INTEGER NOT NULL,\
								  tf REAL NOT NULL,\
								  FOREIGN KEY(word_id) REFERENCES words(id),\
								  FOREIGN KEY(page_id) REFERENCES pages(id));";
		res = sqlite3_exec(db, tfs_table, nullptr, nullptr, nullptr);
		if (res != SQLITE_OK && res != SQLITE_DONE) {
			throw database_err(database_err::CREATE_TABLE_ERR);
		}
		sqlite3_close_v2(db);
		return db;
	}
	return nullptr;
}

search_engine_db::~search_engine_db() {
	int res =  sqlite3_close_v2(db);
	if (res != SQLITE_OK && res != SQLITE_DONE) {
		std::cerr << "Error while closing connection";
		exit(-1);
	}
	std::cout << "dobro zatvorena veza" << std::endl;
}

std::pair<int, int> search_engine_db::insert_word_url_tf(const std::string& word, const std::string& url, double tf, int add_word, int add_page) {
	if (!add_page) {
		add_page = page_id.load();
		++page_id;
		execute("INSERT INTO pages (id, url) VALUES (?, ?)", "is", add_page, url.c_str());
	}
	if (!add_word) {
		add_word = word_id.load();
		++word_id;
		execute("INSERT INTO words (id, word) VALUES (?, ?)", "is", add_word, word.c_str());
	}
	execute("UPDATE words SET idf = idf + 1 WHERE id = ?", "i", add_word);
	execute("INSERT INTO tfs (word_id, page_id, tf) VALUES (?, ?, ?)", "iid", add_word, add_page, tf);
	return { add_word, add_page };
}

int search_engine_db::execute(const char* query, const char* types, ...)
{
	va_list args;
	va_start(args, types);

	sqlite3_stmt* stmt;
	int res;
	while ((res = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr)) == SQLITE_BUSY);
	if (res != SQLITE_OK && res != SQLITE_DONE) {
		// error TODO;
		std::cerr << "Error while preparing";
		exit(-1);
	}

	int ind = 1;
	while (*types != '\0') {
		if (*types == 'i') {
			int value = va_arg(args, int);
			while ((res = sqlite3_bind_int(stmt, ind, value)) == SQLITE_BUSY);
			if (res != SQLITE_OK && res != SQLITE_DONE) {
				// error TODO
				std::cerr << "Error while binding";
				exit(-1);
			}
		}
		else if (*types == 's') {
			std::string value = va_arg(args, const char*);
			while ((res = sqlite3_bind_text(stmt, ind, value.c_str(), value.size(), nullptr)) == SQLITE_BUSY);
			if (res != SQLITE_OK && res != SQLITE_DONE) {
				// error TODO
				std::cerr << "Error while binding";
				exit(-1);
			}
		}
		else if (*types == 'd') {
			double value = va_arg(args, double);
			while ((res = sqlite3_bind_double(stmt, ind, value)) == SQLITE_BUSY);
			if (res != SQLITE_OK && res != SQLITE_DONE) {
				// error TODO
				std::cerr << "Error while binding";
				exit(-1);
			}
		}
		++ind;
		++types;
	}
	while ((res = sqlite3_step(stmt)) == SQLITE_BUSY);
	if (res != SQLITE_OK && res != SQLITE_DONE) {
		// error TODO
		std::cerr << "Error while executing query";
		exit(-1);
	}
	
	while ((res = sqlite3_finalize(stmt)) == SQLITE_BUSY);
	if (res != SQLITE_OK && res != SQLITE_DONE) {
		// error TODO
		std::cerr << "Error while finalising query";
		exit(-1);
	}
	va_end(args);
	return 0;
}
