#include "invalid_parentheses.hpp"
#include <iostream>

invalid_parentheses::invalid_parentheses() {
}

void invalid_parentheses::print_error() {
	std::cout << "Right parenthesis doen't have its match.";
}
