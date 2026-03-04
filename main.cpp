#include "TcpServer.hpp"
#include "PostgresManager.hpp"
#include <iostream>
#include <cstdlib>

int main() {
    const char* dbName = std::getenv("POSTGRES_DB");
    const char* dbUser = std::getenv("POSTGRES_USER");
    const char* dbPassword = std::getenv("POSTGRES_PASSWORD");
    const char* dbHost = std::getenv("POSTGRES_HOST");

    std::string connStr = "dbname=" + std::string(dbName ? dbName : "kanban_db") +
                          " user=" + std::string(dbUser ? dbUser : "postgres") +
                          " password=" + std::string(dbPassword ? dbPassword : "postgres") +
                          " host=" + std::string(dbHost ? dbHost : "db");

    std::cout << "Connecting to DB: " << connStr << "..." << std::endl;
    
    PostgresManager::getInstance().connect(connStr);
    
    if (!PostgresManager::getInstance().isConnected()) {
        std::cerr << "Could not connect to DB. Check main.cpp connection string!" << std::endl;
        return 1;
    }

    TcpServer server(8080);
    server.startListen();
    server.run();

    return 0;
}
