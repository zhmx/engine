#pragma once

#include <iostream>
#include "boost/asio.hpp"
#include "kaguya/kaguya.hpp"
#include "kaguya/another_binding_api.hpp"
#include "mongoc.h"
#include "bson/bcon.h"

extern kaguya::State *g_pKaguyaState;
extern lua_State *g_luaState;
extern boost::asio::io_context *g_pIO;
extern boost::asio::deadline_timer *g_pTimer;
extern boost::asio::signal_set g_single;
extern std::string g_strSecTimer;
extern char* g_pSendbuff;

// ���ݰ� ͷ(���ݰ�����)+����
typedef unsigned int BODY_LEN_TYPE;	// ���ݰ�����
typedef unsigned int CMDID_LEN_TYPE;// ��Ϣid����

#define	MAX_BUFF_SIZE 102400	// ����&���ͻ�������С 1024*100 100k
#define MAXPORT 65535			// �������˿�
#define MINPORT	80				// ������С�˿�
#define MIN_SEND_SIZE 2			// ��С�����ֽ�


struct StruMessageHead
{
	StruMessageHead() :len(0), msgId(0)
	{}
	~StruMessageHead()
	{}

	BODY_LEN_TYPE len;		// ��Ϣ��ĳ���
	CMDID_LEN_TYPE msgId;	// ��Ϣid
};

class CGlobal
{
public:
	enum
	{
		TYPEMIN,

		ON_ACCEPT,
		ON_DATA,
		ON_DISCONNECT,

		TYPEMAX,
	};

	CGlobal();
	~CGlobal();

	static void Stop();
	static unsigned char STATIC_ON_ACCEPT;
	static unsigned char STATIC_ON_DATA;
	static unsigned char STATIC_ON_DISCONNECT;
};