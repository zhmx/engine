
#include "Global.h"
#include "Server.h"
#include "Client.h"
#include "DB.h"

extern "C"
{
	int luaopen_sproto_core(lua_State *L);
	int luaopen_lpeg(lua_State *L);
}

void BindInterfaceToLua(kaguya::State *pKaguyaState)
{
	if (NULL == pKaguyaState)
	{
		std::cout << "[ERROR] " << "BindInterfaceToLua error pKaguyaState is NULL"<< std::endl;
		return;
	}
	// global
	(*pKaguyaState)["CGlobal"].setClass(kaguya::UserdataMetatable<CGlobal>()
		.setConstructors<CGlobal()>()
		.addStaticFunction("Stop", &CGlobal::Stop)
		.addStaticField("STATIC_ON_ACCEPT", CGlobal::STATIC_ON_ACCEPT)
		.addStaticField("STATIC_ON_DATA", CGlobal::STATIC_ON_DATA)
		.addStaticField("STATIC_ON_DISCONNECT", CGlobal::STATIC_ON_DISCONNECT)
	);
	// server connector
	(*pKaguyaState)["CConnector"].setClass(kaguya::UserdataMetatable<CConnector>()
		.setConstructors<CConnector(CServer*)>()
		.addFunction("Start", &CConnector::Start)
		.addFunction("Send", &CConnector::Send)
		.addFunction("GetSocket", &CConnector::GetSocket)
		.addFunction("GetIP", &CConnector::GetIP)
	);
	// server
	(*pKaguyaState)["CServer"].setClass(kaguya::UserdataMetatable<CServer>()
		.setConstructors<CServer(unsigned short)>()
		.addFunction("Stop", &CServer::Stop)
		.addFunction("StartAccept", &CServer::StartAccept)
		.addFunction("EraseConnector", &CServer::EraseConnector)
		.addFunction("GetAcceptor", &CServer::GetAcceptor)
		.addFunction("RegCallBack", &CServer::RegCallBack)
	);
	// client
	(*pKaguyaState)["CClient"].setClass(kaguya::UserdataMetatable<CClient>()
		.setConstructors<CClient()>()
		.addFunction("Connect", &CClient::Connect)
		.addFunction("Send", &CClient::Send)
		.addFunction("Stop", &CClient::Stop)
		.addFunction("RegCallBack", &CClient::RegCallBack)
	);
	// mongodb
	(*pKaguyaState)["CMongoCollection"].setClass(kaguya::UserdataMetatable<CMongoCollection>()
		.setConstructors<CMongoCollection(mongoc_client_t *, std::string, std::string, bool &)>()
		.addFunction("Insert", &CMongoCollection::Insert)
		.addFunction("Update", &CMongoCollection::Update)
		.addFunction("Delete", &CMongoCollection::Delete)
		.addFunction("Fetch", &CMongoCollection::Fetch)
		.addFunction("Drop", &CMongoCollection::Drop)
		.addFunction("EnsureIndex", &CMongoCollection::EnsureIndex)
	);
	(*pKaguyaState)["CMongo"].setClass(kaguya::UserdataMetatable<CMongo>()
		.setConstructors<CMongo(std::string, unsigned int, std::string, std::string)>()
		.addFunction("GetDB", &CMongo::GetDB)
		.addFunction("GetCollection", &CMongo::GetCollection)
	);

	pKaguyaState->openlib("sproto.core", &luaopen_sproto_core);
	pKaguyaState->openlib("lpeg", &luaopen_lpeg);
}
