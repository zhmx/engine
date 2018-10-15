#include "DB.h"

CMongo::CMongo(std::string host, unsigned int port, std::string user, std::string password)
	:m_pUri(NULL),
	m_pClient(NULL)
{
	if (user.empty())
	{
		m_url = "mongodb://" + host + ":" + std::to_string(port);
	}
	else
	{
		m_url = "mongodb://" + user + ":" + password + "@" + host + ":" + std::to_string(port);
	}
	bson_t reply;
	bson_error_t error;
	std::cout << "[NORMAL] " << "init mongo connector " << m_url << std::endl;
	m_pUri = mongoc_uri_new_with_error(m_url.c_str(), &m_error);
	if (!m_pUri)
	{
		std::cout << "[ERROR] " << "CMongo::CMongo " << "init " << m_url << " >>\n" << m_error.message << std::endl;
		return ;
	}
	m_pClient = mongoc_client_new_from_uri(m_pUri);
	if (NULL == m_pClient)
	{
		std::cout << "[ERROR] " << "CMongo::CMongo " << "new client error" << std::endl;
		return;
	}
	else if (!mongoc_client_get_server_status(m_pClient, NULL, &reply, &error))
	{
		std::cout << "[ERROR] " << "CMongo::CMongo " << "db status no live >>\n" << error.message << std::endl;
		return;
	}
}

CMongo::~CMongo()
{
	for (std::map<std::string, CMongoCollection*>::iterator collIter = m_collectionMap.begin(); collIter != m_collectionMap.end(); ++collIter)
	{
		delete collIter->second;
	}
	m_collectionMap.clear();
	for (std::map<std::string, mongoc_database_t*>::iterator dbIter = m_dbMap.begin(); dbIter != m_dbMap.end(); ++dbIter)
	{
		mongoc_database_destroy(dbIter->second);
	}
	m_dbMap.clear();
	mongoc_uri_destroy(m_pUri);
	mongoc_client_destroy(m_pClient);
}

mongoc_database_t* CMongo::GetDB(std::string dbname)
{
	mongoc_database_t* pTmp = NULL;
	std::map<std::string, mongoc_database_t*>::iterator dbIter = m_dbMap.find(dbname);
	if (dbIter == m_dbMap.end())
	{
		pTmp = mongoc_client_get_database(m_pClient, dbname.c_str());
	}
	else
	{
		return dbIter->second;
	}
	if (NULL == pTmp)
	{
		std::cout << "[ERROR] " << "CMongo::GetDB " << dbname << " not exit!" << std::endl;
		return NULL;
	}

	return pTmp;
}

CMongoCollection* CMongo::GetCollection(std::string dbname, std::string collection)
{
	std::map<std::string, CMongoCollection*>::iterator collIter = m_collectionMap.find(dbname+collection);
	if (collIter == m_collectionMap.end())
	{
		bool succ = false;
		CMongoCollection* pTmp = new CMongoCollection(m_pClient, dbname, collection, succ);
		if (!succ)
		{
			delete pTmp;
			pTmp = NULL;
			return NULL;
		}
		m_collectionMap[dbname + collection] = pTmp;
		return pTmp;
	}
	else
	{
		return collIter->second;
	}
}

//////////////////////////////

CMongoCollection::CMongoCollection(mongoc_client_t *pClient, std::string dbname, std::string collection, bool &succ)
	:m_pCollection(NULL),m_dbname(dbname),m_collectionname(collection),m_pClient(pClient)
{
	if (pClient == NULL)
	{
		succ = false;
		std::cout << "[ERROR] " << "CMongoCollection::CMongoCollection " << " pClient NULL" << std::endl;
		return ;
	}
	// bson_error_t error;
	m_pCollection = mongoc_client_get_collection(pClient, dbname.c_str(), collection.c_str());
	if (m_pCollection == NULL)
	{
		succ = false;
		std::cout << "[ERROR] " << "CMongoCollection::CMongoCollection " << " get collection error dbname:" << dbname.c_str() << " collection:" << collection.c_str() << std::endl;
		return ;
	}
	//else if (!mongoc_collection_stats(m_pCollection, NULL, NULL, &error))
	//{
	//	succ = false;
	//	std::cout << "[ERROR] " << "CMongoCollection::CMongoCollection >>\n" << error.message << std::endl;
	//	return ;
	//}
	succ = true;
}

CMongoCollection::~CMongoCollection()
{
	if (m_pCollection != NULL)
	{
		mongoc_collection_destroy(m_pCollection);
		m_pCollection = NULL;
	}
}

bool CMongoCollection::Insert(const char* pJsonStrData)
{
	if (NULL == pJsonStrData)
	{
		return false;
	}
	bson_t *bson;
	bson_error_t error;
	bson = bson_new_from_json((const uint8_t *)pJsonStrData, -1, &error);
	if (!bson)
	{
		std::cout << "[ERROR] " << "CMongoCollection::Insert >>\n" << error.message << std::endl;
		return false;
	}
	bool ret = mongoc_collection_insert_one(m_pCollection, bson, NULL, NULL, &error);
	bson_destroy(bson);
	bson = NULL;
	if (!ret)
	{
		std::cout << "[ERROR] " << "CMongoCollection::Insert >>\n" << error.message << std::endl;
		return false;
	}

	return true;
}

