#pragma once

#include "Global.h"
#include "mongoc.h"

class CMongoCollection
{
public:
	CMongoCollection(mongoc_client_t *pClient, std::string dbname, std::string collection, bool &succ);
	~CMongoCollection();
	bool Insert(const char* pJsonStrData);
	bool Update(const char* pJsonStrCondition, const char* pJsonStrData, bool bUpser = false);
	bool Delete(const char* pJsonStrCondition);
	std::string Fetch(const char* pJsonStrCondition);
	bool Drop();
	bool EnsureIndex(const char* pJsonKey);
private:
	mongoc_collection_t* m_pCollection;
	std::stringstream m_ss;
	std::string m_dbname;
	std::string m_collectionname;
	mongoc_client_t *m_pClient;
};

class CMongo
{
public:
	CMongo(std::string host, unsigned int port, std::string user, std::string password);
	~CMongo();
	
	mongoc_database_t* GetDB(std::string dbname);
	CMongoCollection* GetCollection(std::string dbname, std::string collection);
private:
	std::string m_url;
	mongoc_uri_t *m_pUri;
	mongoc_client_t *m_pClient;
	std::map<std::string, mongoc_database_t*> m_dbMap;
	std::map<std::string, CMongoCollection*> m_collectionMap;
	bson_error_t m_error;
};