# Win32 Socket Server

​		The websocket package used by the Win32 platform, which encapsulates the socket and ThreadPool of Win32, can receive and send the socket with very little code.This suite provides a variety of state listeners, configurable thread pools.

**[Statement]  ThreadPool refers to GitHub：https://github.com/progschj/ThreadPool**

### Instructions：

Source code is divided into three files, thread_ pool.cpp ,socket_ server.hpp ,socket_ server.cpp The first is the header file of the thread pool. If you have no special use of the thread pool, you do not need to rely on it. The second is the header file of socketserver, which declares all relevant classes and functions. Usually, you just need to introduce socket_ server.hpp That's fine.

### 1、Bring in the header file and open the namespace.

```c++
#include "socket_ server.hpp"

using namespace yqh;
```

except for thread_ Except for pool, all declarations are defined in the "yqh" namespace, so you need to open the namespace when using it.

### 2、Create a socket_server Instance object

```c++
socket_server server("127.0.0.1",10001,10,5);
```

Function analysis:

```c++
socket_server::socket_server(const char* addr, unsigned short port, unsigned thpool,unsigned backlog);
```

* 1、addr

IP address to be monitored is usually 127.0.0.1, because this socket is only used as a server.

* 2、port

port to listen on.

* 3、thpool

the size of the thread pool refers to the number of threads created to execute each socket connection.

* 4、backlog

the waiting number of connection queues in the socket. Sockets exceeding this number will be rejected immediately.

### 3、Start listening to the socket and send it to the worker thread for processing.

```c++
try{
    server.loop(socketProc);
}catch(const socket_server_exce &exce){
    cout<<exce.error()<<endl;
}
```

The loop function receives a function pointer. The circle of the function pointer is defined as follows:

```c++
typedef void (*socket_proc)(socket_client_ptr);
```

Whenever a new thread in the socket pool is idle, it points to a new thread to execute the function. The function pointer receives a parameter named **"socket_client"**, which is an intelligent pointer. In order to reduce the difficulty of memory management, we use the **std::unique_ t** to wrap the client pointer so that you don't need to focus on memory issues. We use this type to represent the client. It contains some common operations of the client, such as:

```c++
typedef std::unique_t<socket_client> socket_client_ptr;
```

```c++
void socketProc(socket_client_ptr client)
{
	char *data = client->read(255);
	delete[] data;
    const char *sendData="This Socket Proc.";
	client->send("Hello World!");
    client->send(sendData,strlen(sendData));
}
```

**<font color=#e60000>Note that the read method does not need to be used to release the memory automatically.</font>**

Of course, if you want to know the IP address of the client, you should call the **addr ()** method to get it, for example:

```c++
void socketProc(socket_client_ptr client)
{
	const char *clientAddr=client->addr();
}
```

If you have finished processing the client's task, generally speaking, you don't need to call the method to close the socket, because the client is in the scope of automatic memory management. When the function call is finished, the client object will be automatically released, and the link will be closed. Of course, if it is necessary to manually close the connection, you can call "close()" To explicitly close.

```c++
void socketProc(socket_client_ptr client)
{
	client->close();
}
```

Some errors will occur in the process of loop. We have defined an exception class to handle these errors. You must use try... Catch to catch this exception unless you guarantee that no error will occur.

This exception type provides some methods for us to use.

```c++
try{
    server.loop(socketProc);
}catch(const socket_server_exce &exce){
    const char *error=exce.error();
    const chat *address=exec.address();
    unsigned short port=exec.port();
    unsigned long errorCode=exec.errorCode();
}
```

It is worth noting that the last method, "errorcode()", is the same as "GetLastError()" in Win32.

### 4、Status monitoring

We also provide some callback listeners related to socket state. If you want to use these listeners, you must call before the "loop ()" function is called.

##### ① socket_listener_init

This function will be called after the initialization of “WSAStartup ()”.

```c++
typedef void (*socket_listener_init)();
```

```c++
server.addInitListener([](){
    
});
try{
    server.loop(socketProc);
}catch(const socket_server_exce &exce){
    cout<<exce.error()<<endl;
}
```

##### ② socket_listener_create

This function will be called after the Socket is created.

```c++
typedef void (*socket_listener_create)();
```

```c++
server.addCreateListener([](){
    
});
try{
    server.loop(socketProc);
}catch(const socket_server_exce &exce){
    cout<<exce.error()<<endl;
}
```

##### ③ socket_listener_bind

This function will be called after successful binding to the port.

```c++
typedef void (*socket_listener_bind)();
```

```c++
server.addBindListener([](){
    
});
try{
    server.loop(socketProc);
}catch(const socket_server_exce &exce){
    cout<<exce.error()<<endl;
}
```

##### ④ socket_listener_start

This function will be called after the listening port is successful.

```c++
typedef void (*socket_listener_start)();
```

```c++
server.addStartListener([](){
    
});
try{
    server.loop(socketProc);
}catch(const socket_server_exce &exce){
    cout<<exce.error()<<endl;
}
```

##### ⑤ socket_listener_clear

This method is called before the entire port listening is turned off.

```c++
typedef void (*socket_listener_clear)();
```

```c++
server.addClearListener([](){
    
});
try{
    server.loop(socketProc);
}catch(const socket_server_exce &exce){
    cout<<exce.error()<<endl;
}
```



