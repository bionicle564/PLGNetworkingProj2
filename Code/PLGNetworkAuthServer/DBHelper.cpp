#include "DBHelper.h"

sql::PreparedStatement* g_GetEmails;
sql::PreparedStatement* g_InsertWebAuth;

DBHelper::DBHelper(void)
	: m_IsConnected(false)
	, m_IsConnecting(false)
	, m_Connection(0)
	, m_Driver(0)
	, m_ResultSet(0)
{
}

bool DBHelper::IsConnected(void)
{
	return m_IsConnected;
}

CreateAccountWebResult DBHelper::CreateAccount(const string& email, const string& password)
{
	g_GetEmails->setString(1, email);
	try
	{
		m_ResultSet = g_GetEmails->executeQuery();
	}
	catch (SQLException e)
	{
		printf("Failed to retrieved web_auth data!\n");
		return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
	}

	if (m_ResultSet->rowsCount() > 0)
	{
		printf("Account already exists with email!\n");
		return CreateAccountWebResult::ACCOUNT_ALREADY_EXISTS;
	}

	try
	{
		g_InsertWebAuth->setString(1, email);
		g_InsertWebAuth->setString(2, password);
		g_InsertWebAuth->setString(3, "Salt");
		g_InsertWebAuth->setInt(4, 7);
		int result = g_InsertWebAuth->executeUpdate();
	}
	catch (SQLException e)
	{
		printf("Failed to insert account into web_auth!\n");
		return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
	}

	sql::Statement* stmt = m_Connection->createStatement();
	try
	{
		m_ResultSet = stmt->executeQuery("SELECT LAST_INSERT_ID();");
	}
	catch (SQLException e)
	{
		printf("Failed to retrieve last insert id!\n");
		return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
	}
	int lastId = 0;
	if (m_ResultSet->next())
	{
		lastId = m_ResultSet->getInt(1);
	}

	printf("Successfully retrieved web_auth data!\n");
	return CreateAccountWebResult::SUCCESS;
}

void DBHelper::GeneratePreparedStatements(void)
{
	g_GetEmails = m_Connection->prepareStatement(
		"SELECT email FROM `web_auth` WHERE email = ?;");
	g_InsertWebAuth = m_Connection->prepareStatement(
		"INSERT INTO web_auth(email, hashed_password, salt, userId) VALUES (?, ?, ?, ?);");
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
		m_Connection->setSchema("authenticationservice");
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