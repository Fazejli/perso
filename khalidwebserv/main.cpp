#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <fstream>


int main()
{
    int  socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd == -1)
    {
        std::cout << "Failed to create a socket" << std::endl;
        return 0;
    }
    int opt = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        return(0);

    sockaddr_in sockaddr;

    memset(&sockaddr, 0, sizeof(sockaddr));

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(9999);
    sockaddr.sin_addr.s_addr = INADDR_ANY;


    int binding = bind(socketfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
    if(binding == -1)
    {
        std::cout << "Failed to bind to port 9999" << std::endl;
        return 0;
    }

    int listening = listen(socketfd, 10);
    if(listening == -1)
    {
        std::cout << "Queue to big" << std::endl;
    }

    std::vector<pollfd> fds;

    pollfd server;
    server.fd = socketfd;
    server.events = POLLIN;
    server.revents = 0;
    fds.push_back(server);

    std::cout << "Server running..." <<std::endl;

    while(true)
    {
        int ret = poll(&fds[0], fds.size(), -1);
        if(ret == -1)
        {
            std::cout << "poll error" << std::endl;
            break;
        }
        for(size_t i = 0; i < fds.size(); i++)
        {
            if(fds[i].fd == socketfd && (fds[i].revents & POLLIN))
            {
                sockaddr_in clientAddr;
                socklen_t len = sizeof(clientAddr);

                int clientFd = accept(socketfd, (struct sockaddr*)&clientAddr, &len);
                if(clientFd != -1)
                {
                    pollfd client;
                    client.fd = clientFd;
                    client.events = POLLIN;
                    client.revents = 0;
                    fds.push_back(client);

                    std::cout << "New client connected : " << clientFd << std::endl;
                }
            }
            else if(fds[i].revents & POLLIN)
            {
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));

                ssize_t bytesRead = read(fds[i].fd, buffer, 1024);
                if(bytesRead <= 0)
                {
                    std::cout << "Client disconnected: " << fds[i].fd << std::endl;
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                    i--;
                }

                else
                {
                    std::string request(buffer, bytesRead);

                    size_t pos = request.find("\r\n");
                    if(pos == std::string::npos)
                        return 0;
                    std::string requestLine = request.substr(0, pos);

                    std::istringstream iss(requestLine);

                    std::string method;
                    std::string path;
                    std::string version;

                    iss >> method >> path >> version;
                    // std::ifstream file("index.html");

                    std::string filePath;
                    if(path == "/")
                        filePath = "index.html";
                    else 
                        filePath = "." + path;
                    
                    std::ifstream file(filePath.c_str());
                    
                    if(!file.is_open())
                    {
                        std::string body = "<h1>404 Not Found</h1>";

                        std::ostringstream response;
                        response << "HTTP/1.1 404 Not Found\r\n";
                        response << "Content-Type: text/html\r\n";
                        response << "Content-Length: " << body.size() << "\r\n";
                        response << "\r\n";
                        response << body;

                        std::string res = response.str();
                        send(fds[i].fd, res.c_str(), res.size(), 0);
                    }

                    else
                    {
                        std::stringstream bufferFile;
                        bufferFile << file.rdbuf();
                        std::string body = bufferFile.str();
                        
    
                        std::ostringstream response;
                        response << "HTTP/1.1 200 OK\r\n";
                        response << "Content-Type: text/html\r\n";
                        response << "Content-Length: " << body.size() << "\r\n";
                        response << "\r\n";
                        response << body;
    
                        std:: string responseStr = response.str();
                        send(fds[i].fd, responseStr.c_str(), responseStr.size(), 0);
    
                        
                    }
                    buffer[bytesRead] = '\0';
                    std::cout << "Message: " << buffer << std::endl;
                }
            }
        }
    }



    close(socketfd);

}