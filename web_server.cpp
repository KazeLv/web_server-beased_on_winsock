#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdlib.h>
#include <io.h>
#include <winsock2.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS

//link ddl for winsock2
#pragma comment(lib,"ws2_32.lib")

#define SERVER_PORT 80     		 		//listen port
#define LIST_SIZE 5         			//maximum number of requests
#define SRC_DIR "c:/web_server_src"		//path of server resource

//thread functions
void thread_serve_client(SOCKET sServer);	//handle one TCP link
void thread_listen_quit(bool& b_quit); 		//detect quit message and set the flag b_quit

//functions to handle with message
void send_404(SOCKET);								//send 404 response to client
void send_file(SOCKET, std::string&, std::string&);	//send file to client,(.html,.txt,.img)
void send_post_response(SOCKET, std::string&);		//send response to client for login

int main(){
    std::vector<std::thread*> vec_pThreads;	//save points to trheads

    bool b_quit = false;    //flag to quit

    //winsock initialization
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2,2);  //version of WinSock DLL
    int ret = WSAStartup(wVersionRequested,&wsaData);
    if(ret!=0){
        std::cout<<"WSAStartup() failed!"<<std::endl;
        return 0;
    }

    if(LOBYTE(wsaData.wVersion)!=2 || HIBYTE(wsaData.wVersion)!=2){
        WSACleanup();
        std::cout<<"Invalid Winsock version!"<<std::endl;
        return 0;
    }

    //create socket
    SOCKET sListen = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(sListen == INVALID_SOCKET){
        WSACleanup();
        std::cout<<"socket() failed!"<<std::endl;
        return 0;
    }

    struct sockaddr_in saServer;
    saServer.sin_family = AF_INET;                      //address family
    saServer.sin_port = htons(SERVER_PORT);             //server listening port
    saServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);  //any address is allowed

    //bind socket with address
    ret = bind(sListen,(struct sockaddr*)&saServer,sizeof(saServer));
    if(ret == SOCKET_ERROR){
        std::cout<<"bind() failed! error code:"<<WSAGetLastError()<<std::endl;
        closesocket(sListen);
        WSACleanup();
        return 0;
    }

    //set sListen to listen requests
    ret = listen(sListen,LIST_SIZE);
    if(ret == SOCKET_ERROR){
        std::cout<<"listen() failed! error code:"<<WSAGetLastError()<<std::endl;
        closesocket(sListen);
        WSACleanup();
        return 0;
    }

	//start a thread to detect quit message
    std::thread t_quit(thread_listen_quit,std::ref(b_quit));

	//accept requests and handle them by threads
	//parameters for accept
    SOCKET sServer;
    struct sockaddr_in saClient;
    int len = sizeof(saClient);

	//parameters for select model
    fd_set sock_set;
	FD_ZERO(&sock_set);
    timeval timeout = {1,0};	//timeout for select, set 1s
 
    while(true){
        //select model to avoid infinite stall by accept()
		do {
			FD_SET(sListen, &sock_set);								//add sListen to sock_set
			select(0, &sock_set, nullptr, nullptr, &timeout);		//check the state of sListen
		} while ((FD_ISSET(sListen, &sock_set) == 0)&&(!b_quit));	//if sListen is not readable and b_quit is false,continue loop
		if (b_quit) break;		//if quit flag b_qui is true,quit loop

        sServer = accept(sListen,(struct sockaddr*)&saClient,&len);		//accept a request
        if(sServer == INVALID_SOCKET){
            std::cout<<"accept() failed! error code:"<<WSAGetLastError()<<std::endl;
            break;
		}
		else {
			std::cout<<"Accepted client: "<<inet_ntoa(saClient.sin_addr)<<":"<<ntohs(saClient.sin_port)<<std::endl;		//print client information
			std::thread* pt_tmp = new std::thread(thread_serve_client, sServer);		//start a thread to handle message
			vec_pThreads.push_back(pt_tmp);
		}
    }

    //join to wait all threads return
	for (int i = 0; i < vec_pThreads.size(); i++) {
		vec_pThreads[i]->join();	//join thread
		delete vec_pThreads[i];		//release memory
	}
	t_quit.join();	//wait t_quit thread
    
	//clean socket parameters
    closesocket(sListen);
    WSACleanup();

    return 0;
}

void thread_listen_quit(bool& flag_quit){
	std::string str_cmd;
	std::cout << "Waiting for client connecting!" << std::endl;
	std::cout << "Enter q to quit!" << std::endl;
    do{
        std::cin>>str_cmd;
	} while (str_cmd != "q");

	flag_quit = true;
}

