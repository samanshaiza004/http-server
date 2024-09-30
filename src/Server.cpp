#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/event.h>

Server::Server(int port, size_t thread_count) : port(port), thread_pool(thread_count)
{
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    setupServerSocket();
}

Server::~Server()
{
    if (server_fd > 0)
    {
        close(server_fd);
    }
}

void Server::setupServerSocket()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        throw std::runtime_error("socket creation failed!");
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        close(server_fd);
        perror("Bind failed");
    }

    setNonBlocking(server_fd);

    if (listen(server_fd, 128) < 0)
    {
        close(server_fd);
        perror("Listen failed");
    }

    std::cout << "listening on localhost:" << port << "\n";

    kq = kqueue();
    if (kq == -1)
    {
        throw std::runtime_error("failed to create kqueue!");
    }

    struct kevent change_event;
    EV_SET(&change_event, server_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    if (kevent(kq, &change_event, 1, NULL, 0, NULL) == -1)
    {
        close(kq);
        throw std::runtime_error("failed to add server socket to kqueue!");
    }
}

void Server::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        throw std::runtime_error("failed to get descriptor flags!");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        throw std::runtime_error("failed to set non-blocking mode!");
    }
}

void Server::run()
{
    handleEvents();
}

void Server::handleEvents()
{
    const int MAX_EVENTS = 10;
    struct kevent events[MAX_EVENTS];

    while (true)
    {
        int num_events = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
        if (num_events == -1)
        {
            std::cerr << "kevent failed! " << strerror(errno) << "\n";
            break;
        }

        for (int i = 0; i < num_events; i++)
        {
            if (events[i].ident == server_fd && events[i].filter == EVFILT_READ)
            {
                int addrlen = sizeof(address);
                int client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                if (client_socket < 0)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        perror("accept failed");
                    }
                    continue;
                }
                if (client_socket >= 0)
                {
                    setNonBlocking(client_socket);

                    struct kevent change_event;
                    EV_SET(&change_event, client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
                    if (kevent(kq, &change_event, 1, NULL, 0, NULL) == -1)
                    {
                        std::cerr << "kevent failed: " << strerror(errno) << "\n";
                        close(client_socket);
                    }
                    else
                    {
                        std::cout << "Client socket added to kqueue." << "\n";
                    }
                }
            }
            else if (events[i].filter == EVFILT_READ)
            {
                // Handle incoming data from client
                int client_socket = events[i].ident;

                // Enqueue the task to the thread pool
                thread_pool.enqueue([this, client_socket]()
                                    { handleClient(client_socket); });

                // Remove the client from kqueue to avoid multiple enqueues
                struct kevent change_event;
                EV_SET(&change_event, client_socket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                if (kevent(kq, &change_event, 1, NULL, 0, NULL) == -1)
                {
                    std::cerr << "kevent EV_DELETE failed: " << strerror(errno) << "\n";
                }
            }
        }
    }
}

void Server::handleClient(int client_socket)
{
    char buffer[4096];
    int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);

    if (bytes_read > 0)
    {
        std::cout << "received: " << buffer << "\n";

        const char *response = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nHello, Client!";

        ssize_t bytes_sent = 0;
        ssize_t total_sent = 0;
        size_t response_len = strlen(response);

        while (total_sent < response_len)
        {
            bytes_sent = send(client_socket, response + total_sent, response_len - total_sent, 0);
            if (bytes_sent == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // Socket buffer full, try again later
                    continue;
                }
                else
                {
                    std::cerr << "send error: " << strerror(errno) << "\n";
                    close(client_socket);
                    return;
                }
            }
            total_sent += bytes_sent;
        }

        std::cout << "Response sent successfully.\n";
        close(client_socket);
    }
    else if (bytes_read == 0)
    {
        close(client_socket);
    }
    else
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            std::cerr << "recv error: " << strerror(errno) << "\n";
            close(client_socket);
        }
    }
}
