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


#include "WebPage.h"
#include <fstream>

WebPage::WebPage(string url)
{
	this->url = url;
	baseURL = this->url.substr(0, this->url.find_first_of("/", 7));
	curl = curl_easy_init();
	initCURL();
	xpathCtx = 0;
	xpathObj = 0;
	htmlDoc = 0;
}

WebPage::WebPage()
{
	curl = curl_easy_init();
	xpathCtx = 0;
	xpathObj = 0;
	htmlDoc = 0;
}

void WebPage::initCURL()
{
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 6.3; WOW64; rv:38.0) Gecko/20100101 Firefox/38.0");
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
}

void WebPage::setURL(string url)
{
	this->url = url;
	baseURL = this->url.substr(0, this->url.find_first_of("/", 7));
	initCURL();
}

//this helps with reuse before destruction
void WebPage::reset()
{
	if (xpathObj) { xmlXPathFreeObject(xpathObj); xpathObj = 0; }
	if (xpathCtx) { xmlXPathFreeContext(xpathCtx); xpathCtx = 0; }
	if (htmlDoc) { xmlFreeDoc(htmlDoc); htmlDoc = 0; }


	data.clear();
}


WebPage::~WebPage()
{
	//cout << "#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#WebPage::~WebPage()" << endl;
	if (curl){ curl_easy_cleanup(curl); }
	if (xpathObj) { xmlXPathFreeObject(xpathObj); }
	if (xpathCtx) { xmlXPathFreeContext(xpathCtx); }
	if (htmlDoc) { xmlFreeDoc(htmlDoc); }
}

int WebPage::fetchAndProcess()
{
	int rVal;
	rVal = fetch();
	rVal = process();
	return rVal;
}

int WebPage::process()
{
	htmlDoc = htmlReadDoc((const xmlChar*)data.c_str(), baseURL.c_str(), 0, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
	if (htmlDoc == NULL)
	{
		cout << "Error: unable to create htmlDocPtr" << endl;
		return(-1);
	}
	xpathCtx = xmlXPathNewContext(htmlDoc);
	if (xpathCtx == NULL)
	{
		cout << "Error: unable to create new XPath context" << endl;
		return(-1);
	}
	return 0;
}

int WebPage::process(string altData)
{
	htmlDoc = htmlReadDoc((const xmlChar*)altData.c_str(), baseURL.c_str(), 0, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
	if (htmlDoc == NULL)
	{
		cout << "Error: unable to create htmlDocPtr" << endl;
		return(-1);
	}
	xpathCtx = xmlXPathNewContext(htmlDoc);
	if (xpathCtx == NULL)
	{
		cout << "Error: unable to create new XPath context" << endl;
		return(-1);
	}
	return 0;
}

int WebPage::fetch()
{
	/*
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 6.3; WOW64; rv:37.0) Gecko/20100101 Firefox/37.0");
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	//curl_easy_setopt(curl, CURLOPT_USERPWD, "username:password");
	*/
	CURLcode err = curl_easy_perform(curl);
	//int rVal = processXML();
	return 0;
}

size_t WebPage::writeCallback(char* buf, size_t size, size_t nmemb, void* data)
{
	string *dataString = (string*)data;
	for (int c = 0; c<size*nmemb; c++)
	{
		dataString->push_back(buf[c]);
	}
	return size*nmemb;
}

int WebPage::saveData(string path)
{
	std::ofstream myfile;
	myfile.open(path, std::ios::trunc);
	myfile << data;
	myfile.close();

	myfile.open(path+".url.txt", std::ios::trunc);
	myfile << url;
	myfile.close();

	return 0;
}

int WebPage::saveHTML(string path)
{
	htmlSaveFile(path.c_str(), htmlDoc);
	return 0;
}

string WebPage::evaluateRegex_first(string str)
{
	regex rankREGEX(str);
	smatch match;
	regex_search(data, match, rankREGEX);
	//std::regex_search()
	return match.str();

}

string WebPage::evaluateRegex_first(string str, int pos)
{
	regex rankREGEX(str);
	smatch match;
	regex_search(data, match, rankREGEX);
	if (pos < match.size())
		return match[pos].str();
	else
		return "";
}

vector<string> WebPage::evaluateRegex(string str)
{
	vector<string> rVal;
	regex rankREGEX(str);
	sregex_iterator  it(data.begin(), data.end(), rankREGEX);
	sregex_iterator  it_end;
	while (it != it_end)
	{
		rVal.push_back((*it).str());
		it++;
	}
	return rVal;
}

vector<string> WebPage::evaluateRegex(string str, int pos)
{
	vector<string> rVal;
	regex rankREGEX(str);
	sregex_token_iterator it(data.begin(), data.end(), rankREGEX, pos);
	sregex_token_iterator it_end;
	while (it != it_end)
	{
		rVal.push_back((*it).str());
		it++;
	}
	return rVal;
}



string WebPage::evaluateRegexURL_first(string str)
{
	regex rankREGEX(str);
	smatch match;
	regex_search(url, match, rankREGEX);
	return match.str();
}

string WebPage::evaluateRegexURL_first(string str, int pos)
{
	regex rankREGEX(str);
	smatch match;
	regex_search(url, match, rankREGEX);
	if (pos < match.size())
		return match[pos].str();
	else
		return "";
}

vector<string> WebPage::evaluateRegexURL(string str)
{
	vector<string> rVal;
	regex rankREGEX(str);
	sregex_iterator  it(url.begin(), url.end(), rankREGEX);
	sregex_iterator  it_end;
	while (it != it_end)
	{
		rVal.push_back((*it).str());
		it++;
	}
	return rVal;
}

vector<string> WebPage::evaluateRegexURL(string str, int pos)
{
	vector<string> rVal;
	regex rankREGEX(str);
	sregex_token_iterator it(url.begin(), url.end(), rankREGEX, pos);
	sregex_token_iterator it_end;
	while (it != it_end)
	{
		rVal.push_back((*it).str());
		it++;
	}
	return rVal;
}

string WebPage::evaluateRegexStr_first(string str, string theData)
{
	regex rankREGEX(str);
	smatch match;
	bool result = regex_search(theData, match, rankREGEX);
	if(result)
	{
		if (match.size())
			return match.str();
		else
			return "";
	}
	else
		return "";
	
}

string WebPage::evaluateRegexStr_first(string str, string theData, int pos)
{
	regex rankREGEX(str);
	smatch match;
	bool result = regex_search(theData, match, rankREGEX);
	if(result)
	{
		if (pos < match.size())
			return match[pos].str();
		else
			return "";
	}
	else
		return "";
}

string WebPage::evaluateRegexOnXPath_first(string exp, string xpath)
{
	evaluateXPath(xpath);
	if (xpathObj)
	{
		string flattened;
		for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++)
		{
			xmlChar *strRep = xmlXPathCastNodeToString(xpathObj->nodesetval->nodeTab[i]);
			string nodeStr((char*)strRep);
			if (!nodeStr.empty())
				flattened += nodeStr + string("\n");
			xmlFree(strRep);
		}
		regex rankREGEX(exp);
		smatch match;
		regex_search(flattened, match, rankREGEX);
		return match.str();
	}
	else
		return "";
}

vector<string> WebPage::evaluateRegexOnXPath(string exp, string xpath, int pos)
{
	vector<string> rVal;
	evaluateXPath(xpath);
	if (xpathObj)
	{
		string flattened;
		for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++)
		{
			xmlChar *strRep = xmlXPathCastNodeToString(xpathObj->nodesetval->nodeTab[i]);
			string nodeStr((char*)strRep);
			if (!nodeStr.empty())
				flattened += nodeStr + string("\n");
			xmlFree(strRep);
		}
		regex rankREGEX(exp);
		sregex_token_iterator it(flattened.begin(), flattened.end(), rankREGEX, pos);
		sregex_token_iterator it_end;
		while (it != it_end)
		{
			rVal.push_back((*it).str());
			it++;
		}
		return rVal;
	}
	else
		return rVal;
}

