#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

void * rebootdsp(void * p)
{
		int    fd;
		struct sockaddr_in  connaddr;
		char    buf[512];
		int		ret;
		socklen_t   opt_len = 0;
		unsigned int ip  = 0;

		fd = socket(AF_INET, SOCK_STREAM, 0);

		ip=(unsigned int)p;

		memset(&connaddr , 0 , sizeof(connaddr));
		connaddr.sin_family      = AF_INET;
		connaddr.sin_addr.s_addr = htonl(ip);//inet_addr("192.168.200.30");//htonl(dspip);
		connaddr.sin_port        = htons(23);


		ret = connect(fd , (struct sockaddr*)&connaddr , sizeof(connaddr));
		if(-1 == ret)
		{
				perror("connect ");
				close(fd);
				return -1;
		}
		//send(fd , "\r\n" , 2 , 0);
		while(1)
		{
				recv(fd , buf , 100 , 0);
			//	printf(buf);
			//	printf("\n");
				if(NULL != strstr((const char*)buf , "login:"))
				{
						char root[16] = {'r','o','o','t','\r','\n',0,0,0};
			//			printf("1111111111111111111111111111111111111111111111111111111111111111111111111\n");
						write(fd , root , 6);
						sleep(1);
						continue;
				}

				if(NULL != strstr((const char *)buf , "Password:"))
				{
						char wisdom[16] ={'w','i','s' , 'd','o' , 'm' , '\r', '\n'}; 

			//			printf("22222222222222222222222222222222222222222222222222222222222222222222222222\n");
						write(fd , wisdom , 8);
						sleep(1);
						continue;
				}

				if(NULL != strstr((const char *)buf , "root@(none) #"))
				{
						char reboot[16] = "reboot\r\n";

			//			printf("3333333333333333333333333333333333333333333333333333333333333333333333333\n");
						sleep(3);
						write(fd , reboot , 8);
						sleep(2);
						close(fd);
						return NULL;	
				}			
		}

}
int main(int argc , char * argv[])
{
		int i , j;
		unsigned int ip = 0;
		pthread_t tid[128];

		printf("argv[1] %s , argv[2] : %d\n" , argv[1] , atoi(argv[2]));

		ip = ntohl(inet_addr(argv[1]));

		for(i = 0 ; i < atoi(argv[2]) ; i++)
		{
				pthread_create(&tid[i] , NULL , rebootdsp , ip + i);

//			printf("pthread_join(&tid[%d] , NULL);\n" , i);
//			pthread_join(&tid[i] , NULL);
		}

		sleep(15);
/*
		for( j = 0 ; j < i ; j++)
		{
			printf("pthread_join(&tid[%d] , NULL);\n" , j);
			pthread_join(&tid[j] , NULL);
		}
*/
		return 0;
}
