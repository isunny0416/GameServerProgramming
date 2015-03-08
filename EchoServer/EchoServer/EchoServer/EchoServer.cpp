#include "stdafx.h"
#include <WinSock2.h>	//Winsock �Լ��� ����. winsock�� winsock2�� ���̴� ����
#include <stdio.h>
#include <vector>
#include <map>

#pragma warning(disable: 4996) //error C4996�� �Ѿ�� ���ؼ� ����. VS ���������� warning�̴� �� 2013���� error�� ó���ȴٴµ� �׷��� �ڵ带 ��� �ٲٶ�� ������ ���� �𸣰ڴ�
#pragma comment(lib, "ws2_32.lib") //Winsock ����� ���� �ʿ�

#define BUFSIZE 4096	//���� ũ��
#define PORTNUM 9001	//��Ʈ ��ȣ
#define WM_SOCKET WM_USER +1 //WM_USER : private �޽����� ����� �� ���. WM_USER + n ���·� ���. WM_APP�� ��Ȯ�� ��� �ٸ��� �� �𸣰ڴ�. 

SOCKET listenerSocket = NULL;

struct Session
{
	char recvBuf[BUFSIZE];
};

//std::vector<Session*> Sessions;
std::map<SOCKET, Session*> SessionMap;


void printError(const char* processName, int errCode){
	printf_s("There was an error while running \"%s\", Error code : %d\n", processName, errCode);
	return;
}

bool initListener(HWND hwnd){
	WSADATA wsaData;
	SOCKADDR_IN addr;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
		printError("WSAStartUp", WSAGetLastError());
		return false;
	}

	listenerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenerSocket == INVALID_SOCKET){
		printError("socket", WSAGetLastError());
		return false;
	}
	
	int opt = 1;
	setsockopt(listenerSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));
	
	ZeroMemory(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORTNUM);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int result = bind(listenerSocket, (struct sockaddr*)&addr, sizeof(addr));
	if (result != 0){
		printError("bind", WSAGetLastError());
		return false;
	}

	if (listen(listenerSocket, SOMAXCONN) != 0)
	{
		printError("listen", WSAGetLastError());
		return false;
	}

	if (WSAAsyncSelect(listenerSocket, hwnd, WM_SOCKET, FD_ACCEPT | FD_CLOSE) != 0){
		printError("WSAAsyncSelect", WSAGetLastError());
		return false;
	}

	printf("socket init completed.");
	return true;
}



LRESULT CALLBACK Winproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	SOCKET serverSocket = 0;
	SOCKADDR_IN addr;
	Session* session = nullptr;

	int addrLen = 0;

	if (uMsg == WM_CREATE){
		initListener(hwnd);
	}
	if (uMsg == WM_SOCKET){
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_ACCEPT:
			addrLen = sizeof(addr);
			serverSocket = accept(wParam, (SOCKADDR*)&addr, &addrLen);
			if (serverSocket == INVALID_SOCKET){
				printError("accept", WSAGetLastError());
				return -1;
			}

			printf_s("Accepted. ip : %s   port : %d   \n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

			session = new Session();
			SessionMap[serverSocket] = session;
			
			if (WSAAsyncSelect(serverSocket, hwnd, WM_SOCKET, FD_WRITE | FD_READ | FD_CLOSE) == SOCKET_ERROR){
				closesocket(serverSocket);
				printError("WSAAsyncSelect", WSAGetLastError());
				return -1;
			}
				
			break;
		case FD_READ:
			if (SessionMap.find(wParam) != SessionMap.end())
			{
				session = SessionMap[wParam];
				if (recv(wParam, session->recvBuf, BUFSIZE, NULL) == SOCKET_ERROR)
				{
					closesocket(wParam);
					SessionMap.erase(wParam);
					printError("recv", WSAGetLastError());
					return -1;
				}

				printf_s("Received. socket : %d,   recvBuf : %s\n", wParam, session->recvBuf);

				if (send(wParam, session->recvBuf, BUFSIZE ,NULL) == SOCKET_ERROR)
				{
					closesocket(wParam);
					SessionMap.erase(wParam);
					printError("send", WSAGetLastError());
					return -1;
				}

				printf_s("Sended. socket : %d,   recvBuf : %s\n", wParam, session->recvBuf);

			}
			break;
		case FD_CLOSE:
			closesocket(serverSocket);
			break;
		default:
			break;
		}
	}
	return 1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	MSG msg;
	WNDCLASSEX windowClass;
	HWND window;
	char* className = "EchoServer";

	windowClass.cbSize = sizeof(WNDCLASSEX); //size of this structure. set before calling the GetClassInfoEx().
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW; // class style. can be any combination of the Window Class Styles. 
	windowClass.lpfnWndProc = (WNDPROC)Winproc; // pointer to window procedure. (WindowProc callback function)
	windowClass.cbClsExtra = 0; // The number of extra bytes to allocate following the window-class structure. The system initializes the bytes to zero.
	windowClass.cbWndExtra = 0; // extra bytes to allocate following the window instance.
	windowClass.hInstance = NULL; //A handle to the instance that contains the window procedure for the class.
	windowClass.hIcon = NULL; //handle to the class icon. must be a handle to an icon resource. NULL == default icon.
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW); //handle to the class cursor. handle to a cursor resource.
	windowClass.hbrBackground = (HBRUSH)COLOR_WINDOWFRAME; // handle to the class background brush.
	windowClass.lpszMenuName = NULL; //Pointer to a null-terminated character string that specifies the resource name of the class menu. NULL == no default menu.
	windowClass.lpszClassName = (LPCWSTR)className; // specifies the window class name. The class name can be any name registered with RegisterClass or RegisterClassEx. max 256
	windowClass.hIconSm = NULL; //handle to a small icon.

	RegisterClassEx(&windowClass);

	window = CreateWindow((LPCWSTR)className, (LPCWSTR)className, WS_OVERLAPPEDWINDOW, 200, 200, 600, 300, NULL, 0, NULL, NULL);

	while (GetMessage(&msg, 0, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

