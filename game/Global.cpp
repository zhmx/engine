#include "Global.h"
#include "md5/md5.h"

kaguya::State *g_pKaguyaState = new kaguya::State;
lua_State *g_luaState = g_pKaguyaState->state();
boost::asio::io_context *g_pIO = new boost::asio::io_context;
boost::asio::deadline_timer *g_pTimer = new boost::asio::deadline_timer(*g_pIO);
boost::asio::signal_set g_single(*g_pIO);
std::string g_strSecTimer("OnSecTimer");
char *g_pSendbuff = new char[MAX_BUFF_SIZE];


unsigned char CGlobal::STATIC_ON_ACCEPT = CGlobal::ON_ACCEPT;
unsigned char CGlobal::STATIC_ON_DATA = CGlobal::ON_DATA;
unsigned char CGlobal::STATIC_ON_DISCONNECT = CGlobal::ON_DISCONNECT;

CGlobal::CGlobal()
{

}

CGlobal::~CGlobal()
{

}

void CGlobal::Stop()
{
	std::cout << "[DEBUG] " << "CGlobal::Stop function called" << std::endl;
	if (g_pTimer != NULL)
	{
		g_pTimer->cancel();
		delete g_pTimer;
		g_pTimer = NULL;
	}
	if (!g_pIO->stopped())
	{
		g_pIO->stop();
	}
}

std::string CGlobal::md5(const char* pStr)
{
	if (pStr == NULL)
	{
		return "";
	}
	return MD5(pStr).toStr();
}