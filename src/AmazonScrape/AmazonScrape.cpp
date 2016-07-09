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


//#define RANK_HTML "../../public_html/amazontest/rankList.html"
//#define RANK_CSV "../../public_html/amazontest/rankList.csv"
//#define STATUS_HTML "../../public_html/amazontest/status.html"

#define BOOST_ALL_NO_LIB

#include "AmazonResultsPage.h"
#include "AmazonItemPage.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <tclap/CmdLine.h>

#include <libconfig.h++>

void expandTilde(string* pathStr);


using namespace libconfig;

#include <ctime>

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

Config config;
string RANK_HTML;
string STATUS_HTML;
string CAPTCHA_IMG;
string OCR_SCRIPT;
string OCR_RESULTS;
string COOKIES;
string HTML_TOP_TEMPLATE;
string HTML_BOTTOM_TEMPLATE;
string HTML_CSS;
string CSS_FILE_NAME;
int UPPER_DELAY;
int LOWER_DELAY;

CURL *globalCURL;
struct sigaction sigIntHandler;
string top;
string bottom;
std::ofstream progressFile;
std::ofstream statusFile;
time_t startTime;
time_t currTime;

int currResultPage;
int totResultPage;
int currItemPage;
int totItemPage;


void updateScraperStatus(string status)
{
	
	
	
	time(&currTime);
	
	float runningTime = (float)difftime(currTime, startTime)/60.0/60.0;
	
	statusFile.open(STATUS_HTML, std::ios::trunc);
	if (!statusFile.is_open())
	{
		cout<<"status file not created"<<endl;
		return;
	}
	statusFile<<"<!DOCTYPE HTML><html><head><title>Scrape Status</title></head><body>";
	statusFile<<"The current scrape has been running for: "<<runningTime<<" hours";
	statusFile<<"<br>";
	statusFile<<"Estimated time until completion: "<<((float)totResultPage*totItemPage)/(float)((currResultPage*totItemPage)+currItemPage)*runningTime<<" hours";
	statusFile<<"<br>";
	statusFile<<"Results page "<<currResultPage<<" of "<<totResultPage<<" is currently being scraped";
	statusFile<<"<br>";
	statusFile<<"Item "<<currItemPage<<" of "<<totItemPage<<" is currently being processed";
	statusFile<<"<br>";
	statusFile<<"The current scraper status is: "<<status;
	if(status.compare("Processing Captcha")==0)
	{
		statusFile<<"<br>";
		statusFile<<"<img src=\"captcha.png\">";
	}
	statusFile<<"</body></html>";
	statusFile.close();
}


struct ItemData
{
	ItemData(){}
	ItemData(string ASIN_in, string Title_in, string Rank_in, string URL_in)
	{
		ASIN = ASIN_in;
		Title = Title_in;
		Rank = Rank_in;
		URL = URL_in;
	}
	string ASIN;
	string Title;
	string Rank;
	string URL;
};

vector<ItemData> otherRankList;


void my_handler(int s)
{
updateScraperStatus("Exiting, signal: "+boost::lexical_cast<std::string>(s)+" received");
printf("Caught signal %d\n",s);
cout<<"Saving Cookies to: "<<COOKIES<<endl;
curl_easy_cleanup(globalCURL);
exit(1); 
}




void evaluateRank(AmazonItemPage &ip, vector<std::pair<string, string>> &rankList)
{
	//ip.printInfo();
	//progressFile << "url: " << ip.url << endl;
	//progressFile << "ASIN: " << ip.ASIN << endl;
	string match = ip.evaluateRegexStr_first("Health & Personal Care", ip.mainRank);
	if (!match.empty())
	{
		match = ip.evaluateRegexStr_first("#(\\d{1,3}(,\\d{3})*)", ip.mainRank, 1);
		//progressFile << "Health & Personal Care #" << match << endl;
		rankList.push_back(std::pair<string, string>(match, ip.url));
		otherRankList.push_back(ItemData(ip.ASIN,ip.Title,match,ip.url));
	}

	for (int i = 0; i<ip.extraRank.size(); i++)
	{
		match = ip.evaluateRegexStr_first("Health & Personal Care", ip.extraRank[i]);
		if (!match.empty())
		{
			match = ip.evaluateRegexStr_first(">", ip.extraRank[i]);
			if (match.empty())
			{
				//progressFile << "not nested!" << endl;
			}
		}

	}
}

bool rankSorter(std::pair<string, string> a, std::pair<string, string> b) 
{
	string strA = a.first;
	string strB = b.first;

	boost::erase_all(strA, ",");
	boost::erase_all(strB, ",");

	int rankA = std::stoi(strA);
	int rankB = std::stoi(strB);
	return rankA < rankB;
}

