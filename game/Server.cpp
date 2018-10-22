#include "Server.h"
#include "boost/bind/bind.hpp"

CServer::CServer(unsigned short usPort):
	m_pAcceptor(new boost::asio::ip::tcp::acceptor(*g_pIO))
{
	if (usPort > MAXPORT || usPort <= MINPORT)
	{
		std::cout << "[ERROR] " << "CServer::Listen " << "port invalid" << std::endl;
		return;
	}
	std::cout << "[NORMAL] " << "CServer::CServer " << " init listen on port: " << usPort << std::endl;
	
	boost::system::error_code ec;
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), usPort);
	m_pAcceptor->open(endpoint.protocol());
	m_pAcceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(false));
	m_pAcceptor->bind(endpoint, ec);
	if (ec)
	{
		m_pAcceptor->close();
		std::cout << "[ERROR] " << "CServer::CServer " << ec.message() << std::endl;
		return;
	}
	m_pAcceptor->listen();
}

CServer::~CServer()
{
	Stop();
	delete m_pAcceptor;
	m_pAcceptor = NULL;
}

void CServer::Stop()
{
	std::cout << "[WARNING] " << "CServer::Stop " << "get single term or quit" << std::endl;

	m_pAcceptor->close();

	// 链接清理和通知
	std::set<CConnector *>::iterator it = m_conMgr_set.begin();
	it = m_conMgr_set.begin();
	for (; it != m_conMgr_set.end();)
	{
		it = EraseConnector(*it);
	}
}

void CServer::StartAccept()
{
	if (!m_pAcceptor->is_open())
	{
		std::cout << "[ERROR] " << "CServer::StartAccept server listen error"<< std::endl;
		return ;
	}
	CConnector *p_connector = new CConnector(this); // 创建新的链接对象
	m_pAcceptor->async_accept(
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

	int tmp_iAddress = pConnector->m_iAddress;

	delete pConnector;
	pConnector = NULL;
	it = m_conMgr_set.erase(it); // 返回下一个迭代器

	std::cout << "[NORMAL] " << "connect disconnected and remove" << std::endl;

	std::map<unsigned short, std::string>::iterator it_fun = m_mapCallbackFun.find(CGlobal::ON_DISCONNECT);
	if (it_fun != m_mapCallbackFun.end())
	{
		// 调用lua全局函数 断线通知
		(*g_pKaguyaState)[it_fun->second](tmp_iAddress);
	}

	return it;
}

boost::asio::ip::tcp::acceptor* CServer::GetAcceptor()
{
	return m_pAcceptor;
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
	if (pConnector == NULL || !m_pAcceptor->is_open())
	{
		std::cout << "[NORMAL] " << "CServer::OnAcceptHandle accept listen error" << std::endl;
		delete pConnector;

		return ;
	}
	m_conMgr_set.insert(pConnector);

	pConnector->Start();

	std::map<unsigned short, std::string>::iterator it = m_mapCallbackFun.find(CGlobal::ON_ACCEPT);
	if (it != m_mapCallbackFun.end())
	{
		// 调用lua全局函数 连接通知
		(*g_pKaguyaState)[it->second](pConnector, pConnector->m_iAddress);
	}

	StartAccept();
}

/////////////////////////////////////////////////////

CConnector::CConnector(CServer* p_server):
	m_pserver(p_server)
{
	m_iAddress = reinterpret_cast<int>(this);
	m_pSocket = new boost::asio::ip::tcp::socket(*g_pIO);
}

CConnector::~CConnector()
{
	if (m_pSocket->is_open())
	{
		m_pSocket->close();
	}
	delete m_pSocket;
}

void CConnector::Start()
{
	m_pSocket->async_read_some(boost::asio::buffer(m_buffer_array, sizeof(StruMessageHead)),
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
	if (!m_pSocket->is_open())
	{
		return;
	}
	StruMessageHead *pMsghead = (StruMessageHead *)g_pSendbuff;
	pMsghead->len = len;
	pMsghead->msgId = msgId;
	memcpy(g_pSendbuff + sizeof(StruMessageHead), pData, len);

	m_pSocket->async_write_some(
		boost::asio::buffer(g_pSendbuff, len + sizeof(StruMessageHead)),
		boost::bind(&CConnector::SendHandle, this, boost::asio::placeholders::error)
	);
}

boost::asio::ip::tcp::socket* CConnector::GetSocket()
{
	return m_pSocket;
}

void CConnector::Stop()
{
	if (m_pSocket->is_open())
	{
		m_pSocket->close();
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
	
	m_pSocket->async_read_some(boost::asio::buffer(m_buffer_array, pMsg->len),
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
		(*g_pKaguyaState)[it->second](m_iAddress, msgId, str);
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
	if (m_pSocket->is_open())
	{
		return m_pSocket->remote_endpoint().address().to_string();
	}
	return "";
}