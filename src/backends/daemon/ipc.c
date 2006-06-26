#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#include "ipc.h"

#define IPCPATH_MAX	100

int ipc_stream(void)
{
	int s;
	
	s = socket(AF_UNIX,SOCK_STREAM,0);
	if (s == -1) return -1;
	if (ndelay_on(s) == -1) { close(s); return -1; }
	return s;
}

static int ipc_bindit(int s,const char *p,int del)
{
	struct sockaddr_un sa;
	unsigned int l;
	
	l = strlen(p);
	if (l > IPCPATH_MAX) {
		errno = EISDIR;
		return -1;
	}
	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	memcpy(sa.sun_path, p, l);
	if (del) unlink(sa.sun_path);
	return bind(s,(struct sockaddr *) &sa,sizeof sa);
}

int ipc_bind(int s,const char *p)
{
	  return ipc_bindit(s,p,0);
}

int ipc_bind_reuse(int s,const char *p)
{
	  return ipc_bindit(s,p,1);
}

int ipc_accept(int s,char *p,int l,int *trunc)
{
	int fd;
	struct sockaddr_un sa;
	int dummy = sizeof sa;
	
	memset(&sa, 0, sizeof(sa));
	fd = accept(s,(struct sockaddr *) &sa,&dummy);
	if (fd == -1) return -1;
	
	memset(sa.sun_path, 0, dummy);
	
	*trunc = 1;
	if (!l) return fd;
	
	if (l < (dummy + 1))
		dummy = l - 1;
	else
		*trunc = 0;

	memcpy(p, sa.sun_path, dummy);	
	p[dummy] = 0;
	
	return fd;
}

int ipc_local(int s,char *p,int l,int *trunc)
{
	struct sockaddr_un sa;
	int dummy = sizeof sa;
	
	memset(&sa, 0, sizeof(sa));
	if (getsockname(s,(struct sockaddr *) &sa,&dummy) == -1) return -1;

	memset(sa.sun_path, 0, dummy);	
	
	*trunc = 1;
	if (!l) return 0;
	
	if (l < (dummy + 1))
		dummy = l - 1;
	else
		*trunc = 0;
	
	memcpy(p, sa.sun_path, dummy);
	p[dummy] = 0;
	
	return 0;
}

int ipc_connect(int s,const char *p)
{
	struct sockaddr_un sa;
	unsigned int l;
	
	l = strlen(p);
	if (l > IPCPATH_MAX) {
		return -1;
	}
	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	memcpy(sa.sun_path, p, l);
	
	if (connect(s,(struct sockaddr *) &sa,sizeof sa) == -1) return -1;
	
	if (ndelay_off(s) == -1) return -1;
		return 0;
}

int ipc_listen(int s,int backlog)
{
	return listen(s,backlog);
}

int ndelay_on(int fd)
{
	  return fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0) | O_NONBLOCK);
}

int ndelay_off(int fd)
{
	return fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0) & ~O_NONBLOCK);
}

int ipc_eid(int s,int *u,int *g)
{
	uid_t dummyu;
	gid_t dummyg;
	
	if (getpeereid(s,&dummyu,&dummyg) == -1) 
		return -1;
	
	*u = dummyu;
	*g = dummyg;
	
	return 0;
}

int getpeereid(int s,uid_t *u,gid_t *g)
{
	struct ucred dummy = {0};
	int len = sizeof(dummy);
	
	if (getsockopt(s,SOL_SOCKET,SO_PEERCRED,&dummy,&len) == -1)
		return -1;
	*u = dummy.uid;
	*g = dummy.gid;

	return 0;
}