#include "socket_server.hpp"
#ifdef linux
#include <unistd.h>
#include <cerrno>
#include <string.h>
#endif

#ifdef WIN32
#define _error_code GetLastError()
#define _close_socket closesocket
#elif linux
#define _error_code errno
#define _close_socket ::close
#endif


using namespace yqh;

static void _CBG_PROC_SOCKET_CLIENT(socket_client *,socket_proc);

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
#ifdef WIN32
    WORD socketVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(socketVersion, &wsaData) != 0)
    {
        throw socket_server_exce(csAddr, usPort,"startup websocket error!", GetLastError());
    }
#endif
    if(this->proc_init!=nullptr)
    {
        this->proc_init();
    }
    this->sListener = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef WIN32
    if (sListener == INVALID_SOCKET)
#elif linux
    if (sListener < 0)
#endif
    {
        throw socket_server_exce(csAddr, usPort, "create websocket error", _error_code);
    }
    if (this->proc_create != nullptr)
    {
        this->proc_create();
    }
    sockaddr_in liAddrIn;
    liAddrIn.sin_family = AF_INET;
    liAddrIn.sin_port = htons(usPort);
#ifdef WIN32
    liAddrIn.sin_addr.S_un.S_addr = inet_addr(csAddr);
#elif linux
    liAddrIn.sin_addr.s_addr = inet_addr(csAddr);
#endif
    if (::bind(sListener,
#ifdef WIN32
                (LPSOCKADDR)
#elif linux
               (sockaddr*)
#endif
               &liAddrIn,
               sizeof(liAddrIn))
#ifdef WIN32
== SOCKET_ERROR
#elif linux
<0
#endif
               )
    {
        throw socket_server_exce(csAddr, usPort, "bind socket port error", _error_code);
    }
    if (this->proc_bind != nullptr)
    {
        this->proc_bind();
    }
    if (::listen(sListener, uBacklog)
#ifdef WIN32
== SOCKET_ERROR
#elif linux
<0
#endif
    )
    {
        throw socket_server_exce(csAddr, usPort, "listener socket port error", _error_code);
    }
    if (this->proc_start != nullptr)
    {
        this->proc_start();
    }
    bClose = false;
    while(true)
    {
        sockaddr_in remoteAddr;
#ifdef WIN32
        int
#elif linux
        socklen_t
#endif
        nAddrlen = sizeof(remoteAddr);
        _socket sClient=accept(sListener,
#ifdef WIN32
(SOCKADDR*)
#elif linux
(sockaddr*)
#endif
        &remoteAddr, &nAddrlen);
        if(sClient
#ifdef WIN32
!= INVALID_SOCKET
#elif linux
< 0
#endif
        )
        {
            socket_client* client = new socket_client(sClient, remoteAddr);
            threadPool.enqueue(_CBG_PROC_SOCKET_CLIENT, client,this->procSocket);
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
        _close_socket(sListener);
#ifdef WIN32
        WSACleanup();
#endif
    }
}

socket_server::~socket_server()
{
    close();
}

socket_client::socket_client(_socket socket, sockaddr_in inaddr):
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
        _close_socket(u64Socket);
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

void _CBG_PROC_SOCKET_CLIENT(socket_client* client, socket_proc proc){
    std::unique_ptr<socket_client> upClient(client);
    proc(std::move(upClient));
}