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


#include "AmazonResultsPage.h"
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <tesseract/strngs.h>
#include <iostream>
#include <fstream>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

int recursionCountRP;

//#define CAPTCHA_IMG "../../public_html/amazontest/captcha.png"

AmazonResultsPage::AmazonResultsPage()
{
	updateStatus=NULL;
	recursionCountRP=0;
}

AmazonResultsPage::AmazonResultsPage(string url) :WebPage(url)
{
	updateStatus=NULL;
	recursionCountRP=0;
}


AmazonResultsPage::~AmazonResultsPage()
{
}

int AmazonResultsPage::fetchAndProcess()
{
	WebPage::fetch();
	WebPage::process();
	//cout<<"doing robot check on results page..."<<endl;
	if(isRobotTest())
	{
		//saveData("robotCheck.html");
		int result = evaluateXPath("//input[@name = 'amzn']/@value");
		string amzn = get_content_from_child();
		//cout << amzn << endl;

		result = evaluateXPath("//div[@class='a-box-inner']//img/@src");
		string captchaIMG = get_content_from_child();
		
		WebPage image(captchaIMG);
		image.fetch();
		image.saveData(CAPTCHA_IMG);
		updateStatus("Processing Captcha");
		
		//cout << captchaIMG << endl;

		string newURL("www.amazon.com/errors/validateCaptcha?amzn=");

		size_t f = amzn.find("=");
		while (f != -1)
		{
			amzn.replace(f, std::string("=").length(), "%3D");
			f = amzn.find("=");
		}
		
		
		result = evaluateXPath("//input[@name='amzn-r']/@value");
		string amznRef = get_content_from_child();

		newURL = newURL + amzn;
		
		
		//system((string("./ocrCAPTCHA ")+string(CAPTCHA_IMG)).c_str());
		system((OCR_SCRIPT+" "+CAPTCHA_IMG).c_str());
	
		string ocrResult;
		std::ifstream ocrFile;
		//ocrFile.open("out.txt");
		ocrFile.open(OCR_RESULTS+".txt");
		if (ocrFile.is_open())
		{
			getline(ocrFile, ocrResult);
			ocrFile.close();
			boost::filesystem::remove(OCR_RESULTS+".txt");
		}
		else
		{
			updateStatus("ERROR: "+OCR_RESULTS+".txt "+"not opened");
			exit(-1);
			/*
			cout << "solve captcha: ";
			std::cin >> ocrResult;
			std::cin.get();
			*/
		}
		//cout << ocrResult << endl;
		


		newURL = newURL + "&amzn-r=" + amznRef + "&field-keywords=" + ocrResult;
		//cout << newURL << endl;
		string oldURL = url;
		reset();
		setURL(newURL);
		WebPage::fetch();
		WebPage::process();
		//saveData("00results.html");
		if(isRobotTest())
		{
			recursionCountRP++;
			if(recursionCountRP>=12)
			{
				updateStatus("ERROR: Failed Captcha 12 times in a row...");
				cout<<"ERROR: Failed Captcha 12 times in a row..."<<endl;
				exit(-1);
			}
			
			//cout<<"*****THEY BEAT US*****"<<endl;
			reset();
			setURL(oldURL);
			fetchAndProcess();
		}
		else
		{
			recursionCountRP=0;
			//cout<<"*****WE BEAT THEM*****"<<endl;
		}
		
		/*
		int result = evaluateXPath("//input[@name = 'amzn']/@value");
		string amzn = get_content_from_child();
		cout << amzn << endl;

		result = evaluateXPath("//div[@class='a-box-inner']//img/@src");
		string captchaIMG = get_content_from_child();
		cout << captchaIMG << endl;
		WebPage image(captchaIMG);
		image.fetch();
		image.saveData("image.png");
		tesseract::TessBaseAPI api;
		api.Init(getenv("TESSDATA_PREFIX"), "eng", tesseract::OEM_DEFAULT);
		api.SetPageSegMode(static_cast<tesseract::PageSegMode>(8));
		api.SetOutputName("out");
		PIX   *pixs = pixRead("image.png");
		STRING text_out;
		api.ProcessPages("image.png", NULL, 0, &text_out);
		cout<<text_out.string();
		exit(-1);
		*/
	}
	else
	{
		//cout<<"We legit"<<endl;
	}
	//if(!nested)
	//{
	//}
	return 0;
}

bool AmazonResultsPage::isRobotTest()
{
	int results = evaluateXPath("//html/head/title[text()='Robot Check']");
	if(results)
	{
		//cout<<"found"<<endl;
		return true;
	}	
	//cout<<"not found"<<endl;
	return false;
}

int AmazonResultsPage::getPageCount()
{
	int result = evaluateXPath("//*[@id='pagn']/span[@class='pagnRA']/preceding-sibling::span[1]/text()");
	if (result)
		return std::stoi(get_content());
	else
	{
		result = evaluateXPath("//*[@id='pagn']/span[@class='pagnRA1']/preceding-sibling::span[1]/text()");
		if (result)
		{
			return std::stoi(get_content());
		}
		else
		{
			result = evaluateXPath("//*[@id='pagn']/span[@class='pagnRA']/preceding-sibling::span[1]/a/text()");
			if (result)
			{
				return std::stoi(get_content());
			}
		}
	}
		return 0;
}

string AmazonResultsPage::getPageLink()
{
	int result = evaluateXPath("//*[@id='pagn']/span[@class='pagnRA'][1]/a/@href");
	if (result)
	{
		string link = get_link_from_a_href();
		if (link[0] == '/')
			link.assign(baseURL + link);
		return link;
		
	}
	else
		return string("");
}

int AmazonResultsPage::getCurrentPageNumber()
{
	int result = evaluateXPath("//*[@id='pagn']/span[@class='pagnCur']/text()");
	if (result)
	{
		return std::stoi(get_content());
	}
	else
		return -1;
}

vector<string> AmazonResultsPage::getItemURLs()
{
	vector<string> itemList;
	int count = evaluateXPath("//a[contains(@class,'s-access-detail-page')]/@href");
	if (count)
	{
		itemList = get_links_from_a_href();
	}
	for (int i=0;i<itemList.size();i++)
	{
		if (itemList[i][0] == '/')
			itemList[i].assign(baseURL + itemList[i]);
	}
	return itemList;
}

void AmazonResultsPage::reset()
{
	WebPage::reset();
}