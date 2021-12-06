#pragma warning(disable : 4996) //disable the depricated localtime "warning", we don't need to deal with that here

#include "DBHelper.h"

#include <time.h>

//example ones
sql::PreparedStatement* g_GetEmails;
//sql::PreparedStatement* g_InsertWebAuth;

//the ones we'll actually use
sql::PreparedStatement* createUser;
sql::PreparedStatement* createAuth;
sql::PreparedStatement* updateLoginTime;

DBHelper::DBHelper(void)
	: m_IsConnected(false)
	, m_IsConnecting(false)
	, m_Connection(0)
	, m_Driver(0)
	, m_ResultSet(0)
{
}

DBHelper::~DBHelper() {
	delete g_GetEmails;
	delete createUser;
	delete createAuth;
	delete updateLoginTime;
	delete m_Driver;
	delete m_Connection;
	delete m_ResultSet;
}

bool DBHelper::IsConnected(void)
{
	return m_IsConnected;
}

DatabaseResponse DBHelper::CreateAccount(const string& email, const string& password, const string& salt)
{
	DatabaseResponse reponse;

	g_GetEmails->setString(1, email);
	try
	{
		m_ResultSet = g_GetEmails->executeQuery();
	}
	catch (SQLException e)
	{
		printf("Failed to retrieved web_auth data!\n");
		reponse.result = DatabaseReturnCode::INTERNAL_SERVER_ERROR;
		return reponse;
	}

	if (m_ResultSet->rowsCount() > 0)
	{
		printf("Account already exists with email!\n");
		reponse.result = DatabaseReturnCode::ACCOUNT_ALREADY_EXISTS;
		return reponse;
	}

	//This is for adding the user
	//YYYY-MM-DD hh:mm:ss <-Format for Timestamp and DateTime	
	
	//std::string nowAsDateTime = GetTimeInDateTimeFormat();
	//reponse.date = nowAsDateTime;

	try {
		//createUser->setString(1, nowAsDateTime);
		int result = createUser->executeUpdate();
	}
	catch (SQLException e)
	{
		printf("Failed to insert account into web_auth!\n");
		reponse.result = DatabaseReturnCode::INTERNAL_SERVER_ERROR;
		return reponse;
	}

	//this is for adding the authentication data
	sql::Statement* stmt = m_Connection->createStatement();
	try
	{
		m_ResultSet = stmt->executeQuery("SELECT LAST_INSERT_ID();");
	}
	catch (SQLException e)
	{
		printf("Failed to retrieve last insert id!\n");
		reponse.result = DatabaseReturnCode::INTERNAL_SERVER_ERROR;
		return reponse;
	}
	int lastId = 0;
	if (m_ResultSet->next())
	{
		lastId = m_ResultSet->getInt(1);
	}
	delete stmt;


	try
	{
		//g_InsertWebAuth->setString(1, email);
		//g_InsertWebAuth->setString(2, password);
		//g_InsertWebAuth->setString(3, "Salt");
		//g_InsertWebAuth->setInt(4, 7);
		//int result = g_InsertWebAuth->executeUpdate();

		createAuth->setString(1, email);
		createAuth->setString(2, salt);
		createAuth->setString(3, password);
		createAuth->setInt(4, lastId);
		int result = createAuth->executeUpdate();

	}
	catch (SQLException e)
	{
		printf("Failed to insert account into web_auth!\n");
		reponse.result = DatabaseReturnCode::INTERNAL_SERVER_ERROR;
		return reponse;
	}

	sql::Statement* stmt2 = m_Connection->createStatement();
	try
	{
		m_ResultSet = stmt2->executeQuery("SELECT LAST_INSERT_ID();");
	}
	catch (SQLException e)
	{
		printf("Failed to retrieve last insert id!\n");
		reponse.result = DatabaseReturnCode::INTERNAL_SERVER_ERROR;
		return reponse;
	}
	lastId = 0;
	if (m_ResultSet->next())
	{
		lastId = m_ResultSet->getInt(1);
		reponse.userId = lastId;
	}
	delete stmt2;

	printf("Successfully retrieved web_auth data!\n");
	reponse.result = DatabaseReturnCode::SUCCESS;
	return reponse;
}

