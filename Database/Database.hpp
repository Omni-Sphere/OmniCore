#pragma once

#include <iostream>
#include <sql.h>
#include <sqlext.h>
#include <stdexcept>
#include <map>

#include "Datatable.hpp"
#include "SQLParams.hpp"

namespace omnicore::service
{
    class Database
    {
    private:
        SQLHENV henv;
        SQLHDBC hdbc;
        SQLHSTMT hstmt;

        std::string _ServerName;
        std::string _DatabaseName;
        std::string _UserName;
        std::string _Password;
        bool _TrustServerCertificate;

        void PrepareStatement(const std::string &);
        std::string ExtractError(const char *, SQLHANDLE, SQLSMALLINT);
        void RunQuery(const std::string &);

        std::vector<double> doubleStorage;
        std::vector<std::string> stringStorage;
        std::vector<std::vector<uint8_t>> binaryStorage;
        std::vector<int> intStorage;
        std::vector<SQLLEN> indStorage;

    public:
        Database();
        ~Database();
        void ServerName(const std::string &serverName);
        void DatabaseName(const std::string &dbName);
        void UserName(const std::string &userName);
        void Password(const std::string &password);
        void TrustServerCertificate(const bool &trustedServerCertificate);
        bool Connect();
        bool RunStatement(const std::string &query);

        bool RunPrepared(const std::string &query, const std::vector<type::SQLParam> &params);

        type::Datatable FetchPrepared(const std::string &query, const std::vector<type::SQLParam> &params);
        type::Datatable FetchPrepared(const std::string &query, const std::vector<std::string> &params);
        type::Datatable FetchPrepared(const std::string &query, const std::string &param);
        type::Datatable FetchResults(const std::string &query);
        
        bool BeginTransaction();
        bool CommitTransaction();
        bool RollbackTransaction();

        void Disconnect();
    };

}