void thread_serve_client(SOCKET sServerLinked){
    //parameters for resolution
    char c0,c1,c2,c_cur;
    c0 = c1 = c2 = c_cur='\0';
    std::string str_head,str_tmp;

    //resolution results and response member
    std::string str_method,str_path,str_fileType;
    std::string str_content;

    std::ifstream fi;

	//read HEAD,get method,path and file type
    while(true){
        c0 = c1; c1 = c2; c2 = c_cur;       //circle assign
        recv(sServerLinked,&c_cur,1,0);
		str_head.push_back(c_cur);
        if((c0 == '\r')&&(c1 == '\n')&&(c2 == '\r')&&(c_cur == '\n')){   //have read the Method and headers of HTTP request
			//get method
            str_method = str_head.substr(0, str_head.find(' '));
			//get path
            str_tmp = str_head.substr(str_head.find(' ')+1);
            str_path = str_tmp.substr(0,str_tmp.find(' '));
			if (str_path == "/") str_path.append("html/test.html");		//set default web page path to "/"
			//get file type
            str_fileType = str_path.substr(str_path.rfind('.')+1);
			str_path = SRC_DIR + str_path;
			//print simple infomation of message received
            std::cout<<"receive method: "<<str_method<<"\tpath: "<<str_path<<std::endl; 
			break;
        }
    }

	//handle methods
	if (str_method == "GET") {   		 //this is a GET request
		if (_access(str_path.c_str(), 0) != 0) send_404(sServerLinked);		//file doesn't exist, return 404 message to client
		else send_file(sServerLinked, str_path, str_fileType);				//file exist send http message to client
	}
	else if (str_method == "POST") {     //this is a POST request
		if (str_path.substr(str_path.rfind('/')+1) != "dopost") send_404(sServerLinked);	//the action is not dopost, return 404 message to client
		else send_post_response(sServerLinked, str_head);					//action is dopost, handle the body and return a message of login
	}
	closesocket(sServerLinked);			//just handle one request ,then close socket
}

void send_404(SOCKET sServer) {
	std::string str_buf;
	str_buf += "404 Not Found!\r\n";
	send(sServer, str_buf.c_str(), str_buf.length(), 0);
}

void send_file(SOCKET sServer, std::string& str_path, std::string& str_fileType) {
	std::string str_buf;		//buf for packet to send
	std::string str_content;	//content of file 

	std::ifstream fi;
	//fill 200 for http
	str_buf += "HTTP/1.1 200 OK\r\n";

	//fill headers
	//content-type
	if (str_fileType == "html" || str_fileType == "txt") {	//.html and .txt
		if(str_fileType == "html") str_buf += "Content-Type: text/html\r\n";
		else str_buf += "Content-Type: text/plain\r\n";
		fi = std::ifstream(str_path);
		if (!fi.is_open()) {
			std::cout << "open file failed!" << std::endl;
			return;
		}
	}
	else if (str_fileType == "jpg") {						//.jpg
		str_buf += "Content-Type: image/jpeg\r\n";
		fi = std::ifstream(str_path, std::ios_base::binary);
		if (!fi.is_open()) {
			std::cout << "open file failed!" << std::endl;
			return;
		}
	}

	//read file's content to str_content
	char c;
	while (fi.get(c)) {
		str_content.push_back(c);
	}

	//content-length
	str_buf += "Content-Length: " + std::to_string(str_content.length());
	str_buf += "\r\n";

	//connection
	str_buf += "Connection: keep-alive,close\r\n";

	//body
	str_buf += "\r\n";
	str_buf += str_content;
	str_buf += "\r\n";

	//send final packet to client through sServer
	send(sServer, str_buf.c_str(), str_buf.length(), 0);
}

void send_post_response(SOCKET sServer, std::string& str_head) {
	//get length of body from client
	int n_conLen = -1;			//length
	std::string str_conLen;		
	int i_ret;

	do {
		i_ret = str_head.find('\r');
		std::string head_tmp = str_head.substr(0, i_ret);
		if (head_tmp.substr(0, std::string("Content-Length").length()) == "Content-Length") {	//find the line of head"Content-Length"
			auto i_f = head_tmp.find(' ');
			auto i_l = head_tmp.find('\r');
			str_conLen = head_tmp.substr(i_f + 1, i_l - i_f - 1);
			std::stringstream ss(str_conLen);
			ss >> n_conLen;
			break;
		}
		else str_head = str_head.substr(i_ret + 2);
	} while (i_ret != str_head.length() - 2);

	//read the body of request
	char c;
	int cnt = 0;
	std::string str_body;
	std::string str_login, str_pass;
	while (cnt < n_conLen) {
		recv(sServer, &c, 1, 0);
		str_body.push_back(c);
		cnt++;
	}

	//print body content
	std::cout << str_body << std::endl;

	//get value of "login" and "pass"
	auto i_f = str_body.find('=');
	auto i_l = str_body.find('&');
	str_login = str_body.substr(i_f + 1, i_l - i_f - 1);
	i_f = str_body.rfind('=');
	str_pass = str_body.substr(i_f + 1);

	//fill body of reponse with a html format
	std::string str_buf, str_resBody;

	str_resBody.append("<html><body>Login ");
	if ((str_login == "3160101256") && (str_pass == "1256")) str_resBody.append("Succeed!");
	else str_resBody.append("Failed!");
	str_resBody.append("</body></html>");

	//fill headers of HTTP packet
	str_buf.append("HTTP/1.1 200 OK\r\n");
	str_buf.append("Content-Type: txt/html\r\n");
	str_buf.append("Content-Length: " + std::to_string(str_resBody.length())+"\r\n");
	str_buf.append("Connection: keep-alive,close\r\n");
	str_buf.append("\r\n");
	str_buf.append(str_resBody);

	//send reponse to client
	send(sServer, str_buf.c_str(), str_buf.length(), 0);
}
