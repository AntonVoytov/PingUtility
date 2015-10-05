#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <strings.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define ICMP_SIZE 64
#define PING_TIMES 10
#define BUFF_SIZE 100

struct icmp_packet
{
    struct icmphdr icmp_header;
    char message[ICMP_SIZE - sizeof(struct icmphdr)];
};

int count = 1;

unsigned short checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;

    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int ping(char *address, const int ttl)
{
    int sock;
    struct hostent *host_name;
    struct sockaddr_in addr_ping, reply_addr, *paddr_ping;
    struct icmp_packet i_packet;
    int rlen, i, j, recv_size;
    pid_t pid;
    char *hr_reply_addr;

    pid = getpid();
    host_name = gethostbyname(address);
    bzero(&addr_ping, sizeof(addr_ping));
    addr_ping.sin_family = host_name->h_addrtype;
    addr_ping.sin_port = 0;
    addr_ping.sin_addr.s_addr = *(long*)host_name->h_addr;
    paddr_ping = &addr_ping;

    sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if ( sock < 0)
    {
        perror("Error socket");
        return 1;
    }
    if ( setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) != 0)
    {
        perror("Error set sock option");
        return 1;
    }

    for (i = 0; i < PING_TIMES; i++)
    {
        rlen = sizeof(reply_addr);

        bzero(&i_packet, sizeof(i_packet));
        i_packet.icmp_header.type = ICMP_ECHO;
        i_packet.icmp_header.un.echo.id = pid;
        i_packet.icmp_header.un.echo.sequence = count++;
        for (j = 0; j < sizeof(i_packet.message) - 1; j++)
            i_packet.message[j] = j + '0';
        i_packet.message[j] = 0;
        i_packet.icmp_header.checksum = checksum(&i_packet, sizeof(i_packet));

        if( sendto(sock, &i_packet, sizeof(i_packet), 0, (struct sockaddr*) paddr_ping, sizeof(*paddr_ping) ) < 0)
        {
            perror("error send");
        }
        sleep(1);

        if ( (recv_size = recvfrom(sock, &i_packet, sizeof(i_packet), 0, (struct sockaddr *)&reply_addr, &rlen)) > 0)
        {
            hr_reply_addr = inet_ntoa(reply_addr.sin_addr);
            printf("receive %d bytes from %s\n", recv_size, hr_reply_addr);
            close(sock);
            return 0;
        }
    }
}

int main(void)
{
    char addr_name[BUFF_SIZE];
    int ttl;
    printf("Write address to ping (example www.google.com or 127.0.0.1)\n");
    scanf("%s", addr_name);
    printf("Write ttl(from 1 to 255 better then ttl > 10)\n");
    scanf("%d", &ttl);
    if ( ping(addr_name, ttl))
        printf("ping not ok\n");
    else
        printf("ping ok\n");


    return 0;
}

