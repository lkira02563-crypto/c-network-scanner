#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include<netinet/ip.h>
 #include<netinet/tcp.h>
 #include<netinet/udp.h>
 #include<arpa/inet.h>
#include <linux/if_ether.h>
#include<netinet/ip_icmp.h> 
 #include<sys/select.h>
#include <linux/if_packet.h>
 #include <net/if.h>
#include <sys/ioctl.h>
#include<time.h>
#include <assert.h>
#include <errno.h>


//linked list to store infos.

typedef struct infos {
    char ip[INET_ADDRSTRLEN];
    int ports[1000];
    struct infos *next;
} infos;



// simple func to chop off the last number in ip address.

char chop_letters(char word[INET_ADDRSTRLEN], int offset)
{
    int len = strlen(word);

    if (offset <= len)
        return  word[len - offset] = '\0';
}


 // this is how i decided to handle that but u can do it way u see it better those and the major parts.

int major_ports[20] = {
    20,   // FTP data
    21,   // FTP control
    22,   // SSH
    23,   // Telnet
    25,   // SMTP
    53,   // DNS
    80,   // HTTP
    110,  // POP3
    111,  // RPCbind
    135,  // MS RPC
    139,  // NetBIOS
    143,  // IMAP
    443,  // HTTPS
    445,  // SMB
    3389, // RDP
    3306, // MySQL
    5432, // PostgreSQL
    6379, // Redis
    8080, // HTTP-alt
    8443  // HTTPS-alt
};
//func to establish connection with a giving ip address and port.
//its is slow since it open over 21 conncetion for each port ofc u can decrease that.

int tcp_server(const char *ip, int  port){
  int fd=socket(AF_INET, SOCK_STREAM, 0);
  struct timeval tv;
tv.tv_sec = 0;
tv.tv_usec = 200000; 

setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  assert(fd>=0);
  struct sockaddr_in target;
     memset(&target, 0, sizeof(target));
  target.sin_family=AF_INET;
  target.sin_port=htons(port);
  inet_pton(AF_INET, ip, &(target.sin_addr));
 
    int con = connect(fd, (struct sockaddr *)&target, sizeof(target));

   
// port is open.
    if (con == 0) {
        close(fd);
        return 1;
    }
// port is filtered.
    if (errno == ECONNREFUSED) {
        close(fd);
        return 0;
    }
  //anything else means its closed.
close(fd);
  return 0;
}
// an ip address is usually 192.168.X.X ofc over the /24 so we left with host being in range of 0-255 that isnt always true tho.
// this func captures an ip address with 192.168 then take the network part again over the /24 .
// its not optimal but it works :).

char *finding_network(char filtered_ip[INET_ADDRSTRLEN])
{
 
    int sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
  assert(sock_raw>=0);
char  captured_ip[INET_ADDRSTRLEN];
   // this is how to catch ip
 struct packet_mreq mr;
   memset(&mr, 0, sizeof(mr));
   mr.mr_ifindex=if_nametoindex("wan0");
   mr.mr_type= PACKET_MR_PROMISC;

setsockopt(sock_raw, SOL_SOCKET,  PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr));


    unsigned char buffer[70000];

    for (;;) {
        int bytes = recvfrom(sock_raw, buffer, sizeof(buffer), 0, NULL, NULL);
 if (bytes < 14 + sizeof(struct iphdr)) {
            continue;
        }
        
        struct iphdr *ip_i = (struct iphdr *)(buffer + 14);

    inet_ntop(AF_INET, &(ip_i->saddr), captured_ip, sizeof(captured_ip));
     


        if (strncmp(captured_ip, "192.168", 7) == 0) {
           
chop_letters(captured_ip, 3);
      strcpy(filtered_ip, captured_ip);

           // its impotant to close the the sock_raw in here so it will loop over in case it isnt what we looking for. 

            
      close(sock_raw);
      break;
        }
    }

    
  return filtered_ip;
}


infos *head = NULL;


// this func to check over all ports of a giving ip .
// and also link them up using a lined list.

int host_alive(const char* ip) {

    infos *node = malloc(sizeof(infos));
    if (!node) return 0;
   memset(node, 0, sizeof(infos));
   memset(node->ports, 0, sizeof(node->ports));
    strcpy(node->ip, ip);
   
    node->next = NULL;

    
    if (head == NULL) {
        head = node;
    } else {
        infos *tmp = head;
        while (tmp->next != NULL) {
            tmp = tmp->next;
	 
	    
        }
        tmp->next = node;
    }
    int alive=0;
    // scan ports
    for (int i = 0; i < 20; i++) {
        if (tcp_server(ip, major_ports[i]) == 1) {
            node->ports[i] = major_ports[i];
            alive=1;
        }else{
	  node->ports[i]=0;
	}
    }

    return alive;
}







int main(){
  infos *f;
  int ups=0, downs=0;
  char host[100][32]={0};
  
  char ip[40];
  
  int j=0;
  char filtered_ip[32];
 finding_network(filtered_ip);

//printf("network found: %s\n", filtered_ip); 
   for (int i = 1; i <= 225; i++) {
  
     snprintf(ip,sizeof(ip), "%s%d",filtered_ip, i);
    


        if (host_alive(ip)>0) {
            printf("[+] Host up: %s\n", ip);
	    strcpy(host[j++], ip);
	    ups++;
	     
	     
	     
        }else{

	  printf("[-] Host is down: %s\n", ip);
	  
	  downs++;
	}
    }

   printf("hosts up are %d, not ups are %d\n", ups, downs);
    for(int i=0; i<ups; i++)
    {
    printf("%s\n", host[i]);
    
    }
    f=head;
    while(f!=NULL){
      printf("ip::%s\n", f->ip);
    for(int i=0; i<20; i++){
if(f->ports[i] >0){
      printf(" open ports are %d\n",f->ports[i]);
    }
      }
      f=f->next;
    }
  return 0;
  
}




