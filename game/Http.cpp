#include "Http.h"
#include "boost/bind/bind.hpp"



CHttp::CHttp()
	:m_resolver(*g_pIO),
	m_socket(*g_pIO),
	m_request_stream(&m_request),
	m_timer(*g_pIO, boost::posix_time::seconds(WEB_HTTP_TIMEOUT))
{
}


CHttp::~CHttp()
{
}

void CHttp::GetPost(std::string strType, std::string strUrl, std::string strPage, std::string strData)
{

	//GET /?tn=93380420_hao_pg HTTP/1.1
	//Host: www.baidu.com
	//Connection: keep-alive
	//Cache-Control: max-age=0
	//Upgrade-Insecure-Requests: 1
	//User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.81 Safari/537.36
	//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8
	//Accept-Encoding: gzip, deflate, br
	//Accept-Language: zh-CN,zh;q=0.9
	//Cookie: PSTM=1524126443; BD_UPN=12314353; BIDUPSID=158BF6F9218FBFFCCAE2461E04BABA3E; __cfduid=d01bcd0a576735ee513d21c429c855f391524198896; BAIDUID=64548E0E336578924FD0E0D78A02A6BE:FG=1; BDUSS=5hVjFLOU50bXlEQWRKWERndXUwRmxEcnFMT3YxS2pCNm0wUDdNeGMyYWpkYWRiQVFBQUFBJCQAAAAAAAAAAAEAAADJLKkLTUqC98jLAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAKPof1uj6H9bc3; ispeed_lsm=2; H_PS_PSSID=; H_PS_645EC=7d52EMLiIY1me6x7sNN7Tez08umsFIRGOHyRbc08XOYU5rA9rnH5MTLBUhc%2BoATIjuu64zbH; delPer=0; BD_CK_SAM=1; PSINO=7; BDRCVFR[-2cV2yayoy0]=mk3SLVN4HKm; BD_HOME=1
	if (m_socket.is_open())
	{
		m_socket.close();
		m_timer.cancel();
	}
	m_header = "";
	m_response.consume(m_response.size()+1); //If n is greater than the size of the input sequence, the entire input sequence is consumed and no error is issued.

	m_timer.async_wait(boost::bind(&CHttp::OnClose, this, "time out"));

	m_request_stream.flush();
	if (strType == "POST")
	{
		m_request_stream << "POST " << strPage << " HTTP/1.0\r\n";
		m_request_stream << "Host: " << strUrl << "\r\n";
		m_request_stream << "Accept: */*\r\n";
		m_request_stream << "Content-Length: " << strData.length() << "\r\n";
		m_request_stream << "Content-Type: application/x-www-form-urlencoded\r\n";
		m_request_stream << "Connection: close\r\n\r\n";
		m_request_stream << strData;
	}
	else
	{
		m_request_stream << "GET " << strPage << " HTTP/1.0\r\n";
		m_request_stream << "Host: " << strUrl << "\r\n";
		m_request_stream << "Accept: */*\r\n";
		m_request_stream << "Connection: close\r\n\r\n";
	}

	m_resolver.async_resolve(strUrl, "http",
		boost::bind(&CHttp::handle_get_resolve, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::results));
}

void CHttp::Stop()
{
	m_header = "";
	m_response.consume(m_response.size() + 1); //If n is greater than the size of the input sequence, the entire input sequence is consumed and no error is issued.
	if (m_socket.is_open())
	{
		m_socket.close();
		m_timer.cancel();
	}
}

bool CHttp::RegCallBack(unsigned short type, std::string strCallbackFun)
{
	if (type >= CGlobal::TYPEMAX || type <= CGlobal::TYPEMIN)
	{
		std::cout << "[ERROR] " << "CHttp::RegCallBack " << "type error" << std::endl;
		return false;
	}
	if (strCallbackFun.empty())
	{
		std::cout << "[ERROR] " << "CHttp::RegCallBack " << "call back function error" << std::endl;
		return false;
	}
	m_mapCallbackFun[type] = strCallbackFun;

	return true;
}

int CHttp::GetAddress()
{
	return reinterpret_cast<int>(this);
}

void CHttp::handle_get_resolve(const boost::system::error_code& err, const boost::asio::ip::tcp::resolver::results_type& endpoints)
{
	if (!err)
	{
		boost::asio::async_connect(m_socket, endpoints,
			boost::bind(&CHttp::handle_get_connect, this,
				boost::asio::placeholders::error));
	}
	else
	{
		//std::cout << "[ERROR] " << "CHttp::handle_get_resolve " << err.message() << std::endl;
		OnClose(err.message());
	}
}

void CHttp::handle_get_connect(const boost::system::error_code& err)
{
	if (!err)
	{
		boost::asio::async_write(m_socket, m_request,
			boost::bind(&CHttp::handle_get_write_request, this,
				boost::asio::placeholders::error));
	}
	else
	{
		//std::cout << "[ERROR] " << "CHttp::handle_get_connect " << err.message() << std::endl;
		OnClose(err.message());
	}
}

