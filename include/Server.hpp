#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <thread>
#include "ThreadPool.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>

class Server {
public:
	Server(int port, size_t thread_count);
	void run();
	~Server();

private:
	int server_fd;
	int port;
	int kq;
	struct sockaddr_in address;
	ThreadPool thread_pool;
	
	void setupServerSocket();
	void setNonBlocking(int fd);
	void handleEvents();
	static void handleClient(int client_socket);
};

#endif