bool CMongoCollection::Update(const char* pJsonStrCondition, const char* pJsonStrData, bool bUpser)
{
	if (NULL == pJsonStrCondition || NULL == pJsonStrData)
	{
		std::cout << "[ERROR] " << "CMongoCollection::Update " << "data is empty" << std::endl;
		return false;
	}
	bson_error_t error;
	bson_t *opts = NULL;
	bson_t *bsonCondition = bson_new_from_json((const uint8_t *)pJsonStrCondition, -1, &error);
	if (!bsonCondition)
	{
		std::cout << "[ERROR] " << "CMongoCollection::Update 1 >>\n" << error.message << std::endl;
		return false;
	}
	bson_t *bsonData = bson_new_from_json((const uint8_t *)pJsonStrData, -1, &error);
	if (!bsonData)
	{
		bson_destroy(bsonCondition);
		bsonCondition = NULL;
		std::cout << "[ERROR] " << "CMongoCollection::Update 2 >>\n" << error.message << std::endl;
		return false;
	}

	if (bUpser)
	{
		opts = BCON_NEW("upsert", BCON_BOOL(true));
	}
	else
	{
		opts = BCON_NEW("upsert", BCON_BOOL(false));
	}
	bool ret = mongoc_collection_update_many(m_pCollection, bsonCondition, bsonData, opts, NULL, &error);
	bson_destroy(bsonCondition);
	bsonCondition = NULL;
	bson_destroy(bsonData);
	bsonData = NULL;
	bson_destroy(opts);
	opts = NULL;
	if (!ret)
	{
		std::cout << "[ERROR] " << "CMongoCollection::Update 3 >>\n" << error.message << std::endl;
		return false;
	}
	return true;
}


bool CMongoCollection::Delete(const char* pJsonStrCondition)
{
	if (NULL == pJsonStrCondition)
	{
		std::cout << "[ERROR] " << "CMongoCollection::Delete " << "data is empty" << std::endl;
		return false;
	}
	bson_error_t error;
	bson_t *bsonCondition = bson_new_from_json((const uint8_t *)pJsonStrCondition, -1, &error);
	if (!bsonCondition)
	{
		std::cout << "[ERROR] " << "CMongoCollection::Delete 1 >>\n" << error.message << std::endl;
		return false;
	}
	bool ret = mongoc_collection_delete_many(m_pCollection, bsonCondition, NULL, NULL, &error);
	bson_destroy(bsonCondition);
	bsonCondition = NULL;
	if (!ret)
	{
		std::cout << "[ERROR] " << "CMongoCollection::Delete 2 >>\n" << error.message << std::endl;
		return false;
	}
	return true;
}

std::string CMongoCollection::Fetch(const char* pJsonStrCondition)
{
	m_ss.str("");
	if (pJsonStrCondition == NULL)
	{
		std::cout << "[ERROR] " << "CMongoCollection::Fetch " << "data is empty" << std::endl;
		return m_ss.str();
	}
	bson_error_t error;
	bson_t *bsonCondition = bson_new_from_json((const uint8_t *)pJsonStrCondition, -1, &error);
	if (!bsonCondition)
	{
		std::cout << "[ERROR] " << "CMongoCollection::Delete 1 >>\n" << error.message << std::endl;
		return m_ss.str();
	}
	mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(m_pCollection, bsonCondition, NULL, NULL);
	const bson_t *doc = NULL;
	char *str = NULL;

	while (mongoc_cursor_next(cursor, &doc))
	{
		str = bson_as_json(doc, NULL);
		m_ss << str;
		bson_free(str);
	}
	if (mongoc_cursor_error(cursor, &error))
	{
		std::cout << "[ERROR] " << "CMongoCollection::Delete 2 >>\n" << error.message << std::endl;
	}
	mongoc_cursor_destroy(cursor);
	bson_destroy(bsonCondition);
	//std::cout << m_ss.str() << std::endl;
	return m_ss.str();
}

bool CMongoCollection::Drop()
{
	bson_error_t error;
	if (!mongoc_collection_drop(m_pCollection, &error))
	{
		if (error.code != 26)
		{
			std::cout << "[ERROR] " << "CMongoCollection::Drop >>\n" << error.message << std::endl;
			return false;
		}
	}
	return true;
}

bool CMongoCollection::EnsureIndex(const char* pJsonKey)
{
	bson_error_t error;
	bson_t *bsonKey = NULL;

	bsonKey = bson_new_from_json((const uint8_t *)pJsonKey, -1, &error);
	if (bsonKey == NULL)
	{
		std::cout << "[ERROR] " << "CMongoCollection::EnsureIndex >>\n" << error.message << std::endl;
		return false;
	}
	if (!mongoc_collection_create_index(m_pCollection, bsonKey, NULL, &error))
	{
		std::cout << "[ERROR] " << "CMongoCollection::EnsureIndex >>\n" << error.message << std::endl;
		bson_destroy(bsonKey);
		return false;
	}
	bson_destroy(bsonKey);

	return true;
}
//////////////////////////////////////
