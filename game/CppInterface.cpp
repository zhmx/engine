
#include "Global.h"
#include "Server.h"
#include "Client.h"
#include "DB.h"
#include "Http.h"
#include "WebSocket.h"

extern "C"
{
	int luaopen_sproto_core(lua_State *L);
	int luaopen_lpeg(lua_State *L);
}

void BindInterfaceToLua(kaguya::State& kaguyaState)
{
	// global
	kaguyaState["CGlobal"].setClass(kaguya::UserdataMetatable<CGlobal>()
		.setConstructors<CGlobal()>()
		.addStaticFunction("Stop", &CGlobal::Stop)
		.addStaticFunction("md5", &CGlobal::md5)
		.addStaticField("STATIC_ON_ACCEPT", CGlobal::STATIC_ON_ACCEPT)
		.addStaticField("STATIC_ON_DATA", CGlobal::STATIC_ON_DATA)
		.addStaticField("STATIC_ON_DISCONNECT", CGlobal::STATIC_ON_DISCONNECT)
	);
	// server connector
	kaguyaState["CConnector"].setClass(kaguya::UserdataMetatable<CConnector>()
		.setConstructors<CConnector(CServer*)>()
		.addFunction("Start", &CConnector::Start)
		.addFunction("Send", &CConnector::Send)
		.addFunction("GetSocket", &CConnector::GetSocket)
		.addFunction("GetIP", &CConnector::GetIP)
		.addFunction("GetAddress", &CConnector::GetAddress)
	);
	// server
	kaguyaState["CServer"].setClass(kaguya::UserdataMetatable<CServer>()
		.setConstructors<CServer()>()
		.addFunction("StartListen", &CServer::StartListen)
		.addFunction("Stop", &CServer::Stop)
		.addFunction("EraseConnector", &CServer::EraseConnector)
		.addFunction("GetAcceptor", &CServer::GetAcceptor)
		.addFunction("RegCallBack", &CServer::RegCallBack)
	);
	// client
	kaguyaState["CClient"].setClass(kaguya::UserdataMetatable<CClient>()
		.setConstructors<CClient()>()
		.addFunction("Connect", &CClient::Connect)
		.addFunction("Send", &CClient::Send)
		.addFunction("Stop", &CClient::Stop)
		.addFunction("RegCallBack", &CClient::RegCallBack)
	);
	// mongodb
	kaguyaState["CMongoCollection"].setClass(kaguya::UserdataMetatable<CMongoCollection>()
		.setConstructors<CMongoCollection(mongoc_client_t *, std::string, std::string, bool &)>()
		.addFunction("Insert", &CMongoCollection::Insert)
		.addFunction("Update", &CMongoCollection::Update)
		.addFunction("Delete", &CMongoCollection::Delete)
		.addFunction("Fetch", &CMongoCollection::Fetch)
		.addFunction("Drop", &CMongoCollection::Drop)
		.addFunction("EnsureIndex", &CMongoCollection::EnsureIndex)
	);
	kaguyaState["CMongo"].setClass(kaguya::UserdataMetatable<CMongo>()
		.setConstructors<CMongo(std::string, unsigned int, std::string, std::string)>()
		.addFunction("GetDB", &CMongo::GetDB)
		.addFunction("GetCollection", &CMongo::GetCollection)
	);
	// web http post get
	kaguyaState["CHttp"].setClass(kaguya::UserdataMetatable<CHttp>()
		.setConstructors<CHttp()>()
		.addFunction("GetPost", &CHttp::GetPost)
		.addFunction("RegCallBack", &CHttp::RegCallBack)
		.addFunction("Stop", &CHttp::Stop)
		.addFunction("GetAddress", &CHttp::GetAddress)
	);
	// websocket session
	kaguyaState["WSSession"].setClass(kaguya::UserdataMetatable<WSSession>()
		.setConstructors<WSSession()>()
		.addFunction("Run", &WSSession::Run)
	);
	// websocket
	kaguyaState["CWebSocket"].setClass(kaguya::UserdataMetatable<CWebSocket>()
		.setConstructors<CWebSocket()>()
		.addFunction("Start", &CWebSocket::Start)
	);


	kaguyaState.openlib("sproto.core", &luaopen_sproto_core);
	kaguyaState.openlib("lpeg", &luaopen_lpeg);
}
