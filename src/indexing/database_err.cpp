#include "database_err.hpp"
#include <iostream>

database_err::database_err(database_err::cause err_cause): err_cause(err_cause) {
}

void database_err::print_err() {
	if (err_cause == database_err::CONNECTING_ERR) {
		std::cerr << "Failed to connect to database";
	}
	else if (err_cause == database_err::CREATE_TABLE_ERR) {
		std::cerr << "Failed to create table";
	}
	else {
		std::cerr << "Unexpected error in database";
	}
}
