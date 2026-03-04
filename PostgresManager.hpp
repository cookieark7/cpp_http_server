#ifndef POSTGRESMANAGER_HPP
#define POSTGRESMANAGER_HPP

#include <libpq-fe.h>
#include <map>
#include <string>
#include <vector>

class PostgresManager {
public:
    // Returns a reference to the singleton instance.
    static PostgresManager &getInstance();

    PostgresManager(const PostgresManager &) = delete;
    PostgresManager &operator=(const PostgresManager &) = delete;

    void connect(const std::string &connInfo);

    bool isConnected() const;

    bool execute(const std::string &sql, const std::vector<std::string> &params = {});

    std::vector<std::map<std::string, std::string>> query(
        const std::string &sql,
        const std::vector<std::string> &params = {});

private:
    // Private constructor for singleton.
    PostgresManager();
    ~PostgresManager();
    PGconn *m_connection;

};

#endif
