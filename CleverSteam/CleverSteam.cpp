/*
	CleverSteam
	https://github.com/Spacetech/CleverSteam
*/

#include "CleverSteam.h"

//#define DEBUG_MODE

struct friendData
{
	int setup;
	bool ignore;
	CleverBot *bot;
};

typedef map<CSteamID, friendData> friend_map;
friend_map friend_ids;
friend_map::iterator it;

struct msgData
{
	CSteamID sender;
	string message;
	bool chatRoom;
	time_t timeStamp;
};
vector<msgData> msgQueue;

IClientFriends *clientFriends = NULL;
ISteamFriends001 *steamFriends = NULL;

void CheckFriend(CSteamID id)
{
	if(friend_ids[id].setup == NULL)
	{
		cout << "\tSetup Friend CleverBot" << endl;
		friendData data;
		data.setup = 1;
		data.ignore = false;
		data.bot = new CleverBot();
		friend_ids[id] = data;
	}
}

int main()
{
	cout << "CleverSteam " << CLEVERSTEAM_VERSION << endl;
	cout << "https://github.com/Spacetech/CleverSteam" << endl;

	CSteamAPILoader loader;

	CreateInterfaceFn factory = loader.GetSteam3Factory();

	ISteamClient009 *steamClient = (ISteamClient009*)factory(STEAMCLIENT_INTERFACE_VERSION_009, NULL);
	if(!steamClient)
	{
		cout << "Error: Unable to load steamClient" << endl;
		return EXIT_FAILURE;
	}

	HSteamPipe steamPipe = steamClient->CreateSteamPipe();
	HSteamUser steamUser = steamClient->ConnectToGlobalUser(steamPipe);

	IClientEngine *engine = (IClientEngine*)factory(CLIENTENGINE_INTERFACE_VERSION, NULL);
	if(!engine)
	{
		cout << "Error: Unable to load engine" << endl;
		return EXIT_FAILURE;
	}
	
	IClientUser *clientUser = engine->GetIClientUser(steamUser, steamPipe, CLIENTUSER_INTERFACE_VERSION);
	if(!clientUser)
	{
		cout << "Error: Unable to load clientUser" << endl;
		return EXIT_FAILURE;
	}

	clientFriends = engine->GetIClientFriends(steamUser, steamPipe, CLIENTFRIENDS_INTERFACE_VERSION);
	if(!clientFriends)
	{
		cout << "Error: Unable to load clientFriends" << endl;
		return EXIT_FAILURE;
	}

	steamFriends = (ISteamFriends001*)steamClient->GetISteamFriends(steamUser, steamPipe, STEAMFRIENDS_INTERFACE_VERSION_001);
	if(!steamFriends)
	{
		cout << "Error: Unable to load steamFriends" << endl;
		return EXIT_FAILURE;
	}

	CSteamID mySteamID = clientUser->GetSteamID();

	CallbackMsg_t callbackMsg;

	while(true)
	{
		if(Steam_BGetCallback(steamPipe, &callbackMsg))
		{
			bool groupChat = callbackMsg.m_iCallback == ChatRoomMsg_t::k_iCallback;
			if(callbackMsg.m_iCallback == FriendChatMsg_t::k_iCallback || groupChat)
			{
				FriendChatMsg_t *chatMsg = (FriendChatMsg_t*)callbackMsg.m_pubParam;

				EChatEntryType msgType;

				CSteamID sender = chatMsg->m_ulFriendID;

				string s_friendName(steamFriends->GetFriendPersonaName(chatMsg->m_ulSenderID));

				char pvData[2048];

				bool chatRoom = false;

				int ret = steamFriends->GetChatMessage(sender, chatMsg->m_iChatID, pvData, 2048, &msgType);
				if(ret == 0)
				{
					chatRoom = true;
					clientFriends->GetChatRoomEntry(sender, chatMsg->m_iChatID, &chatMsg->m_ulSenderID, pvData, 2048, &msgType);
				}

				string message = pvData;

				if(msgType == k_EChatEntryTypeChatMsg)
				{
#ifndef DEBUG_MODE
					if(chatMsg->m_ulSenderID == mySteamID)
					{
#endif
						if(message == "!ignore")
						{
							cout << "Found Ignore Command" << endl;

							CheckFriend(chatMsg->m_ulFriendID);
							
							friend_ids[chatMsg->m_ulFriendID].ignore = !friend_ids[chatMsg->m_ulFriendID].ignore;

							if(friend_ids[chatMsg->m_ulFriendID].ignore)
							{
								cout << "\tIgnoring " << steamFriends->GetFriendPersonaName(chatMsg->m_ulFriendID) << ": On" << endl;
							}
							else
							{
								cout << "\tIgnoring " << steamFriends->GetFriendPersonaName(chatMsg->m_ulFriendID) << ": Off" << endl;
							}
						}
#ifndef DEBUG_MODE
					}
					else
					{
#endif
						cout << s_friendName + ": " << message << endl;

						CheckFriend(sender);

						if(!friend_ids[sender].ignore)
						{
							msgData data;
							data.sender = sender;
							data.message = message;
							data.chatRoom = chatRoom;
							data.timeStamp = time(NULL);

							msgQueue.insert(msgQueue.begin(), data);

							cout << "\tAdding Message to Queue" << endl;
						}
						else
						{
							cout << "\tIgnoring Message" << endl;
						}
#ifndef DEBUG_MODE
					}
#endif
				}
			}
			else if(callbackMsg.m_iCallback == ChatRoomInvite_t::k_iCallback)
			{
				ChatRoomInvite_t *chatRoomInvite = (ChatRoomInvite_t*)callbackMsg.m_pubParam;
				
				string s_friendName(steamFriends->GetFriendPersonaName(chatRoomInvite->m_ulSteamIDPatron));

				cout << "Received Chatroom Invite from " << s_friendName << endl;

				bool joined = clientFriends->JoinChatRoom(chatRoomInvite->m_ulSteamIDChat);
				if(joined)
				{
					cout << "\tJoined Chatroom" << endl;
				}
				else
				{
					cout << "\tFailed to join Chatroom" << endl;
				}
			}

			Steam_FreeLastCallback(steamPipe);
		}
		else if(msgQueue.size() > 0)
		{
			cout << "Running Queue" << endl;
			msgData data = msgQueue.back();

			CleverBot *bot = friend_ids[data.sender].bot;

			cout << "\tAsking CleverBot: " << data.message << endl;
			string answer = bot->Ask(data.message);

			cout << "\tCleverBot Answered: " << answer << endl;

			if(data.chatRoom)
			{
				clientFriends->SendChatMsg(data.sender, k_EChatEntryTypeChatMsg, answer.c_str(), answer.length());
			}
			else
			{
				steamFriends->SendMsgToFriend(data.sender, k_EChatEntryTypeChatMsg, answer.c_str(), answer.length());
			}

			msgQueue.pop_back();

			// some good code right here
			while(true)
			{
				bool done = true;

				for(unsigned int i=0; i < msgQueue.size(); i++)
				{
					if(msgQueue[i].sender == data.sender)
					{
						done = false;
						msgQueue.erase(msgQueue.begin() + i);
						cout << "\tRemoved Message" << endl;				
						break;
					}
				}

				if(done)
				{
					break;
				}
			}
		}

		Sleep(10);
	}

	return EXIT_SUCCESS;
}
