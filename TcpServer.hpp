#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include<sys/socket.h> //Core socket function
#include<netinet/in.h>//sockaddr_in structure
#include<unistd.h> //close()
#include<string>
#include<map> 

class TcpServer{
public:
    TcpServer(int port);

    ~TcpServer();

    //initialize the listner
    void startListen();

    void run();

private:
    int m_server_fd;  // The Master Socket
    int m_port;  // the port
    struct sockaddr_in m_address;


    void handleClient(int client_socket);

    void sendResponse(int client_socket, std::string status, std::string contentType, std::string content);

    //helper function to parse the params (key value [air..])
    std::map<std::string,std::string> parseQueryString(std::string query);
    std::string urlDecode(std::string str);
};

#endif
