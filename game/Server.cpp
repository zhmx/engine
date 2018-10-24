#include "Server.h"
#include "boost/bind/bind.hpp"

CServer::CServer():
	m_acceptor(g_IO), m_usPort(0)
{
}

CServer::~CServer()
{
	Stop();
}

void CServer::Stop()
{
	std::cout << "[WARNING] " << "CServer::Stop " << "get single term or quit" << std::endl;

	m_acceptor.close();

	// 链接清理和通知
	std::set<CConnector *>::iterator it = m_conMgr_set.begin();
	for (; it != m_conMgr_set.end();)
	{
		it = EraseConnector(*it);
	}
}

void CServer::StartListen(unsigned short usPort)
{
	if (usPort == 0)
	{
		usPort = m_usPort; // 重新监听默认使用上一次的监听端口
	}
	if (usPort > MAXPORT || usPort <= MINPORT)
	{
		std::cout << "[ERROR] " << "CServer::StartListen " << "port invalid" << std::endl;
		return;
	}
	m_usPort = usPort;
	std::cout << "[NORMAL] " << "CServer::StartListen " << " init listen on port: " << usPort << std::endl;

	boost::system::error_code ec;
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), usPort);
	m_acceptor.open(endpoint.protocol());
	m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(false));
	m_acceptor.bind(endpoint, ec);
	if (ec)
	{
		m_acceptor.close();
		std::cout << "[ERROR] " << "CServer::StartListen " << ec.message() << std::endl;
		return;
	}
	m_acceptor.listen();
	StartAccept();
}

void CServer::StartAccept()
{
	if (!m_acceptor.is_open())
	{
		std::cout << "[ERROR] " << "CServer::StartAccept " << "acceptor has not open" << std::endl;
		return;
	}
	CConnector *p_connector = new CConnector(this); // 创建新的链接对象
	m_acceptor.async_accept(
		*(p_connector->GetSocket()),
		boost::bind(&CServer::OnAcceptHandle, this, boost::asio::placeholders::error, p_connector)
	);
}

std::set<CConnector*>::iterator CServer::EraseConnector(CConnector* pConnector)
{
	if (pConnector == NULL)
	{
		return m_conMgr_set.end();
	}
	std::set<CConnector*>::iterator it = m_conMgr_set.find(pConnector);
	if (it == m_conMgr_set.end())
	{
		return m_conMgr_set.end();
	}

	int tmp_iAddress = pConnector->GetAddress();

	delete pConnector;
	pConnector = NULL;
	it = m_conMgr_set.erase(it); // 返回下一个迭代器

	std::cout << "[NORMAL] " << "connect disconnected and remove" << std::endl;

	std::map<unsigned short, std::string>::iterator it_fun = m_mapCallbackFun.find(CGlobal::ON_DISCONNECT);
	if (it_fun != m_mapCallbackFun.end())
	{
		// 调用lua全局函数 断线通知
		g_kaguyaState[it_fun->second](tmp_iAddress);
	}

	return it;
}

boost::asio::ip::tcp::acceptor* CServer::GetAcceptor()
{
	return &m_acceptor;
}

bool CServer::RegCallBack(unsigned short type, std::string strCallbackFun)
{
	if (type >= CGlobal::TYPEMAX || type <= CGlobal::TYPEMIN)
	{
		std::cout << "[ERROR] " << "CServer::RegCallBack " << "type error" << std::endl;
		return false;
	}
	if (strCallbackFun.empty())
	{
		std::cout << "[ERROR] " << "CServer::RegCallBack " << "call back function error" << std::endl;
		return false;
	}
	m_mapCallbackFun[type] = strCallbackFun;
	
	return true;
}

void CServer::OnAcceptHandle(const boost::system::error_code& e, CConnector* pConnector)
{
	if (e)
	{
		std::cout << "[NORMAL] " << "CServer::OnAcceptHandle accept listen error" << std::endl;
		delete pConnector;

		return ;
	}
	m_conMgr_set.insert(m_conMgr_set.begin(), pConnector);

	pConnector->Start();

	std::map<unsigned short, std::string>::iterator it = m_mapCallbackFun.find(CGlobal::ON_ACCEPT);
	if (it != m_mapCallbackFun.end())
	{
		// 调用lua全局函数 连接通知
		g_kaguyaState[it->second](pConnector);
	}

	StartAccept();
}

