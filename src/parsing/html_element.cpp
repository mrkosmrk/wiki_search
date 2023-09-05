#include "html_element.hpp"
#include "special_html_strings.hpp"
#include "invalid_parentheses.hpp"
#include <iostream>

html_element::html_element(std::string& html, unsigned long long index, html_element* parent)
	: html(html),
	index(index),
	parent(parent),
	lenght(0),
	words_count(0) {
	if (parent) {
		word_freq = parent->word_freq;
		links = parent->links;
	}
	else {
		word_freq = new std::unordered_map<std::string, unsigned>();
		links = new std::unordered_set<std::string>();
	}
}

void html_element::parse() {
	bool le_opening = false;
	bool ge_opening = false;
	bool le_closing = false;
	bool ge_closing = false;
	bool tag_curr_opening = false;
	bool self_closing_tag = false;
	std::string curr_word = "";

	for (unsigned long long curr_ind = index; curr_ind < html.size(); ++curr_ind) {
		++lenght;
		char curr_char = html[curr_ind];

		if (curr_char == '<') {
			if (curr_word != "") {
				++((*word_freq)[curr_word]);
				++words_count;
				curr_word = "";
			}
			if (tag == "script" && html[curr_ind + 1] != '/') {
				continue;
			}
			if (!le_opening) {
				le_opening = true;
				tag_curr_opening = true;
			}
			else if (!is_closing_tag(curr_ind)) {
				if (self_closing_tag) {
					le_opening = true;
					tag_curr_opening = true;
					self_closing_tag = false;
					ge_opening = false;
					le_closing = false;
					ge_closing = false;
					curr_word = "";
					continue;
				}
				html_element child = html_element(html, curr_ind, this);
				child.parse();
				lenght += child.lenght - 1;
				curr_ind += child.lenght - 1;
				words_count += child.words_count;
			}
			else {
				le_closing = true;
			}
		}
		else if (curr_char == '>') {
			if (tag == "!--") {
				if (html[curr_ind - 1] != '-' || html[curr_ind - 2] != '-') {
					continue;
				}
			}
			if (tag == "script" && !le_closing) {
				continue;
			}
			if (tag_curr_opening) {
				tag = curr_word;
				tag_curr_opening = false;
				if (std::find(self_closing.begin(), self_closing.end(), tag) != self_closing.end()) {
					self_closing_tag = true;
				}
				curr_word = "";
				if (tag == "a") {
					auto url = find_url_in_a_tag(curr_ind);
					if (url.first != 0) {
						links->insert(html.substr(url.first, url.second - url.first + 1));
					}
				}
			}
			if (le_closing) {
				ge_closing = true;
				if (!parent) {
					le_opening = ge_opening = le_closing = ge_closing = self_closing_tag = tag_curr_opening = false;
					curr_word = "";
					continue;
				}
				break;
			}
			else if (le_opening) {
				ge_opening = true;
			}
			else {
				throw invalid_parentheses();
			}
		}
		else if (tag_curr_opening) {
			if (curr_char == ' ' || curr_char == 9) {
				tag = curr_word;
				tag_curr_opening = false;
				if (std::find(self_closing.begin(), self_closing.end(), tag) != self_closing.end()) {
					self_closing_tag = true;
				}
				curr_word = "";
				if (tag == "a") {
					auto url = find_url_in_a_tag(curr_ind);
					if (url.first != 0) {
						links->insert(html.substr(url.first, url.second - url.first + 1));
					}
				}
			}
			else {
				curr_word.push_back(curr_char);
			}
		}
		else if (((ge_opening && !le_closing) || (self_closing_tag && ge_opening)) && tag != "script" && tag != "style" && tag != "!--") {
			if (curr_char == '&') {
				for (const std::string& sign : and_sign_strings) {
					size_t len = sign.size();
					if (curr_ind + len < html.size() && html.substr(curr_ind + 1, len) == sign) {
						curr_ind += len;
						if (curr_word == "")
							break;
						++((*word_freq)[curr_word]);
						++words_count;
						curr_word = "";
						break;
					}
				}
			}
			else if ((curr_char >= 'a' && curr_char <= 'z') || (curr_char >= 'A' && curr_char <= 'Z') || (curr_char >= '0' && curr_char <= '9') || curr_char == '-' || curr_char == '\\' || curr_char == '/' || curr_char == '\'') {
				char lower_curr_char = curr_char;
				if (curr_char >= 'A' && curr_char <= 'Z') {
					lower_curr_char = 'a' + (curr_char - 'A');
				}
				curr_word.push_back(lower_curr_char);
			}
			else {
				if (curr_word == "")
					continue;
				++((*word_freq)[curr_word]);
				++words_count;
				curr_word = "";
			}
		}
	}
}


bool html_element::is_closing_tag(unsigned long long postition) {
	if (html[postition + 1] == '/')
		return true;
	char prev_non_blank_char = '<';
	for (unsigned long long curr_ind = postition; html[curr_ind] != '>'; ++curr_ind) {
		if (html[curr_ind] != ' ' && html[curr_ind] != 9) { // 9 is tab in ascii
			prev_non_blank_char = html[curr_ind];
		}
	}
	return prev_non_blank_char == '/';
}

std::pair<unsigned long long, unsigned long long > html_element::find_url_in_a_tag(unsigned long long position)
{
	bool href_found = false;
	unsigned long long start, end;
	for (unsigned long long curr_ind = position; html[curr_ind] != '>' && curr_ind + 11 < html.size(); ++curr_ind) {
		if (!href_found) {
			if (html.substr(curr_ind, 12) == "href=\"/wiki/") {
				start = curr_ind + 6;
				end = curr_ind + 11;
				curr_ind += 11;
				href_found = true;
			}
		}
		else {
			if (html[curr_ind] == ':') { // doesn't link to wiki article
				return { 0, 0 };
			}
			if (html[curr_ind] == '"') {
				return { start, end };
			}
			else {
				end = curr_ind;
			}
		}
	}
	return { 0, 0 };
}