DatabaseResponse DBHelper::LoginUser(const string& email, const string& password) {

	DatabaseResponse reponse;

	g_GetEmails->setString(1, email);
	try
	{
		m_ResultSet = g_GetEmails->executeQuery();
	}
	catch (SQLException e)
	{
		printf("Failed to retrieved web_auth data!\n");
		reponse.result = DatabaseReturnCode::INTERNAL_SERVER_ERROR;
		return reponse;
	}

	if (m_ResultSet->rowsCount() == 0)
	{
		printf("No such Account exists!\n");
		reponse.result = DatabaseReturnCode::INVALID_CREDENTIAL;
		return reponse;
	}

	if (!m_ResultSet->next())
	{
		reponse.result = DatabaseReturnCode::INTERNAL_SERVER_ERROR;
		return reponse;
	}

	std::string retrievedPassword = m_ResultSet->getString("hash_password");
	
	//we gotta encode the passed password to see if it matches
	std::string salt = m_ResultSet->getString("salt");
	std::string saltyPassword = salt + password;
	std::string hashedPassword = saltHash.HashPassword(saltyPassword);

	if (retrievedPassword != hashedPassword) {
		printf("PASSWORD MISMATCH!\n");
		reponse.result = DatabaseReturnCode::INVALID_PASSWORD;
		return reponse;
	}

	//std::string nowAsDateTime = GetTimeInDateTimeFormat();
	//reponse.date = nowAsDateTime;

	//update the login time
	try {
		//updateLoginTime->setString(1, nowAsDateTime);
		updateLoginTime->setInt(1, m_ResultSet->getInt(1));
		int result = updateLoginTime->executeUpdate();
	}catch (SQLException e)
	{
		printf("Failed to update login data!\n");
		reponse.result = DatabaseReturnCode::INTERNAL_SERVER_ERROR;
		return reponse;
	}

	reponse.result = DatabaseReturnCode::SUCCESS;
	return reponse;
}

void DBHelper::GeneratePreparedStatements(void)
{
	g_GetEmails = m_Connection->prepareStatement(
	  "SELECT * FROM web_auth WHERE email = ?;"
	);
	//g_InsertWebAuth = m_Connection->prepareStatement(
	//	"INSERT INTO web_auth(email, hashed_password, salt, userId) VALUES (?, ?, ?, ?);");

	createUser = m_Connection->prepareStatement(
		"INSERT INTO users (creation_time) VALUES (NOW());"
	);
	createAuth = m_Connection->prepareStatement(
		"INSERT INTO web_auth (email, salt, hash_password, user_id) VALUES(? , ? , ? , ?); "
	);
	updateLoginTime = m_Connection->prepareStatement(
		"UPDATE users SET last_login = CURTIME() WHERE id = ?;"
	);
}

void DBHelper::Connect(const string& hostname, const string& username, const string& password)
{
	if (m_IsConnecting)
		return;

	m_IsConnecting = true;
	try
	{
		m_Driver = mysql::get_driver_instance();
		m_Connection = m_Driver->connect(hostname, username, password);
		m_Connection->setSchema("networkauthdatabase"); // I think this is correct
	}
	catch (SQLException e)
	{
		printf("Failed to connect to database with error: %s\n", e.what());
		m_IsConnecting = false;
		return;
	}
	m_IsConnected = true;
	m_IsConnecting = false;

	GeneratePreparedStatements();

	printf("Successfully connected to database!\n");
}

std::string DBHelper::GetTimeInDateTimeFormat() {
	time_t rawTime;
	struct tm* timeInfo;

	time(&rawTime);
	timeInfo = localtime(&rawTime);


	std::string nowAsDateTime = "";

	nowAsDateTime += std::to_string(timeInfo->tm_year + 1900);
	nowAsDateTime += "-";
	nowAsDateTime += std::to_string(timeInfo->tm_mon + 1);
	nowAsDateTime += "-";
	nowAsDateTime += std::to_string(timeInfo->tm_mday);
	nowAsDateTime += " ";
	nowAsDateTime += std::to_string(timeInfo->tm_hour);
	nowAsDateTime += ":";
	nowAsDateTime += std::to_string(timeInfo->tm_min);
	nowAsDateTime += ":";
	nowAsDateTime += std::to_string(timeInfo->tm_sec);

	delete timeInfo;

	return nowAsDateTime;
};