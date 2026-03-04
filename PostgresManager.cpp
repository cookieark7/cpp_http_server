#include "PostgresManager.hpp"
#include <iostream>

PostgresManager &PostgresManager::getInstance() {
    static PostgresManager instance;
    return instance;
}

PostgresManager::PostgresManager() : m_connection(nullptr) {
    std::cout << "[PostgresManager] Initialized." << std::endl;
}

PostgresManager::~PostgresManager() {
    if (m_connection != nullptr) {
        PQfinish(m_connection);
        std::cout << "[PostgresManager] Connection Closed." << std::endl;
    }
}

void PostgresManager::connect(const std::string &connInfo) {
    if (m_connection) {
        PQfinish(m_connection);
        m_connection = nullptr;
    }
    std::cout<<connInfo.c_str()<< std::endl;
    m_connection = PQconnectdb(connInfo.c_str());

    if (PQstatus(m_connection) != CONNECTION_OK) {
        std::cerr << "[PostgresManager] Connection to database failed: "
                  << PQerrorMessage(m_connection) << std::endl;
        PQfinish(m_connection);
        m_connection = nullptr;
    } else {
        std::cout << "[PostgresManager] Successfully connected to database!" << std::endl;
    }
}

bool PostgresManager::isConnected() const {
    return m_connection != nullptr && PQstatus(m_connection) == CONNECTION_OK;
}

bool PostgresManager::execute(const std::string &sql, const std::vector<std::string> &params) {
    if (!isConnected())
        return false;

    std::vector<const char *> paramValues;
    paramValues.reserve(params.size());

    for (const auto &p : params) {
        paramValues.push_back(p.c_str());
    }

    PGresult *res = PQexecParams(
        m_connection,
        sql.c_str(),
        static_cast<int>(params.size()),
        nullptr,
        paramValues.data(),
        nullptr,
        nullptr,
        0
    );

    if (res == nullptr || PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Execution failed: " << PQerrorMessage(m_connection) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

std::vector<std::map<std::string, std::string>> PostgresManager::query(const std::string& sql, const std::vector<std::string>& params) {
    std::vector<std::map<std::string, std::string>> results;
    if (!isConnected()) return results;

    std::vector<const char*> paramValues;
    for (const auto& p : params) paramValues.push_back(p.c_str());

    PGresult* res = PQexecParams(
        m_connection,
        sql.c_str(),
        static_cast<int>(params.size()),
        nullptr,
        paramValues.data(),
        nullptr,
        nullptr,
        0
    );

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Query failed: " << PQerrorMessage(m_connection) << std::endl;
        PQclear(res);
        return results;
    }

    int rows = PQntuples(res);
    int cols = PQnfields(res);

    for (int i = 0; i < rows; i++) {
        std::map<std::string, std::string> row;
        for (int j = 0; j < cols; j++) {
            std::string colName = PQfname(res, j);
            std::string colValue = PQgetvalue(res, i, j);
            row[colName] = colValue;
        }
        results.push_back(row);
    }

    PQclear(res);
    return results;
}
