#pragma comment(lib,"ws2_32.lib")
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <process.h>

using namespace std;
#define MAX_CLNT 1024
#define BUF_SIZE 100

int clntCnt = 0;
SOCKET clntSocks[MAX_CLNT];
HANDLE hMutex;

unsigned WINAPI HandleClnt(void *arg);
void SendMsg(char *msg, int len);
int main() {
  WSADATA wsaData;
  SOCKET hServSock, hClntSock;
  SOCKADDR_IN servAddr, clntAddr;
  HANDLE hThread;

  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    cout << "wsastartup err" << endl;
    exit(1);
  }
  
  hMutex = CreateMutex(NULL, FALSE, NULL); // 디폴트 보안설정 NULL, 소유자 존재하지않음 FALSE, 이름없는 MUTEX 생성
  hServSock = socket(PF_INET, SOCK_STREAM, 0);

  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(9090);

  if (bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
    cout << "bind err" << endl;
    exit(1);
  }
  if (listen(hServSock, 5) == SOCKET_ERROR) {
    cout << "listen err" << endl;
    exit(1);
  }

  int clntAddrsize = sizeof(clntAddr);
  while (1) {
    hClntSock = accept(hServSock, (SOCKADDR*)&clntAddr, &clntAddrsize);

    WaitForSingleObject(hMutex, INFINITE);
    clntSocks[clntCnt++] = hClntSock; // 임계영역 보호
    ReleaseMutex(hMutex);
    
    hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClnt, (void*)&hClntSock, 0, NULL); // 요청시마다 스레드 생성
    printf("Connected IP : %s \n", inet_ntoa(clntAddr.sin_addr));
    printf("THREAD HANDLE Num : %d\n", hThread);
    

  }
  
  closesocket(hServSock);
  WSACleanup();
  return 0;
}

unsigned WINAPI HandleClnt(void *arg) {
  SOCKET hClntSock = *((SOCKET*)arg);
  char msg[BUF_SIZE];
  int strLen;
  while (strLen = recv(hClntSock, msg, sizeof(msg), 0)) {
    if (strLen <= 0){  // 정확하게 이렇게 해주는게 좋다(연결 종료, EOF)
      break;
    }
    fputs(msg, stdout);
    SendMsg(msg, strLen);
  }

  WaitForSingleObject(hMutex, INFINITE);
  for (int i = 0; i < clntCnt; i++) {
    if (hClntSock == clntSocks[i]) {
      printf("close sock : %d\n", i);
      while (i++ < clntCnt - 1) {
        clntSocks[i] = clntSocks[i + 1];
      }
      break;
    }
  }
  clntCnt--;
  ReleaseMutex(hMutex);
  cout << "연결종료" << endl;
  closesocket(hClntSock);
  return 0;
}
void SendMsg(char *msg, int len) {
  WaitForSingleObject(hMutex, INFINITE);
  for (int i = 0; i < clntCnt; i++) {
    send(clntSocks[i], msg, BUF_SIZE, 0);
  }
  ReleaseMutex(hMutex);
}