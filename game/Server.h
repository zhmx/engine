#pragma once

#include "Global.h"
#include <set>
#include "boost/array.hpp"

class CServer;

class CConnector
{
public:
	CConnector(CServer* p_server);
	~CConnector();

	void Start();
	void Send(CMDID_LEN_TYPE msgId, const char* pData, unsigned int len);
	boost::asio::ip::tcp::socket* GetSocket();
	std::string GetIP();


public:
	int m_iAddress;


private:
	// 这个方法一调用后 该对象就要被舍弃 不能再使用这个类对象
	void Stop();
	void HeadReadHandle(const boost::system::error_code& err, std::size_t bytes_transferred);
	void BodyReadHandle(const boost::system::error_code& err, std::size_t bytes_transferred, CMDID_LEN_TYPE& msgId);
	void SendHandle(boost::system::error_code err);

	CServer* m_pserver;
	boost::asio::ip::tcp::socket* m_pSocket;
	boost::array<char, MAX_BUFF_SIZE> m_buffer_array;
};

class CServer
{
public:
	CServer(unsigned short usPort);
	~CServer();

	void Stop();
	void StartAccept();
	std::set<CConnector*>::iterator EraseConnector(CConnector* pConnector);
	boost::asio::ip::tcp::acceptor* GetAcceptor();
	bool RegCallBack(unsigned short type, std::string strCallbackFun);


	std::map<unsigned short, std::string> m_mapCallbackFun;

private:
	void OnAcceptHandle(const boost::system::error_code& e, CConnector* pConnector);

	boost::asio::ip::tcp::acceptor *m_pAcceptor;
	std::set<CConnector*> m_conMgr_set;
};

