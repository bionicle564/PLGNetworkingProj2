#pragma once

#include <mysql/jdbc.h>

#include <string>

using namespace sql;

using std::string;

enum CreateAccountWebResult
{
	SUCCESS,
	ACCOUNT_ALREADY_EXISTS,
	INVALID_PASSWORD,
	INTERNAL_SERVER_ERROR
};

class DBHelper
{
public:
	DBHelper(void);

	void Connect(const string& hostname, const string& username, const string& password);
	bool IsConnected(void);

	// SELECT = sql::Statement::executeQuery()
	// UPDATE = and sql::Statement::executeUpdate()
	// INSERT = sql::Statement::execute()

	CreateAccountWebResult CreateAccount(const string& email, const string& password);

private:
	void GeneratePreparedStatements(void);

	mysql::MySQL_Driver* m_Driver;
	Connection* m_Connection;
	ResultSet* m_ResultSet;
	bool m_IsConnected;
	bool m_IsConnecting;
};