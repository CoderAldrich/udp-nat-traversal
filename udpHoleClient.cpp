#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pwd.h>


#define SOCKADDR_IN sockaddr_in
#define SOCKADDR sockaddr
#define SOCKET int
#define INVALID_SOCKET (-1)
/**
 *  * This client connects to the address and port of the server. It proceeds to
 *   * ping-pong data with another instance of itself once a second client connects
 *    * to the relay server.
 *     */

int udpHoleClient();

int main()
{
   return  udpHoleClient();
}

int udpHoleClient()
{

    SOCKET  sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == INVALID_SOCKET)
    {
        printf("create socket failed\n");
        return 0;
    }


    SOCKADDR_IN  myaddr = {0};
    myaddr.sin_port = htons(rand() % 800 + 9001);
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    int ret = 0;

    int val = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)val, sizeof(val));

    ret = bind(sock, (SOCKADDR*)&myaddr, sizeof(SOCKADDR_IN));
    if(ret == -1)
    {
        printf("bind failed  error:%d\n", errno);
        close(sock);
        return 0;
    }
    SOCKADDR_IN servAddr = {0};
    servAddr.sin_port = htons(18901);
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("123.147.223.124");
    //servAddr.sin_addr.S_un.S_addr = inet_addr("192.168.1.20");
    char buf[0x20] = {0};

    ret = sendto(sock, buf, 0x10, 0, (SOCKADDR*)&servAddr, sizeof(SOCKADDR_IN));

    SOCKADDR_IN recvAddr = {0};
    socklen_t addrLen = sizeof(SOCKADDR_IN);
    printf("wait recv peer addr...\n");
    ret = recvfrom(sock, buf, 12, 0, (SOCKADDR*)&recvAddr, &addrLen);
    if(ret == -1)
    {
        printf("recv failed  error:%d\n",errno);
        close(sock);
        return 0;
    }

    printf("recv from: %s:%d\n", inet_ntoa(recvAddr.sin_addr), ntohs(recvAddr.sin_port));

    SOCKADDR_IN  peerAddr = {0};
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_addr.s_addr = *(int*)buf;
   // peerAddr.sin_addr.s_addr = 0x7B93DF7B;
    peerAddr.sin_port = *(short*)&buf[4];
    
    struct in_addr selfIp={0};
    selfIp.s_addr = *(int*)&buf[6];
    short selfPort = *(short*)&buf[10];
    if(peerAddr.sin_addr.s_addr == selfIp.s_addr)
    {
        printf("no need NAT hole, you and peer in the back of the same NAT\n");
        close(sock);
        return 0;
    }

    printf("recv data: My( %s:%d ) peer( %s:%d ) \n",
	 inet_ntoa(selfIp), ntohs(selfPort),
	 inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
    struct timeval timeout = {0,300000};//300ms
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
    char msg[0x20] = "0123456";
    struct passwd *pwd = getpwuid(getuid());
    sprintf(msg,"%d:[%s] hello",getpid(),pwd->pw_name);
    printf("send msg to peer: %s \n",msg);
    printf("wait peer back...\n");
    for(int i = 0; i < 5 ; ++i)
    {
        ret = sendto(sock, msg, strlen(msg) + 1, 0, (SOCKADDR*)&peerAddr, sizeof(SOCKADDR_IN));
        addrLen = sizeof(SOCKADDR_IN);
	ret = recvfrom(sock,buf,0x20,0,(SOCKADDR*)&recvAddr, &addrLen);
	if(ret >=0)
	{
	    break;
	}
    }
    if(ret<1)
    {
	printf("udp hole  failed!! errno:%d",errno);
        close(sock);
        return 0;
    }
    printf("recv from: %s:%d\n", inet_ntoa(recvAddr.sin_addr), ntohs(recvAddr.sin_port));
    printf("data: %s\n", buf);
    ret = sendto(sock, msg, strlen(msg) + 1, 0, (SOCKADDR*)&recvAddr, sizeof(SOCKADDR_IN));
    sleep(2);
    close(sock);
    return 0;

}