bool otherRankSorter(ItemData a, ItemData b)
{
	string strA = a.Rank;
	string strB = b.Rank;

	boost::erase_all(strA, ",");
	boost::erase_all(strB, ",");

	int rankA = std::stoi(strA);
	int rankB = std::stoi(strB);
	return rankA < rankB;
}

void saveRankCSV(vector<std::pair<string, string>> &rankList, string path)
{
	std::sort(rankList.begin(), rankList.end(), rankSorter);

	std::ofstream myfile(path, std::ios::trunc);
	if (myfile.is_open())
	{
		for(int i=0;i<rankList.size();i++)
		{
			string rank = rankList[i].first;
			boost::erase_all(rank, ",");
			myfile << rank << "," << rankList[i].second << endl;
		}
		myfile.close();
	}
}

void saveRankHTML(vector<ItemData> &rankList, string path)
{
	std::sort(otherRankList.begin(), otherRankList.end(), otherRankSorter);
	
	std::ofstream myfile(path, std::ios::trunc);
	if (myfile.is_open())
	{
		string beginRow = "<tr>";
		string beginRowAlt = "<tr class=\"alt\">";
		int row = 0;

		myfile << top;

		for(int i=0;i<rankList.size();i++)
		{
			if (row % 2){ myfile << beginRow << endl; }
			else{ myfile << beginRowAlt << endl; }

			myfile << "<td>";
			myfile << rankList[i].Rank;
			myfile << "</td>" << endl;

			myfile << "<td>";
			myfile << "<a href=\""<<rankList[i].URL<<"\">"<<rankList[i].ASIN<<"</a>";
			myfile << "</td>" << endl;

			myfile << "<td>";
			myfile << rankList[i].Title;
			myfile << "</td>" << endl;

			myfile << "</tr>" << endl;
			row++;
		}

		myfile << bottom;
		myfile.close();
	}
}

int readSavedRanking()
{
	string rankList;
	std::ifstream rankListFile(RANK_HTML);
	if (rankListFile.is_open())
	{
		std::stringstream buffer;
		buffer << rankListFile.rdbuf();
		rankList = buffer.str();
		rankListFile.close();
	}
	else
	{
		return -1;
	}

	WebPage dummy;
	dummy.process(rankList);

	dummy.evaluateXPath("//html/body/div/table/tbody//tr");
	if (dummy.xpathObj)
	{
		if (dummy.xpathObj->nodesetval)
		{
			for (int i = 0; i < dummy.xpathObj->nodesetval->nodeNr; i++)
			{
				dummy.evaluateXPath("//html/body/div/table/tbody//tr["+boost::lexical_cast<std::string>(i+1)+"]/td[1]/text()");
				string rank = dummy.get_content();

				dummy.evaluateXPath("//html/body/div/table/tbody//tr[" + boost::lexical_cast<std::string>(i+1) + "]/td[2]/a/text()");
				string ASIN = dummy.get_content();

				dummy.evaluateXPath("//html/body/div/table/tbody//tr[" + boost::lexical_cast<std::string>(i+1) + "]/td[2]/a/@href");
				string url = dummy.get_link_from_a_href();

				dummy.evaluateXPath("//html/body/div/table/tbody//tr[" + boost::lexical_cast<std::string>(i+1) + "]/td[3]/text()");
				string description = dummy.get_content();

				otherRankList.push_back(ItemData(ASIN,description,rank,url));
				dummy.evaluateXPath("//html/body/div/table/tbody//tr");
			}
		}
	}
	return 0;
}

