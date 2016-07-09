Copyright 2016 Gregory Bryant

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
/***********************************************************************/


#pragma once
#define BOOST_ALL_NO_LIB

#include <string>
#include <iostream>
#include <curl/curl.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/xpath.h>
#include <vector>
#include <thread>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

using std::vector;
using std::string;
using std::thread;
using boost::smatch;
using boost::regex;
using std::cout;
using std::endl;
using boost::sregex_iterator;
using boost::sregex_token_iterator;

class WebPage
{
public:
	WebPage(string url);
	WebPage();
	void setURL(string url);
	~WebPage();
	int fetchAndProcess();
	int fetch();
	static size_t writeCallback(char* buf, size_t size, size_t nmemb, void* data);
	int saveHTML(string path);
	int saveData(string path);
	int evaluateXPath(string xpath);
	void printStringList(vector<string>* strList);
	void printStringListWithBaseURL(vector<string>* strList);

	vector<string> get_hrefs_text_from_a();
	vector<string> get_links_from_a_href();
	string get_link_from_a_href();
	vector<string> get_linktexts_from_a_href();
	vector<string> get_contents_from_children();
	string get_content_from_child();
	string get_content_from_2nd_childs_child();
	vector<string> get_contents();
	string get_content();
	void get_content(string &str);




	string evaluateRegex_first(string str);
	string evaluateRegex_first(string str, int pos);

	vector<string> evaluateRegex(string str);
	vector<string> evaluateRegex(string str, int pos);


	string evaluateRegexURL_first(string str);
	string evaluateRegexURL_first(string str, int pos);

	vector<string> evaluateRegexURL(string str);
	vector<string> evaluateRegexURL(string str, int pos);

	string evaluateRegexStr_first(string str, string theData);
	string evaluateRegexStr_first(string str, string theData, int pos);

	string evaluateRegexOnXPath_first(string exp, string xpath);
	string evaluateRegexOnXPath_first(string exp, string xpath, int pos);

	vector<string> evaluateRegexOnXPath(string exp, string xpath);
	vector<string> evaluateRegexOnXPath(string exp, string xpath, int pos);

	void reset();

	void initCURL();

	int process();
	int process(string altData);



	CURL* curl;
	string url;
	string baseURL;
	string data;
	htmlDocPtr htmlDoc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	thread *threadPtr;
};

