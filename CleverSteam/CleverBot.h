/*
	CleverBot
	https://github.com/Spacetech/CleverSteam
*/

#ifndef CLEVER_BOT_H
#define CLEVER_BOT_H

#include <string>
#include "md5.h"

#define CURL_STATICLIB
#include "curl.h"

#include "boost\regex.hpp"
#include "boost\lexical_cast.hpp"
#include "boost\algorithm\string.hpp"

using namespace std;
using namespace boost;

typedef vector<string> stringVector;

class CleverBot
{
public:
	CleverBot();
	~CleverBot();
	int GetKeyIndex(string key);
	string CleanSpaces(string text);
	string Encode(stringVector keys, stringVector args);
	string Send();
	string Ask(string question);
	void Previous(stringVector myListCopy);
	int curlWriter(char *data, size_t size, size_t nmemb, string *buffer); 
	static int staticCurlWriter(char *data, size_t size, size_t nmemb, string *buffer, void *f);

private:
	CURL *curlHandle;
	string curlData;
	struct curl_slist *headers;
	char curlErrorBuffer[CURL_ERROR_SIZE];

	stringVector msgList;
	stringVector keyList;
	stringVector argList;
};

#endif