void CHttp::handle_get_write_request(const boost::system::error_code& err)
{
	if (!err)
	{
		boost::asio::async_read_until(m_socket, m_response, "\r\n",
			boost::bind(&CHttp::handle_get_read_status_line, this,
				boost::asio::placeholders::error));
	}
	else
	{
		//std::cout << "[ERROR] " << "CHttp::handle_get_write_request " << err.message() << std::endl;
		OnClose(err.message());
	}
}

void CHttp::handle_get_read_status_line(const boost::system::error_code& err)
{
	if (!err)
	{
		//std::cout << &m_response << std::endl;
		std::istream response_stream(&m_response);
		std::string http_version;
		response_stream >> http_version;
		unsigned int status_code;
		response_stream >> status_code;
		std::string status_message;
		std::getline(response_stream, status_message);
		if (!response_stream || http_version.substr(0, 5) != "HTTP/")
		{
			//std::cout << "[ERROR] " << "CHttp::handle_get_read_status_line " << "Invalid response" << std::endl;
			OnClose("Invalid response");
			return;
		}
		if (status_code != 200)
		{
			//std::cout << "[ERROR] " << "CHttp::handle_get_read_status_line " << "Response returned with status code : " << status_code << std::endl;
			OnClose(std::to_string(status_code));
			return;
		}
		//std::cout << &m_response << std::endl;
		//m_response.consume(m_response.size() + 1);
		boost::asio::async_read_until(m_socket, m_response, "\r\n\r\n",
			boost::bind(&CHttp::handle_get_read_headers, this,
				boost::asio::placeholders::error));
	}
	else
	{
		//std::cout << "[ERROR] " << "CHttp::handle_get_read_status_line " << err << std::endl;
		OnClose(err.message());
	}
}

void CHttp::handle_get_read_headers(const boost::system::error_code& err)
{
	if (!err)
	{
		//std::cout << &m_response << std::endl;

		std::istream response_stream(&m_response);
		std::string tmp_str;
		while (std::getline(response_stream, tmp_str) && tmp_str != "\r")
		{
			//std::cout << tmp_str << "\n";
			m_header = m_header + tmp_str + "\n";
		}

		//std::cout << m_header << std::endl;

		//if (m_response.size() > 0)
		//{
		//	std::cout << &m_response;
		//}

		boost::asio::async_read(m_socket, m_response,
			boost::asio::transfer_at_least(1),
			boost::bind(&CHttp::handle_get_read_content, this,
				boost::asio::placeholders::error));
	}
	else
	{
		//std::cout << "[ERROR] " << "CHttp::handle_get_read_headers " << err << std::endl;
		OnClose(err.message());
	}
}

void CHttp::handle_get_read_content(const boost::system::error_code& err)
{
	if (!err)
	{
		//std::cout << &m_response << std::endl;

		//boost::asio::async_read(m_socket, m_response,
		//	boost::asio::transfer_at_least(1),
		//	boost::bind(&CHttp::handle_get_read_content, this,
		//		boost::asio::placeholders::error));

		boost::system::error_code error;
		while (boost::asio::read(m_socket, m_response, boost::asio::transfer_at_least(1), error))
		{
			//std::cout << &m_response;
		}
		//std::cout << &m_response;
		if (error != boost::asio::error::eof)
		{
			OnClose(error.message());
		}
		else
		{
			std::map<unsigned short, std::string>::iterator it = m_mapCallbackFun.find(CGlobal::ON_DATA);
			if (it != m_mapCallbackFun.end())
			{
				// 调用lua全局函数 接收数据通知
				int iAddress = reinterpret_cast<int>(this);
				boost::asio::streambuf::const_buffers_type cbt = m_response.data();
				std::string request_data(boost::asio::buffers_begin(cbt), boost::asio::buffers_end(cbt));
				(*g_pKaguyaState)[it->second](iAddress, m_header, request_data);
			}
		}
	}
	else if (err != boost::asio::error::eof)
	{
		//std::cout << "[ERROR] " << "CHttp::handle_get_read_content " << err << std::endl;
		OnClose(err.message());
	}
}

void CHttp::OnClose(const std::string& msg)
{
	m_header = "";
	m_response.consume(m_response.size() + 1); //If n is greater than the size of the input sequence, the entire input sequence is consumed and no error is issued.
	m_timer.cancel();
	if (m_socket.is_open())
	{
		m_socket.close();
		std::map<unsigned short, std::string>::iterator it = m_mapCallbackFun.find(CGlobal::ON_DISCONNECT);
		if (it != m_mapCallbackFun.end())
		{
			// 调用lua全局函数 超时或者异常通知
			int iAddress = reinterpret_cast<int>(this);
			(*g_pKaguyaState)[it->second](iAddress, msg);
		}
	}
}