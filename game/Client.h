#pragma once

#include "Global.h"
#include "boost/array.hpp"

class CClient
{
public:
	CClient();
	~CClient();

	bool Connect(std::string host, unsigned short usPort);
	void Send(CMDID_LEN_TYPE msgId, const char* pData, BODY_LEN_TYPE len);
	void Stop();
	bool RegCallBack(unsigned short type, std::string strCallbackFun);

private:
	void ConnectHandle(const boost::system::error_code& err);
	void HeadReadHandle(const boost::system::error_code& err, std::size_t bytes_transferred);
	void BodyReadHandle(const boost::system::error_code& err, std::size_t bytes_transferred, CMDID_LEN_TYPE& msgId);
	void SendHandle(boost::system::error_code err);

	boost::asio::ip::tcp::socket m_socket;
	boost::array<char, MAX_BUFF_SIZE> m_buffer_array;
	std::map<unsigned short, std::string> m_mapCallbackFun;
};

