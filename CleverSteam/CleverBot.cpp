/*
	CleverBot
	https://github.com/Spacetech/CleverSteam
*/

#include "CleverBot.h"

string keys[] = { "stimulus", "start", "sessionid", "vText8", "vText7", "vText6", "vText5", "vText4", "vText3", "vText2", "icognoid", "icognocheck", "fno", "prevref", "emotionaloutput", "emotionalhistory", "asbotname", "ttsvoice", "typing", "tweak1", "tweak2", "tweak3", "lineref", "sub", "islearning", "cleanslate" };
string args[] = { "", "y", "", "", "", "", "", "", "", "", "wsf", "", "0", "", "", "", "", "", "", "-1", "-1", "-1", "", "Say", "1", "false" };

CleverBot::CleverBot()
{
	curlHandle = curl_easy_init();

	if (curlHandle == NULL)
	{
		cout << "Failed to initalize curl" << endl;
		return;
	}

	curl_easy_setopt(curlHandle, CURLOPT_COOKIEJAR, "cookies.txt");
	curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, "");

	curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curlHandle, CURLOPT_HEADER, 0);
	curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 0);
	//curl_easy_setopt(curlHandle, CURLOPT_PROXY, "127.0.0.1:8888");

	curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, curlErrorBuffer);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, CleverBot::staticCurlWriter);
	curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &curlData);

	curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(curlHandle, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv3);

	///////////////////////////////////////////////////////////////////////////////

	headers = NULL;

	headers = curl_slist_append(headers, "Cache-Control: no-cache");
	headers = curl_slist_append(headers, "Origin: http://www.cleverbot.com");
	headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/38.0.2114.2 Safari/537.36");
	headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
	headers = curl_slist_append(headers, "Accept: */*");
	headers = curl_slist_append(headers, "DNT: 1");
	headers = curl_slist_append(headers, "Referer: http://www.cleverbot.com/");
	headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.8");

	keyList = stringVector(keys, keys + sizeof(keys) / sizeof(string));
	argList = stringVector(args, args + sizeof(args) / sizeof(string));
}

CleverBot::~CleverBot()
{
	curl_easy_cleanup(curlHandle);
}

int CleverBot::GetKeyIndex(string key)
{
	for (unsigned int i = 0; i < keyList.size(); i++)
	{
		if (_strcmpi(key.c_str(), keyList[i].c_str()) == 0)
		{
			return i;
		}
	}
	return -1;
}

string CleverBot::CleanSpaces(string text)
{
	for (size_t pos = text.find(" "); pos != string::npos; pos = text.find(" ", pos))
	{
		text.replace(pos, 1, "%20");
	}

	return text;
}

string CleverBot::Encode(stringVector keys, stringVector args)
{
	string text = "";
	for (unsigned int i = 0; i < keys.size(); i++)
	{
		text += "&" + keys[i] + "=" + CleanSpaces(args[i]);
	}
	return text.substr(1);
}

string CleverBot::Send()
{
	string data = Encode(keyList, argList);

	string digest_txt = data.substr(9, 26);

	/*
	they sometimes change the hash length
	I submit a real request from their website
	then I search for the hash in the output to figure out the length

	std::ofstream file;
	file.open("output.txt");

	for (int i = 0; i < data.length(); i++)
	{
		for (int j = 0; j < data.length(); j++)
		{
			digest_txt = data.substr(i, j);
			file << i << ", " << j << ": " << md5(digest_txt) << endl;
		}
	}

	file.close();
	*/

	string hash = md5(digest_txt);

	argList[GetKeyIndex("icognocheck")] = hash;

	data = Encode(keyList, argList);

	cout << data << endl;

	curl_easy_setopt(curlHandle, CURLOPT_URL, "http://www.cleverbot.com/webservicemin");
	curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curlHandle, CURLOPT_POST, 1);

	curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, data);
	
	//curl_easy_setopt(curlHandle, CURLOPT_ENCODING, "gzip,deflate");

	curlData.clear();

	if (curl_easy_perform(curlHandle) != CURLE_OK)
	{
		cout << "Error: " << curlErrorBuffer << endl;
		return "";
	}

	return curlData;
}

string CleverBot::Ask(string question)
{
	argList[GetKeyIndex("stimulus")] = question;

	if (msgList.size() > 0)
	{
		args[GetKeyIndex("lineref")] = "!0" + msgList.size() / 2;
	}

	Previous(msgList);

	string reply = Send();

	if (reply.find_first_of("<meta name=\"description\" content=\"Jabberwacky server maintenance\">") == string::npos)
	{
		cout << "The Cleverbot server answered with full load error" << endl;
		return "";
	}

	vector<string> split;
	boost::split(split, reply, boost::is_any_of("\r"));

	argList[GetKeyIndex("sessionid")] = split[1];
	argList[GetKeyIndex("prevref")] = split[10];

	msgList.push_back(question);
	msgList.push_back(split[0]);

	return split[0];
}

void CleverBot::Previous(stringVector myListCopy)
{
	for (int i = 2; i < 9; i++)
	{
		if (myListCopy.size() == 0)
		{
			return;
		}
		argList[GetKeyIndex("vText" + boost::lexical_cast<string>(i))] = myListCopy.back();
		myListCopy.pop_back();
	}
}

int CleverBot::curlWriter(char *data, size_t size, size_t nmemb, string *buffer)
{
	if (buffer != NULL)
	{
		buffer->append(data, size * nmemb);
	}
	return size * nmemb;
}

int CleverBot::staticCurlWriter(char *data, size_t size, size_t nmemb, string *buffer, void *f)
{
	return static_cast<CleverBot*>(f)->curlWriter(data, size, nmemb, buffer);
}