void init(int argc, char** argv)
{
	string configFilePath;
	try
	{
	TCLAP::CmdLine cmd("2015 - athomedev@athomedev.com", ' ', "1.0");
	TCLAP::ValueArg<std::string> configArg("c","config","Configuration file to use",false,"./data/settings.cfg","string");
	cmd.add( configArg );
	TCLAP::ValueArg<int> resumeArg("r","resume","Resume scrape from result page",false,1,"int");
	cmd.add( resumeArg );
	//TCLAP::SwitchArg reverseSwitch("r","reverse","Print name backwards", cmd, false);
	cmd.parse(argc,argv);
	configFilePath = configArg.getValue();
	//bool reverseName = reverseSwitch.getValue();
	
	} catch (TCLAP::ArgException &e)  // catch any exceptions
	{ std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;exit(1); }
	
	
	LOWER_DELAY=1;
	UPPER_DELAY=4;
	srand (time(NULL));
	/*
	progressFile.open("progress.txt", std::ios::out|std::ios::trunc);
	if (!progressFile.is_open())
	{
		cout<<"progress file not created"<<endl;
		exit(0);
	}
	*/
	try
	{
	config.readFile(configFilePath.c_str());
	}
	catch(...)
	{
		cout<<"Error reading configuration file: "+configFilePath<<endl;
		exit(-1);
	}
	RANK_HTML = config.lookup("application.files.RANK_HTML").c_str();
	STATUS_HTML = config.lookup("application.files.STATUS_HTML").c_str();
	CAPTCHA_IMG = config.lookup("application.files.CAPTCHA_IMG").c_str();
	OCR_SCRIPT = config.lookup("application.files.OCR_SCRIPT").c_str();
	OCR_RESULTS = config.lookup("application.files.OCR_RESULTS").c_str();
	COOKIES = config.lookup("application.files.COOKIES").c_str();
	HTML_TOP_TEMPLATE = config.lookup("application.files.HTML_TOP_TEMPLATE").c_str();
	HTML_BOTTOM_TEMPLATE = config.lookup("application.files.HTML_BOTTOM_TEMPLATE").c_str();
	HTML_CSS = config.lookup("application.files.HTML_CSS").c_str();
	UPPER_DELAY = config.lookup("application.parameters.UPPER_DELAY");
	LOWER_DELAY = config.lookup("application.parameters.LOWER_DELAY");
	
	expandTilde(&RANK_HTML);
	expandTilde(&STATUS_HTML);
	expandTilde(&CAPTCHA_IMG);
	expandTilde(&OCR_SCRIPT);
	expandTilde(&OCR_RESULTS);
	expandTilde(&COOKIES);
	expandTilde(&HTML_TOP_TEMPLATE);
	expandTilde(&HTML_BOTTOM_TEMPLATE);
	expandTilde(&HTML_CSS);
	
	if(!boost::filesystem::exists(HTML_CSS))
	{
		updateScraperStatus("ERROR: HTML_CSS: "+HTML_CSS);
		cout<<"ERROR: HTML_CSS: "+HTML_CSS<<endl;
		exit(-1);
	}
	
	string cssPath;
	boost::filesystem::path tempPath;
	tempPath = RANK_HTML;
	cssPath = tempPath.branch_path().string();
	tempPath = HTML_CSS;
	cssPath += "/"+tempPath.leaf().string();
	boost::filesystem::copy_file(HTML_CSS,cssPath,boost::filesystem::copy_option::overwrite_if_exists);
	
	
	
	
	curl_global_init(CURL_GLOBAL_ALL);
	xmlInitParser();

	std::ifstream t(HTML_TOP_TEMPLATE);
	if (t.is_open())
	{
		std::stringstream buffer;
		buffer << t.rdbuf();
		top = buffer.str();
		t.close();
	}
	else
	{
		updateScraperStatus("ERROR: HTML_TOP_TEMPLATE: "+HTML_TOP_TEMPLATE);
		cout<<"ERROR: HTML_TOP_TEMPLATE: "+HTML_TOP_TEMPLATE<<endl;
		exit(1);
	}

	std::ifstream b(HTML_BOTTOM_TEMPLATE);
	if (b.is_open())
	{
		std::stringstream buffer;
		buffer << b.rdbuf();
		bottom = buffer.str();
		b.close();
	}
	else
	{
		updateScraperStatus("ERROR: HTML_BOTTOM_TEMPLATE: "+HTML_BOTTOM_TEMPLATE);
		cout<<"ERROR: HTML_BOTTOM_TEMPLATE: "+HTML_BOTTOM_TEMPLATE<<endl;
		exit(1);
	}
	

   sigIntHandler.sa_handler = my_handler;
   sigemptyset(&sigIntHandler.sa_mask);
   sigIntHandler.sa_flags = 0;

   sigaction(SIGINT, &sigIntHandler, NULL);
	
}

void expandTilde(string* pathStr)
{
	char* homeEnv;
	string homePath;
	homeEnv = getenv("HOME");
	homePath.assign(homeEnv);
	
	if((*pathStr)[0]=='~')
	{
		(*pathStr).erase(0,1);
		(*pathStr).insert(0,homePath);
	}
}


