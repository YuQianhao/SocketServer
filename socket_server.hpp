#pragma once
#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__
#include "thread_pool.hpp"
#include <winsock2.h>
#include <memory>
#include <Windows.h>
#pragma comment(lib,"ws2_32.lib")


namespace yqh
{
	class socket_client;
	class socket_server_exce;

	typedef std::unique_ptr<socket_client> socket_client_ptr;
	
	typedef void (*socket_proc)(socket_client_ptr);
	typedef void (*socket_listener_init)();
	typedef void (*socket_listener_create)();
	typedef void (*socket_listener_bind)();
	typedef void (*socket_listener_start)();
	typedef void (*socket_listener_clear)();

	class socket_server
	{
	public:
		socket_server(const char *,unsigned short ,unsigned,unsigned);
		void addInitListener(socket_listener_init);
		void addCreateListener(socket_listener_create);
		void addBindListener(socket_listener_bind);
		void addStartListener(socket_listener_start);
		void addClearListener(socket_listener_clear);
		void loop(socket_proc)noexcept(false);
		void close();
		~socket_server();
	private:
		socket_server(const socket_server&) = delete;
		socket_server& operator=(const socket_server&) = delete;
	private:
		socket_proc			procSocket;
		const char*			csAddr;
		unsigned short		usPort;
		unsigned			thpoolsize;
		ThreadPool			threadPool;
		const char*			err;
		bool				bClose;
		SOCKET				sListener;
		unsigned			uBacklog;
	private:
		socket_listener_init		proc_init;
		socket_listener_create		proc_create;
		socket_listener_bind		proc_bind;
		socket_listener_start		proc_start;
		socket_listener_clear		proc_clear;
	};

	class socket_client
	{
	public:
		socket_client(SOCKET,sockaddr_in);
		const char* addr();
		char* read(size_t);
		int send(const char*);
		int send(const char*,size_t);
		void close();
		~socket_client();
	private:
		socket_client(const socket_client&)=delete;
		socket_client& operator=(const socket_client&)=delete;
	private:
		SOCKET			u64Socket;
		const char*		csAddr;
		bool			bClose;
	};

	class socket_server_exce
	{
	public:
		socket_server_exce(const char*,unsigned short,const char*,unsigned long);
		const char* error();
		const char* address();
		unsigned short port();
		unsigned long errorCode();
		~socket_server_exce();
	private:
		const char*			cslAddr;
		unsigned short		usPort;
		const char*			csError;
		unsigned long		dwErrorCode;
	};
	
}

#endif