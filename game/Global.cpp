#include "Global.h"
#include "md5/md5.h"

kaguya::State g_kaguyaState;
lua_State *g_luaState = g_kaguyaState.state();
boost::asio::io_context g_IO;
boost::asio::deadline_timer g_timer(g_IO);
boost::asio::signal_set g_single(g_IO);
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
	g_timer.cancel();
	g_IO.stop();
}

std::string CGlobal::md5(const char* pStr)
{
	if (pStr == NULL)
	{
		return "";
	}
	return MD5(pStr).toStr();
}