/////////////////////////////////////////////////////

CConnector::CConnector(CServer* p_server):
	m_pserver(p_server), m_socket(g_IO)
{
	m_iAddress = reinterpret_cast<int>(this);
}

CConnector::~CConnector()
{
	if (m_socket.is_open())
	{
		m_socket.close();
	}
}

void CConnector::Start()
{
	m_socket.async_read_some(boost::asio::buffer(m_buffer_array, sizeof(StruMessageHead)),
		boost::bind(&CConnector::HeadReadHandle, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void CConnector::Send(CMDID_LEN_TYPE msgId, const char* pData, unsigned int len)
{
	if (pData == NULL)
	{
		std::cout << "[ERROR] " << "CClient::Send " << "data error" << std::endl;
		return;
	}
	if (len > MAX_BUFF_SIZE - sizeof(StruMessageHead) || len == 0)
	{
		std::cout << "[ERROR] " << "CClient::Send " << len << " data length limit" << std::endl;
		return;
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
		boost::bind(&CConnector::SendHandle, this, boost::asio::placeholders::error)
	);
}

boost::asio::ip::tcp::socket* CConnector::GetSocket()
{
	return &m_socket;
}

void CConnector::Stop()
{
	if (m_socket.is_open())
	{
		m_socket.close();
	}
	m_pserver->EraseConnector(this);
}

void CConnector::HeadReadHandle(const boost::system::error_code& err, std::size_t bytes_transferred)
{
	if (err)
	{
		// 读取错误 或者断线
		Stop();
		return ;
	}
	else if (bytes_transferred != sizeof(StruMessageHead))
	{
		std::cout << "[ERROR] " << "CConnector::HeadReadHandle " << "byte read error" << std::endl;
		Stop();
		return;
	}

	StruMessageHead *pMsg = (StruMessageHead *)m_buffer_array.data();
	if (pMsg->len > MAX_BUFF_SIZE || pMsg->len == 0)
	{
		std::cout << "[ERROR] " << "CConnector::HeadReadHandle " << pMsg->len << " data length limited msgId: " << pMsg->msgId << std::endl;
		Stop();
		return ;
	}
	
	m_socket.async_read_some(boost::asio::buffer(m_buffer_array, pMsg->len),
			boost::bind(&CConnector::BodyReadHandle, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, pMsg->msgId));
}
void CConnector::BodyReadHandle(const boost::system::error_code& err, std::size_t bytes_transferred, CMDID_LEN_TYPE& msgId)
{
	if (err)
	{
		std::cout << "[ERROR] " << "CConnector::BodyReadHandle " << "connect error" << std::endl;
		Stop();
		return;
	}
	else if (bytes_transferred == 0)
	{
		std::cout << "[ERROR] " << "CConnector::BodyReadHandle " << "byte read error" << std::endl;
		Stop();
		return;
	}

	// 读取正常
	std::map<unsigned short, std::string>::iterator it = m_pserver->m_mapCallbackFun.find(CGlobal::ON_DATA);
	if (it != m_pserver->m_mapCallbackFun.end())
	{
		// 调用lua全局函数 数据通知
		std::string str(m_buffer_array.data(), bytes_transferred);
		g_kaguyaState[it->second](m_iAddress, msgId, str);
	}

	// 继续监听链接
	Start();
}

void CConnector::SendHandle(boost::system::error_code err)
{
	if (err)
	{
		std::cout << "[ERROR] " << "CConnector::SendHandle " << " MSG:" << err.message() << std::endl;
	}
}

std::string CConnector::GetIP()
{
	if (m_socket.is_open())
	{
		return m_socket.remote_endpoint().address().to_string();
	}
	return "";
}

int CConnector::GetAddress()
{
	return m_iAddress;
}