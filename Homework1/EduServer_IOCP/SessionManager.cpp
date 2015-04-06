#include "stdafx.h"
#include "ClientSession.h"
#include "SessionManager.h"

SessionManager* GSessionManager = nullptr;

ClientSession* SessionManager::CreateClientSession(SOCKET sock)
{
	ClientSession* client = new ClientSession(sock);

	//DONE: lock으로 보호할 것

	FastSpinlockGuard lock(m_Lock);
	{
		mClientList.insert(ClientList::value_type(sock, client));
	}

	return client;
}


void SessionManager::DeleteClientSession(ClientSession* client)
{
	//DONE: lock으로 보호할 것
	FastSpinlockGuard lock(m_Lock);
	{
		mClientList.erase(client->mSocket);
	}
	
	delete client;
}