#include<sys/select.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/stat.h>
#include"getFileType.h"
#include<fcntl.h>
#include<sys/sendfile.h>
#include<dirent.h>

int main()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		perror("socket");
	}
	int opt = 1;
	int ret=setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret==-1)
	{
		perror("setsockopt");
	}
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(9999);
	saddr.sin_addr.s_addr = INADDR_ANY;
	socklen_t size_s = sizeof saddr;
	ret = bind(fd, (struct sockaddr*)&saddr, size_s);
	if (ret < 0)
	{
		perror("bind 9999");
	}
	ret = listen(fd, 1);
	if (ret < 0)
	{
		perror("listen");
	}
	int maxfd = fd;
	fd_set rdfd;
	fd_set rdtp;
	FD_ZERO(&rdfd);
	FD_SET(fd, &rdfd);
	while (1)
	{
		rdtp = rdfd;
		int num = select(maxfd + 1, &rdtp, NULL, NULL,NULL);
		if (FD_ISSET(fd, &rdtp))
		{
			int cfd = accept(fd, NULL, NULL);
			if (cfd < 0)
			{
				perror("accept");
			}
			FD_SET(cfd, &rdfd);
			maxfd = maxfd > cfd ? maxfd : cfd;
		}
		for (int i = 0; i < maxfd + 1; i++)
		{
			if (i != fd && FD_ISSET(i, &rdfd))
			{
				char buf[4096] = { 0 };
				char tmp[1024] = { 0 };
				int len = 0, total = 0;
				while ((len = recv(fd, tmp, 1024, 0) > 0))
				{
					if (len+total<sizeof buf)
					{
						memcpy(buf + total, tmp, len);
					}
					total += len;
				}
				if (len==-1&&errno==EAGAIN)
				{
					//解析请求行
					char* ptr = memmem(buf, 4096, "\r\n", sizeof("\r\n"));
					len = ptr - buf;
					buf[len] = '\0';
					parseRue(fd, buf);
				}
				else if (len==0)
				{
					//断开连接
					FD_CLR(fd, &rdfd);
					close(fd);
				}
				else
				{
					perror("recv");
				}
			}
		}
	}
}


void parseRue(int fd,const char* buf)
{
	char* mathod;
	char* path;
	int ret = sscanf(buf, "%[^ ] %[^ ]", mathod, path);
	if (strcasecmp(mathod, "get") != 0)
	{
		return -1;
	}
	
	if (strcmp(path, "/") != 0)
	{
		path = path + 1;
	}
	else
	{
		path = "./";
	}
	struct stat st;
	int ret=stat(path, &st);
	if (ret == -1)
	{
		sendHeadMsg(fd, 404, "NOT Found", getFileType(".html"), -1);
	}
	if (S_ISDIR(st.st_mode))//文件夹
	{

	}
	else//文件
	{

	}
}

void sendHeadMsg(int cfd,int status,char* desr,char* type,int length)
{
	char* buf;
	sprintf(buf, "http/1.1 %d %s\r\n", status, desr);
	sprintf(buf + sizeof(buf),"content-type: %s\r\n",type);
	sprintf(buf + sizeof(buf), "conten-length: %s\r\n", length);
	int ret=send(cfd, buf, sizeof(buf), 0);
	if (ret == -1)
	{
		perror("send");
	}
}

void sendFile(int fd, char* file)
{
	assert(fd > 0 && file != NULL);
	int cfd = open(file, O_RDONLY);
	if (cfd < 0)
	{
		perror("open");
	}
#if 0
	while (1)
	{
		char buf[1024];
		int len = read(cfd, buf, 1024);
		if (len > 0)
		{
			send(fd, buf, sizeof buf, 0);
			usleep(10);
		}
		else if(len == -1 && errno == EAGAIN)
		{
			
		}
		else
		{

		}
}
#else
	size_t off = 0;
	size_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	while (off<size)
	{
		int ret = sendfile(fd, cfd, &off, size-off);
		if (ret == -1 && errno == EAGAIN)
		{
			printf("读取完成\n");
		}
	}
#endif
	close(cfd);
}

void sendDir(int fd,char* file)
{
	assert(fd > 0 && file != NULL);
	char* buf = ".html";
	struct dirent** namelist;
	int num=scandir(file, &namelist, NULL, alphasort);
	for (int i = 0; i < num; i++)
	{
		char* name = namelist[i]->d_name;
		char* subname;
		sprintf(subname, "%s/%s", file, name);
		struct stat st;
		stat(subname, &st);
		if (S_ISDIR(st.st_mode))
		{

		}
		else
		{

		}
	}
}

