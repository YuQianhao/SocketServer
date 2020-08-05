#include "socket_server.hpp"

using namespace yqh;

socket_server::socket_server(const char* addr, unsigned short port, unsigned thpool,unsigned backlog):
		threadPool(thpool),
		csAddr(addr),
		usPort(port),
		err(nullptr),
		bClose(true),
		uBacklog(backlog),
		procSocket(nullptr),
		proc_init(nullptr),
		proc_create(nullptr),
		proc_bind(nullptr),
		proc_start(nullptr),
		proc_clear(nullptr)
{
	if(addr == nullptr)
	{
		this->err = "socket listener address is not empty.";
	}
	if(port<1 || port>65536)
	{
		this->err = "socket listener port is error.";
	}
}

void socket_server::addInitListener(socket_listener_init proc)
{
	this->proc_init = proc;
}

void socket_server::addCreateListener(socket_listener_create proc)
{
	this->proc_create = proc;
}

void socket_server::addBindListener(socket_listener_bind proc)
{
	this->proc_bind = proc;
}

void socket_server::addStartListener(socket_listener_start proc)
{
	this->proc_start = proc;
}

void socket_server::addClearListener(socket_listener_clear proc)
{
	this->proc_clear = proc;
}

void socket_server::loop(socket_proc proc) noexcept(false)
{
	this->procSocket = proc;
	WORD socketVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(socketVersion, &wsaData) != 0)
	{
		throw socket_server_exce(csAddr, usPort,"startup websocket error!", GetLastError());
	}
	if(this->proc_init!=nullptr)
	{
		this->proc_init();
	}
	this->sListener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sListener == INVALID_SOCKET)
	{
		throw socket_server_exce(csAddr, usPort, "create websocket error", GetLastError());
	}
	if (this->proc_create != nullptr)
	{
		this->proc_create();
	}
	sockaddr_in liAddrIn;
	liAddrIn.sin_family = AF_INET;
	liAddrIn.sin_port = htons(usPort);
	liAddrIn.sin_addr.S_un.S_addr = inet_addr(csAddr);
	if (::bind(sListener, (LPSOCKADDR)&liAddrIn, sizeof(liAddrIn)) == SOCKET_ERROR)
	{
		throw socket_server_exce(csAddr, usPort, "bind socket port error", GetLastError());
	}
	if (this->proc_bind != nullptr)
	{
		this->proc_bind();
	}
	if (::listen(sListener, uBacklog) == SOCKET_ERROR)
	{
		throw socket_server_exce(csAddr, usPort, "listener socket port error", GetLastError());
	}
	if (this->proc_start != nullptr)
	{
		this->proc_start();
	}
	bClose = false;
	while(true)
	{
		sockaddr_in remoteAddr;
		int nAddrlen = sizeof(remoteAddr);
		SOCKET sClient=accept(sListener, (SOCKADDR*)&remoteAddr, &nAddrlen);
		if(sClient != INVALID_SOCKET)
		{
			socket_client* client = new socket_client(sClient, remoteAddr);
			threadPool.enqueue(this->procSocket, client);
		}
	}
}

void socket_server::close()
{
	if(!bClose)
	{
		if(this->proc_clear!=nullptr)
		{
			this->proc_clear();
		}
		bClose = true;
		closesocket(sListener);
		WSACleanup();
	}
}

socket_server::~socket_server()
{
	close();
}

socket_client::socket_client(SOCKET socket, sockaddr_in inaddr):
	u64Socket(socket), csAddr(inet_ntoa(inaddr.sin_addr)),bClose(false)
{
}

const char* socket_client::addr()
{
	return csAddr;
}

char* socket_client::read(size_t length)
{
	char* _buf = new char[length + 1]{ 0 };
	::recv(u64Socket, _buf, length, 0);
	return _buf;
}

int socket_client::send(const char* data)
{
	return this->send(data,strlen(data));
}

int socket_client::send(const char* data, size_t length)
{
	return ::send(u64Socket, data, length, 0);
}

void socket_client::close()
{
	if(!bClose)
	{
		bClose = true;
		closesocket(u64Socket);
	}
}

socket_client::~socket_client()
{
	close();
}

socket_server_exce::socket_server_exce(const char* address, unsigned short port, const char* error,unsigned long errcode)
	:cslAddr(address),usPort(port),csError(error), dwErrorCode(errcode)
{
}

const char* socket_server_exce::error()
{
	return csError;
}

const char* socket_server_exce::address()
{
	return cslAddr;
}

unsigned short socket_server_exce::port()
{
	return usPort;
}

unsigned long socket_server_exce::errorCode()
{
	return dwErrorCode;
}

socket_server_exce::~socket_server_exce()
{
}