vector<string> WebPage::evaluateRegexOnXPath(string exp, string xpath)
{
	vector<string> rVal;
	evaluateXPath(xpath);
	if (xpathObj)
	{
		string flattened;
		for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++)
		{
			xmlChar *strRep = xmlXPathCastNodeToString(xpathObj->nodesetval->nodeTab[i]);
			string nodeStr((char*)strRep);
			if (!nodeStr.empty())
				flattened += nodeStr + string("\n");
			xmlFree(strRep);
		}
		regex rankREGEX(exp);
		sregex_iterator  it(flattened.begin(), flattened.end(), rankREGEX);
		sregex_iterator  it_end;
		while (it != it_end)
		{
			rVal.push_back((*it).str());
			it++;
		}
		return rVal;
	}
	else
		return rVal;
}

string WebPage::evaluateRegexOnXPath_first(string exp, string xpath, int pos)
{
	evaluateXPath(xpath);
	if (xpathObj)
	{
		string flattened;
		for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++)
		{
			xmlChar *strRep = xmlXPathCastNodeToString(xpathObj->nodesetval->nodeTab[i]);
			string nodeStr((char*)strRep);
			if (!nodeStr.empty())
				flattened += nodeStr + string("\n");
			xmlFree(strRep);
		}
		regex rankREGEX(exp);
		smatch match;
		regex_search(flattened, match, rankREGEX);
		if (pos < match.size())
			return match[pos].str();
		else
			return "";
	}
	else
		return "";
}



int WebPage::evaluateXPath(string xpath)
{
	if (xpathObj) { xmlXPathFreeObject(xpathObj); }
	xpathObj = xmlXPathEvalExpression((const xmlChar*)xpath.c_str(), xpathCtx);
	if (xpathObj == NULL)
	{
		cout << "Error: unable to evaluate xpath expression: " << xpath << endl;
		//xmlXPathFreeContext(xpathCtx);
		return(-1);
	}

	if (xpathObj->nodesetval)
		return xpathObj->nodesetval->nodeNr;
	else
		return 0;
}

