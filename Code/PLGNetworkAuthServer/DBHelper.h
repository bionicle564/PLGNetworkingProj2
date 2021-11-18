#pragma once

#include <mysql/jdbc.h>

#include <string>

#include "SaltHasher.h"

using namespace sql;

using std::string;

enum CreateAccountWebResult
{
	SUCCESS,
	ACCOUNT_ALREADY_EXISTS,
	INVALID_PASSWORD,
	INTERNAL_SERVER_ERROR
};

struct DatabaseResponse
{
	CreateAccountWebResult result;
	std::string date;
	int userId;
};

class DBHelper
{
public:
	DBHelper(void);
	~DBHelper();

	//Hostname "localhost:3306", username "root", password "password"
	void Connect(const string& hostname, const string& username, const string& password);
	bool IsConnected(void);

	// SELECT = sql::Statement::executeQuery()
	// UPDATE = and sql::Statement::executeUpdate()
	// INSERT = sql::Statement::execute()

	DatabaseResponse CreateAccount(const string& email, const string& password, const string& salt);
	bool LoginUser(const string& email, const string& password);

private:
	SaltHasher saltHash;

	void GeneratePreparedStatements(void);

	mysql::MySQL_Driver* m_Driver;
	Connection* m_Connection;
	ResultSet* m_ResultSet;
	bool m_IsConnected;
	bool m_IsConnecting;

	std::string GetTimeInDateTimeFormat();
};