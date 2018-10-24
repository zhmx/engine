#include "Client.h"
#include "boost/bind/bind.hpp"

CClient::CClient()
	:m_socket(g_IO)
{

}


CClient::~CClient()
{
}

bool CClient::Connect(std::string host, unsigned short usPort)
{
	if (host.empty())
	{
		host = "127.0.0.1";
	}
	if (usPort > MAXPORT || usPort < MINPORT)
	{
		std::cout << "[ERROR] " << "CClient::Connect " << "port invalid" << std::endl;
		return false;
	}
	std::cout << "[NORMAL] " << "connect to host: " << host << " port: " << usPort  << std::endl;

	boost::asio::ip::tcp::resolver _resolver(g_IO);
	boost::asio::async_connect(
		m_socket,
		_resolver.resolve(host, std::to_string(usPort)),
		boost::bind(&CClient::ConnectHandle, this, boost::asio::placeholders::error)
	);

	//boost::system::error_code err;
	//if (err.value() != 0)
	//{
	//	std::cout << "[ERROR] " << "CClient::Connect \n" << err.message() << std::endl;
	//	return false;
	//}

	return true;
}

void CClient::Send(CMDID_LEN_TYPE msgId, const char* pData, BODY_LEN_TYPE len)
{
	if (pData == NULL)
	{
		std::cout << "[ERROR] " << "CClient::Send " << "data error" << std::endl;
		return;
	}
	if (len > MAX_BUFF_SIZE - sizeof(StruMessageHead) || len == 0)
	{
		std::cout << "[ERROR] " << "CClient::Send " << len << " data length limit" << std::endl;
		return ;
	}
	if (!m_socket.is_open())
	{
		return;
	}
	StruMessageHead *pMsghead = (StruMessageHead *)g_pSendbuff;
	pMsghead->len = len;
	pMsghead->msgId = msgId;
	memcpy(g_pSendbuff + sizeof(StruMessageHead), pData, len);

	m_socket.async_write_some(
		boost::asio::buffer(g_pSendbuff, len + sizeof(StruMessageHead)),
		boost::bind(&CClient::SendHandle, this, boost::asio::placeholders::error)
	);
}

void CClient::Stop()
{
	if (m_socket.is_open())
	{
		m_socket.close();
	}
	std::map<unsigned short, std::string>::iterator it_fun = m_mapCallbackFun.find(CGlobal::ON_DISCONNECT);
	if (it_fun != m_mapCallbackFun.end())
	{
		// 调用lua全局函数 断线通知
		g_kaguyaState[it_fun->second]();
	}
}

bool CClient::RegCallBack(unsigned short type, std::string strCallbackFun)
{
	if (type >= CGlobal::TYPEMAX || type <= CGlobal::TYPEMIN)
	{
		std::cout << "[ERROR] " << "CClient::RegCallBack " << "type error" << std::endl;
		return false;
	}
	if (strCallbackFun.empty())
	{
		std::cout << "[ERROR] " << "CClient::RegCallBack " << "call back function error" << std::endl;
		return false;
	}
	m_mapCallbackFun[type] = strCallbackFun;

	return true;
}

void CClient::ConnectHandle(const boost::system::error_code& err)
{
	if (err)
	{
		Stop();
		return ;
	}

	std::map<unsigned short, std::string>::iterator it_fun = m_mapCallbackFun.find(CGlobal::ON_ACCEPT);
	if (it_fun != m_mapCallbackFun.end())
	{
		// 调用lua全局函数 连接通知
		kaguya::FunctionResults result = g_kaguyaState[it_fun->second]();
	}

	m_socket.async_read_some(boost::asio::buffer(m_buffer_array, sizeof(StruMessageHead)),
		boost::bind(&CClient::HeadReadHandle, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

}

void CClient::HeadReadHandle(const boost::system::error_code& err, std::size_t bytes_transferred)
{
	if (err)
	{
		std::cout << "[ERROR] " << "CClient::HeadReadHandle " << "connect error" << std::endl;
		Stop();
		return ;
	}
	else if (bytes_transferred == 0)
	{
		std::cout << "[ERROR] " << "CClient::HeadReadHandle " << "byte read error" << std::endl;
		Stop();
		return;
	}

	StruMessageHead *pMsg = (StruMessageHead *)m_buffer_array.data();
	if (pMsg->len > MAX_BUFF_SIZE || pMsg->len == 0)
	{
		std::cout << "[ERROR] " << "CClient::HeadReadHandle " << pMsg->len << " data length limited" << std::endl;
		Stop();
		return;
	}

	m_socket.async_read_some(boost::asio::buffer(m_buffer_array, pMsg->len),
		boost::bind(&CClient::BodyReadHandle, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, pMsg->msgId));
}

void CClient::BodyReadHandle(const boost::system::error_code& err, std::size_t bytes_transferred, CMDID_LEN_TYPE& msgId)
{
	if (err)
	{
		std::cout << "[ERROR] " << "CClient::BodyReadHandle " << "connect error" << std::endl;
		Stop();
		return;
	}
	else if (bytes_transferred == 0)
	{
		std::cout << "[ERROR] " << "CClient::BodyReadHandle " << "byte read error" << std::endl;
		Stop();
		return;
	}

	// 读取正常 m_buffer_array.data()
	std::map<unsigned short, std::string>::iterator it = m_mapCallbackFun.find(CGlobal::ON_DATA);
	if (it != m_mapCallbackFun.end())
	{
		// 调用lua全局函数 数据通知
		std::string str(m_buffer_array.data(), bytes_transferred);
		g_kaguyaState[it->second](msgId, str);
	}

	m_socket.async_read_some(boost::asio::buffer(m_buffer_array, sizeof(StruMessageHead)),
		boost::bind(&CClient::HeadReadHandle, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void CClient::SendHandle(boost::system::error_code err)
{
	if (err)
	{
		std::cout << "[ERROR] " << "CClient::SendHandle error" << std::endl;
	}
}