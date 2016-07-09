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


#include "AmazonItemPage.h"
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <tesseract/strngs.h>
#include <iostream>
#include <fstream>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>


//#define CAPTCHA_IMG "../../public_html/amazontest/captcha.png"

int recursionCountIP;



AmazonItemPage::AmazonItemPage()
{
	updateStatus=NULL;
	recursionCountIP=0;
}

AmazonItemPage::AmazonItemPage(string url) :WebPage(url)
{
	updateStatus=NULL;
	recursionCountIP=0;
}


AmazonItemPage::~AmazonItemPage()
{
}

bool AmazonItemPage::isRobotTest()
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

int AmazonItemPage::fetchAndProcess()
{
	WebPage::fetch();
	WebPage::process();
	//cout<<"doing robot check on item page..."<<endl;
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
			/*
			cout << "solve captcha: ";
			std::cin >> ocrResult;
			std::cin.get();
			*/
			updateStatus("ERROR: "+OCR_RESULTS+".txt "+"not opened");
			exit(-1);
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
			recursionCountIP++;
			if(recursionCountIP>=12)
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
			recursionCountIP=0;
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
		processAmazonItem();
	//}
	return 0;
}

int AmazonItemPage::processAmazonItem()
{

	int result = evaluateXPath("//*[@id='productTitle']/text()");
	if (result)
		Title = get_content();

	result = evaluateXPath("//b[contains(text(),'ASIN')]/parent::li/text()");
	if (result)
		ASIN = get_content();
	else //alt ASIN, like: http://www.amazon.com/Vulli-Giraffe-Teether-Discontinued-Manufacturer/dp/B000IDSLOG/ref=sr_1_32/187-5305090-1330158?m=ATVPDKIKX0DER&s=hpc&ie=UTF8&qid=1432273447&sr=1-32
	{
		result = evaluateXPath("//td[contains(text(),'ASIN')]/following-sibling::td/text()");
		if (result)
			ASIN = get_content();
	}
	result = evaluateXPath("normalize-space(//*[@id='SalesRank']/text()[2])");
	if (xpathObj->stringval != 0 && xpathObj->stringval[0] != 0)
	{
		mainRank = string((char*)xpathObj->stringval);
		if (mainRank.size() >= 2)
		{
			//mainRank.pop_back(); 
			//mainRank.pop_back();
			mainRank.erase(mainRank.size() - 2,2);
		}
	}
	else//alt rank, like: http://www.amazon.com/Vulli-Giraffe-Teether-Discontinued-Manufacturer/dp/B000IDSLOG/ref=sr_1_32/187-5305090-1330158?m=ATVPDKIKX0DER&s=hpc&ie=UTF8&qid=1432273447&sr=1-32
	{
		//
		result = evaluateXPath("normalize-space(//*[@id='SalesRank']/td[@class='value']/text())");
		if (xpathObj->stringval != 0)
		{
			mainRank = string((char*)xpathObj->stringval);
			if (mainRank.size() >= 2)
			{
				//mainRank.pop_back(); 
				//mainRank.pop_back();
				mainRank.erase(mainRank.size() - 2,2);
			}
		}
	}
	
	//if (result)
	//	mainRank = get_content();
	result = evaluateXPath("//*[@id='SalesRank']/ul[@class='zg_hrsr']//li[@class='zg_hrsr_item']");
	if (result)
	{
		for (int i = 1;i<=result;i++)
		{
			string lineText;
			evaluateXPath("//*[@id='SalesRank']/ul[@class='zg_hrsr']//li[@class='zg_hrsr_item']["+boost::lexical_cast<std::string>(i)+"]/span[@class='zg_hrsr_rank']/text()");
			string numRank = get_content();
			lineText += numRank + " in ";
			int subResult = evaluateXPath("//*[@id='SalesRank']/ul[@class='zg_hrsr']//li[@class='zg_hrsr_item'][" + boost::lexical_cast<std::string>(i) + "]/span[@class='zg_hrsr_ladder']//a/text()");
			vector<string> category = get_contents();
			for (int j = 0; j<category.size(); j++)
			{
				lineText += category[j];
				lineText += " > ";
			}
			if (lineText.size() >= 3)
			{
				//lineText.pop_back(); 
				//lineText.pop_back(); 
				//lineText.pop_back();
				lineText.erase(lineText.size() - 3,3);
			}

			extraRank.push_back(lineText);
		}
	}
	else //alt secondary rank, like: http://www.amazon.com/Vulli-Giraffe-Teether-Discontinued-Manufacturer/dp/B000IDSLOG/ref=sr_1_32/187-5305090-1330158?m=ATVPDKIKX0DER&s=hpc&ie=UTF8&qid=1432273447&sr=1-32
	{
		result = evaluateXPath("//*[@id='SalesRank']/td[@class='value']//ul[@class='zg_hrsr']//li[@class='zg_hrsr_item']");
		if (result)
		{
			for (int i = 1; i <= result; i++)
			{
				string lineText;
				evaluateXPath("//*[@id='SalesRank']/td[@class='value']//ul[@class='zg_hrsr']//li[@class='zg_hrsr_item'][" + boost::lexical_cast<std::string>(i) + "]/span[@class='zg_hrsr_rank']/text()");
				string numRank = get_content();
				lineText += numRank + " in ";
				int subResult = evaluateXPath("//*[@id='SalesRank']/td[@class='value']//ul[@class='zg_hrsr']//li[@class='zg_hrsr_item'][" + boost::lexical_cast<std::string>(i) + "]/span[@class='zg_hrsr_ladder']//a/text()");
				vector<string> category = get_contents();
				for (int j = 0; j<category.size(); j++)
				{
					lineText += category[j];
					lineText += " > ";
				}
				if (lineText.size() >= 3)
				{
					//lineText.pop_back(); 
					//lineText.pop_back(); 
					//lineText.pop_back();
					lineText.erase(lineText.size() - 3,3);
				}
				extraRank.push_back(lineText);
			}
		}
	}


	// main rank line //*[@id='SalesRank']/text()[2]
	// second tier ranking info //*[@id='SalesRank']/ul[@class='zg_hrsr']
	//number of lines //*[@id='SalesRank']/ul[@class='zg_hrsr']//li[@class='zg_hrsr_item']
	//rank of line i //*[@id='SalesRank']/ul[@class='zg_hrsr']//li[@class='zg_hrsr_item'][i]/span[@class='zg_hrsr_rank']/text()
	//category tree of line i //*[@id='SalesRank']/ul[@class='zg_hrsr']//li[@class='zg_hrsr_item'][i]/span[@class='zg_hrsr_ladder']//a/text()
	return 0;
}

void AmazonItemPage::printInfo()
{
	cout << "url: " << url << endl;
	cout << "ASIN: " << ASIN << endl;
	cout << "Main Rank: " << mainRank << endl;
	for (int i=0;i<extraRank.size();i++)
	{
		cout << "Secondary Rank: " << i << endl;
	}
	cout << endl;
}

void AmazonItemPage::reset()
{
	WebPage::reset();
	ASIN.clear();
	mainRank.clear();
	extraRank.clear();
}
