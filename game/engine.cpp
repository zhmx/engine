#include "Global.h"
#include "boost/bind/bind.hpp"

void BindInterfaceToLua(kaguya::State *pKaguyaState);

void SecTimer(boost::system::error_code ec)
{
	if (ec)
	{
		std::cout << "[ERROR] " << "SecTimer error " << ec.message() << std::endl;
		if (g_pIO && !g_pIO->stopped())
		{
			g_pIO->stop();
		}
		return ;
	}
	if (g_pTimer != NULL)
	{
		g_pTimer->expires_from_now(boost::posix_time::seconds(1));
		g_pTimer->async_wait(SecTimer);
		// 调用lua全局函数
		(*g_pKaguyaState)[g_strSecTimer]();
	}
}


int main(int argc, char* argv[])
{
	g_single.add(SIGINT);
	g_single.add(SIGTERM);
#if defined(SIGQUIT)
	g_single.add(SIGQUIT);
#endif // defined(SIGQUIT)
	g_single.async_wait(boost::bind(&CGlobal::Stop));
	
	//// 数据初始化
	memset(g_pSendbuff, 0, MAX_BUFF_SIZE); // 发送缓冲区
	mongoc_init(); // mongo 库环境


	//// cpp to lua
	BindInterfaceToLua(g_pKaguyaState);

	//// lua script execute
	g_pKaguyaState->dofile("main.lua");

	// g_pTimer->expires_from_now(boost::posix_time::milliseconds(400));
	g_pTimer->expires_from_now(boost::posix_time::seconds(1));
	g_pTimer->async_wait(boost::bind(SecTimer, boost::asio::placeholders::error));
	//boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
	//boost::posix_time::time_duration td = now.time_of_day();
	//boost::posix_time::ptime now2 = boost::posix_time::microsec_clock::local_time();
	//boost::posix_time::time_duration td2 = now2.time_of_day();
	//std::cout << "cost " << td2.total_milliseconds() - td.total_milliseconds() << std::endl;
	/*for (int i = 0; i < 1000000; ++i)
	{
		(*g_pKaguyaState)[g_strSecTimer]();
	}*/
	g_pIO->run();


	// 退出后资源的释放
	mongoc_cleanup();

	if (g_pTimer != NULL)
	{
		delete g_pTimer;
		g_pTimer = NULL;
	}

	delete[] g_pSendbuff;
	g_pSendbuff = NULL;

	delete g_pKaguyaState;
	g_pKaguyaState = NULL;

	std::cout << "[NORMAL] " << "APP EXIT! please enter any character ...";

	int iTmp = getchar(); // pause
	delete g_pIO;
	g_pIO = NULL;

	return 0;
}