#include "TcpServer.hpp"
#include "PostgresManager.hpp"
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cerrno>
#include <arpa/inet.h>

const int BUFFER_SIZE = 30720;

TcpServer::TcpServer(int port) : m_server_fd(0), m_port(port) {
    m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_server_fd < 0) {
        std::cerr << "[socket] failed: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(m_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "[setsockopt] SO_REUSEADDR failed: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    
    std::memset(&m_address, 0, sizeof(m_address));
    m_address.sin_family = AF_INET;
    m_address.sin_addr.s_addr = INADDR_ANY;
    m_address.sin_port = htons(m_port);
}

TcpServer::~TcpServer() { close(m_server_fd); }

void TcpServer::startListen() {
    if (bind(m_server_fd, (struct sockaddr *)&m_address, sizeof(m_address)) < 0) {
        std::cerr << "[bind] failed on port " << m_port << ": " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    if (listen(m_server_fd, 10) < 0) {
        std::cerr << "[listen] failed: " << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "\nServer started on port " << m_port << std::endl;
}

void TcpServer::run() {
    while (true) {
        struct sockaddr_in clientAddr;
        std::memset(&clientAddr, 0, sizeof(clientAddr));
        socklen_t addrlen = sizeof(clientAddr);
        int new_socket = accept(m_server_fd, (struct sockaddr *)&clientAddr, &addrlen);
        if (new_socket < 0) {
            std::cerr << "[accept] failed: " << std::strerror(errno) << std::endl;
            continue;
        }

        char ip[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
        std::cout << "[accept] client " << ip << ":" << ntohs(clientAddr.sin_port) << std::endl;
        handleClient(new_socket);
    }
}

std::map<std::string, std::string> TcpServer::parseQueryString(std::string query) {
    std::map<std::string, std::string> params;
    std::istringstream stream(query);
    std::string pair;
    while (std::getline(stream, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            params[pair.substr(0, pos)] = pair.substr(pos + 1);
        }
    }
    return params;
}

// Helper to decode URL encoded strings (e.g., "Learn%20C%2B%2B" -> "Learn C++")
std::string TcpServer::urlDecode(std::string str) {
    std::string ret;
    char ch;
    int i, ii;
    for (i=0; i<str.length(); i++) {
        if (str[i] != '%') {
            if(str[i] == '+') ret += ' ';
            else ret += str[i];
        } else {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
    }
    return ret;
}

void TcpServer::handleClient(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    timeval timeout{};
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    ssize_t bytesRead = read(client_socket, buffer, BUFFER_SIZE);
    if (bytesRead <= 0) {
        std::cerr << "[read] failed/timeout: " << std::strerror(errno) << std::endl;
        close(client_socket);
        return;
    }

    std::string rawRequest(buffer, static_cast<size_t>(bytesRead));
    std::istringstream requestStream(rawRequest);
    std::string method, path, version;
    requestStream >> method >> path >> version;
    std::cout << "[request] " << method << " " << path << " " << version << std::endl;

    // --- 1. CORS PRE-FLIGHT CHECK ---
    // React will ask "OPTIONS" before doing a POST. We must say OK.
    if (method == "OPTIONS") {
        sendResponse(client_socket, "204 No Content", "text/plain", "");
        close(client_socket);
        return;
    }

    // --- 2. PARSE PARAMETERS ---
    std::map<std::string, std::string> params;

    if (method == "GET" || method == "DELETE" || method == "PUT") {
        size_t qPos = path.find('?');
        if (qPos != std::string::npos) {
            params = parseQueryString(path.substr(qPos + 1));
            path = path.substr(0, qPos);
        }
    }
    else if (method == "POST") {
        size_t headerEnd = rawRequest.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            params = parseQueryString(rawRequest.substr(headerEnd + 4));
        }
    }

    PostgresManager& db = PostgresManager::getInstance();

    // --- 3. KANBAN ROUTES ---
    std::cout << "[route] " << path << std::endl;
    // GET /tasks - Fetch the board
    if (path == "/tasks" && method == "GET") {
        std::string userId = urlDecode(params["userId"]);
        // std::cout<<userId<<std::endl;
        // for(auto const &pair : params){
        //     std::cout<<pair.first<<" "<<pair.second<<" "<<std::endl;
        // }
        auto rows = db.query("SELECT id, task, status,user_id FROM tasks WHERE user_id=\'" +userId+"\'"+ " ORDER BY id ASC");
        for(int i = 0;i<rows.size();i++){
            for(const auto &pair :rows[i]){
                std::cout<<pair.first<<" "<<pair.second<<std::endl;
            }
        }
        std::string json = "[";
        for (size_t i = 0; i < rows.size(); ++i) {
            json += "{\"id\": " + rows[i]["id"] + 
                    ", \"task\": \"" + rows[i]["task"] + 
                    "\", \"user_id\": \"" + rows[i]["user_id"] + 
                    "\", \"status\": \"" + rows[i]["status"] + "\"}";
            if (i < rows.size() - 1) json += ",";
        }
        json += "]";
        
        sendResponse(client_socket, "200 OK", "application/json", json);
    }

    // POST /tasks - Add a card (Default status: todo)
    else if (path == "/tasks" && method == "POST") {
        if (params.count("task") && params.count("userId")) {
            std::string task = urlDecode(params["task"]); // Decode "My%20Task"
            std::string userId = urlDecode(params["userId"]);
            bool success = db.execute(
                "INSERT INTO tasks (task, status, user_id) VALUES ($1, 'todo', $2)",
                {task, userId}
            );
            
            if (success) sendResponse(client_socket, "201 Created", "text/plain", "Task Created");
            else sendResponse(client_socket, "500 Error", "text/plain", "DB Error");
        } else {
            sendResponse(client_socket, "400 Bad Request", "text/plain", "Missing task or userId");
        }
    }

    // PUT /tasks?id=X&status=inprogress - Move a card
    else if (path == "/tasks" && method == "PUT") {
         if (params.count("id") && params.count("status")) {
            bool success = db.execute("UPDATE tasks SET status = $1 WHERE id = $2", {params["status"], params["id"]});
            if (success) sendResponse(client_socket, "200 OK", "text/plain", "Task Updated");
            else sendResponse(client_socket, "500 Error", "text/plain", "Update failed");
         } else {
             sendResponse(client_socket, "400 Bad Request", "text/plain", "Missing id or status");
         }
    }

    // DELETE /tasks?id=X - Remove a card
    else if (path == "/tasks" && method == "DELETE") {
        if (params.count("id")) {
            bool success = db.execute("DELETE FROM tasks WHERE id = $1", {params["id"]});
            if (success) sendResponse(client_socket, "200 OK", "text/plain", "Task Deleted");
            else sendResponse(client_socket, "500 Error", "text/plain", "Delete failed");
        } else {
            sendResponse(client_socket, "400 Bad Request", "text/plain", "Missing ID");
        }
    }
    
    else {
        sendResponse(client_socket, "404 Not Found", "text/plain", "Route not found");
    }

    close(client_socket);
}

void TcpServer::sendResponse(int client_socket, std::string status, std::string contentType, std::string content) {
    std::string httpResponse = 
        "HTTP/1.1 " + status + "\r\n" +
        "Content-Type: " + contentType + "\r\n" +
        // --- CORS HEADERS (CRITICAL FOR REACT) ---
        "Access-Control-Allow-Origin: *\r\n" +
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n" +
        "Access-Control-Allow-Headers: Content-Type\r\n" +
        // -----------------------------------------
        "Content-Length: " + std::to_string(content.length()) + "\r\n" +
        "\r\n" + 
        content;
        
    send(client_socket, httpResponse.c_str(), httpResponse.length(), 0);
}