int main(int argc, char** argv)
{
	int startFrom=1;
	time(&startTime);
	
	init(argc, argv);	
	
	
	if (argc>2)
		if (string(argv[1]).compare("r") == 0)
		{
			readSavedRanking();
			startFrom = std::stoi(argv[2]);
			
		}

	AmazonItemPage ip;
	AmazonResultsPage rp;
	
	ip.updateStatus = updateScraperStatus;
	rp.updateStatus = updateScraperStatus;
	
	ip.CAPTCHA_IMG = CAPTCHA_IMG;
	ip.OCR_SCRIPT = OCR_SCRIPT;
	ip.OCR_RESULTS = OCR_RESULTS;
	
	rp.CAPTCHA_IMG = CAPTCHA_IMG;
	rp.OCR_SCRIPT = OCR_SCRIPT;
	rp.OCR_RESULTS = OCR_RESULTS;
	
	
	curl_easy_cleanup(ip.curl);
	ip.curl = rp.curl;
	globalCURL = rp.curl;
	
	curl_easy_setopt(globalCURL,CURLOPT_COOKIEFILE,COOKIES.c_str());
	curl_easy_setopt(globalCURL,CURLOPT_COOKIEJAR,COOKIES.c_str()); 
	
	vector<std::pair<string, string>> rankList;

	//cout<<"fetching first result page"<<endl;
	rp.setURL("http://www.amazon.com/s/ref=sr_pg_"+boost::lexical_cast<std::string>(startFrom)+"?me=ATVPDKIKX0DER&fst=as%3Aoff&rh=n%3A3760901&page="+boost::lexical_cast<std::string>(startFrom)+"&bbn=3760901&ie=UTF8&qid=1431020095&spIA=B00BUTOO6G,B00BMVQS02,B00H6Y4NT2,B00OR1QR3W,B00QVNKSA2,B00PV15BPW");
	rp.fetchAndProcess();
	


	int pageCount = rp.getPageCount();
	totResultPage = pageCount;
	
	if(pageCount<400)
	{
		cout<<"pageCount error"<<endl;
		updateScraperStatus("pageCount error");
		rp.saveData("pageCount_error.html");
		exit(-1);
	}

	
	
	
	//progressFile<<"pageCount: "<<pageCount<<endl;
	
	//rp.saveData(string("0_result_page")+string(".html"));

	//progressFile<<"starting main loop"<<endl;
	for (int i = startFrom; i <= pageCount; i++)
	{
		currResultPage=i;
		
		updateScraperStatus("Scraping");
		
		//progressFile << "starting page " << i/*rp.getCurrentPageNumber()*/ << " of " << pageCount << endl << endl;
		
		
		//string nextURL = string("http://www.amazon.com/s/ref=sr_pg_")+boost::lexical_cast<std::string>(i)+string("?me=ATVPDKIKX0DER&fst=as%3Aoff&rh=n%3A3760901&page=")+boost::lexical_cast<std::string>(i)+("&bbn=3760901&ie=UTF8&qid=1431020095&spIA=B00BUTOO6G,B00BMVQS02,B00H6Y4NT2,B00OR1QR3W,B00QVNKSA2,B00PV15BPW");
		
		
		//rp.reset();
		//rp.setURL(nextURL);
		//rp.fetchAndProcess();
		
		
		vector<string> urls = rp.getItemURLs();
		//rp.saveData(string("result_page_")+boost::lexical_cast<std::string>(i)+string(".html"));
		//progressFile<<urls.size()<<" urls found"<<endl;

		totItemPage=urls.size();

		for (int j = 0; j<urls.size(); j++)
		{
			currItemPage=j+1;
			updateScraperStatus("Scraping");
			ip.reset();
			ip.setURL(urls[j]);
			ip.fetchAndProcess();
			evaluateRank(ip, rankList);
			//std::this_thread::sleep_for(std::chrono::seconds(5));
			int delayVal = rand() % 300 + 100;
			//cout<<"delay value: "<<delayVal<<" milliseconds"<<endl;
			boost::this_thread::sleep( boost::posix_time::milliseconds(delayVal) );
		}
		//saveRankCSV(rankList, RANK_CSV);
		saveRankHTML(otherRankList, RANK_HTML);
		
		string nextPage = rp.getPageLink();
		if (!nextPage.empty())
		{
			string nextURL = rp.getPageLink();
			//progressFile<<"listing url: "<<nextURL<<endl;
			rp.reset();
			rp.setURL(nextURL);
			rp.fetchAndProcess();
		}
		else
		{
			//progressFile << "no next page" << endl;
			break;
		}
		//progressFile << "starting page " << rp.getCurrentPageNumber() << " of " << pageCount << endl << endl;
		
	}

	//for (int i=0;i<rankList.size();i++)
		//progressFile << rankList[i].first << "," << rankList[i].second << endl;
	
	updateScraperStatus("Finished Scraping");
	cout<<"Finished Scraping"<<endl;
	
	curl_easy_cleanup(ip.curl);
	rp.curl=0;
	ip.curl=0;
	//progressFile << endl << endl << "***** END *****" << endl << endl;
	curl_global_cleanup();
	xmlCleanupParser();

	//std::cin.get();
	return 0;
}

