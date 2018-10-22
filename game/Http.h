#pragma once

#include "Global.h"

class CHttp
{
public:
	CHttp();
	virtual ~CHttp();

	void GetPost(std::string strType, std::string strUrl, std::string strPage, std::string strData);
	void Stop();
	bool RegCallBack(unsigned short type, std::string strCallbackFun);
	int GetAddress();

private:
	void handle_get_resolve(const boost::system::error_code& err, const boost::asio::ip::tcp::resolver::results_type& endpoints);
	void handle_get_connect(const boost::system::error_code& err);
	void handle_get_write_request(const boost::system::error_code& err);
	void handle_get_read_status_line(const boost::system::error_code& err);
	void handle_get_read_headers(const boost::system::error_code& err);
	void handle_get_read_content(const boost::system::error_code& err);

	void OnClose(const std::string& msg);


	boost::asio::ip::tcp::resolver m_resolver;
	boost::asio::ip::tcp::socket m_socket;
	boost::asio::streambuf m_request;
	std::string m_header;
	boost::asio::streambuf m_response;

	std::ostream m_request_stream;
	boost::asio::deadline_timer m_timer;

	std::map<unsigned short, std::string> m_mapCallbackFun;
};