void WebPage::printStringList(vector<string>* strList)
{
	for (int i = 0; i<strList->size(); i++)
	{
		cout << (*strList)[i] << endl;
	}
}

void WebPage::printStringListWithBaseURL(vector<string>* strList)
{
	for (int i = 0; i<strList->size(); i++)
	{
		cout << baseURL << (*strList)[i] << endl;
	}
}

vector<string> WebPage::get_hrefs_text_from_a()
{
	xmlNodePtr currNode;
	xmlAttrPtr currAttr;
	vector<string> strList;
	for (int i = 0; i<xpathObj->nodesetval->nodeNr; i++)
	{
		currAttr = xpathObj->nodesetval->nodeTab[i]->properties;
		while (currAttr)
		{
			if (!xmlStrcmp(currAttr->name, (const xmlChar*)"href"))
			{
				//cout << currAttr->name << endl;
				//cout << currAttr->children->content << endl << endl;
				strList.push_back(string((const char*)currAttr->children->content));
			}

			currAttr = currAttr->next;
		}
	}
	return strList;
}

string WebPage::get_link_from_a_href()
{
	xmlNodePtr currNode;
	xmlAttrPtr currAttr;
	string str;
	if (xpathObj->nodesetval->nodeTab[0]->children)
		if (xpathObj->nodesetval->nodeTab[0]->children->content)
			str.assign((const char*)xpathObj->nodesetval->nodeTab[0]->children->content);
	return str;
}

vector<string> WebPage::get_links_from_a_href()
{
	xmlNodePtr currNode;
	xmlAttrPtr currAttr;
	vector<string> strList;
	for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++)
	{
		if (xpathObj->nodesetval->nodeTab[i]->children)
			if (xpathObj->nodesetval->nodeTab[i]->children->content)
				strList.push_back(string((const char*)xpathObj->nodesetval->nodeTab[i]->children->content));
	}
	return strList;
}

string WebPage::get_content_from_2nd_childs_child()
{
	if (xpathObj)
		if (xpathObj->nodesetval)
			if (xpathObj->nodesetval->nodeTab[0])
				if (xpathObj->nodesetval->nodeTab[0]->children)
					if (xpathObj->nodesetval->nodeTab[0]->children->next)
						if (xpathObj->nodesetval->nodeTab[0]->children->next->children)
							return string((const char*)xpathObj->nodesetval->nodeTab[0]->children->next->children->content);
	return "";
}

string WebPage::get_content_from_child()
{
	xmlNodePtr currNode;
	xmlAttrPtr currAttr;
	vector<string> strList;
	if (xpathObj->nodesetval)
		if (xpathObj->nodesetval->nodeTab[0])
			if (xpathObj->nodesetval->nodeTab[0]->children)
				if (xpathObj->nodesetval->nodeTab[0]->children->content)
					return string((const char*)xpathObj->nodesetval->nodeTab[0]->children->content);
	return "";
}

vector<string> WebPage::get_contents_from_children()
{
	xmlNodePtr currNode;
	xmlAttrPtr currAttr;
	vector<string> strList;
	if (xpathObj->nodesetval)
	{
		for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++)
		{
			if (xpathObj->nodesetval->nodeTab[i]->children)
				if (xpathObj->nodesetval->nodeTab[i]->children->content)
					strList.push_back(string((const char*)xpathObj->nodesetval->nodeTab[i]->children->content));
		}
	}
	return strList;
}

string WebPage::get_content()
{
	string str;
	if (xpathObj->nodesetval)
		if (xpathObj->nodesetval->nodeTab[0]->content)
			str.assign((const char*)xpathObj->nodesetval->nodeTab[0]->content);
	return str;
}

void WebPage::get_content(string &str)
{
	if (xpathObj->nodesetval->nodeTab[0]->content)
		str.assign((const char*)xpathObj->nodesetval->nodeTab[0]->content);
}

vector<string> WebPage::get_contents()
{
	vector<string> strList;
	if (xpathObj->nodesetval)
	{
		for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++)
		{
			if (xpathObj->nodesetval->nodeTab[i]->content)
				strList.push_back(string((const char*)xpathObj->nodesetval->nodeTab[i]->content));
		}
	}
	return strList;
}

vector<string> WebPage::get_linktexts_from_a_href()
{
	xmlNodePtr currNode;
	xmlAttrPtr currAttr;
	vector<string> strList;
	for (int i = 0; i < xpathObj->nodesetval->nodeNr; i++)
	{
		if (xpathObj->nodesetval->nodeTab[i]->parent)
		{
			currNode = xpathObj->nodesetval->nodeTab[i]->parent->children;
			while (currNode)
			{
				if (!xmlStrcmp(currNode->name, (const xmlChar*)"text"))
				{
					strList.push_back(string((const char*)currNode->content));
					break;
				}
				currNode = currNode->next;
			}
		}
		strList.push_back(string((const char*)xpathObj->nodesetval->nodeTab[i]->children->content));
	}
	return strList;
}
