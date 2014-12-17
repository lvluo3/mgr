#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define SA  struct sockaddr



void err_sys(const char *fmt, ...);

            /* prototypes for our socket wrapper functions: see {Sec errors} */
int      Accept(int, SA *, socklen_t *);
void     Bind(int, const SA *, socklen_t);
void     Connect(int, const SA *, socklen_t);
void     Getpeername(int, SA *, socklen_t *);
void     Getsockname(int, SA *, socklen_t *);
void     Getsockopt(int, int, int, void *, socklen_t *);

void     Listen(int, int);
ssize_t  Readline(int, void *, size_t);
ssize_t  Readn(int, void *, size_t);
ssize_t  Recv(int, void *, size_t, int);
ssize_t  Recvfrom(int, void *, size_t, int, SA *, socklen_t *);
ssize_t  Recvmsg(int, struct msghdr *, int);
int      Select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
void     Send(int, const void *, size_t, int);
void     Sendto(int, const void *, size_t, int, const SA *, socklen_t);
void     Sendmsg(int, const struct msghdr *, int);
void     Setsockopt(int, int, int, const void *, socklen_t);
void     Shutdown(int, int);
int      Sockatmark(int);
int      Socket(int, int, int);
void     Socketpair(int, int, int, int *);
void     Writen(int, void *, size_t);

int		 Epoll_create();
int		 Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
#include <signal.h>
typedef void    Sigfunc(int);   /* for signal handlers */
Sigfunc *Signal(int, Sigfunc *);


#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

            /* prototypes for our Unix wrapper functions: see {Sec errors} */
void    *Calloc(size_t, size_t);
void     Close(int);
void     Dup2(int, int);
int      Fcntl(int, int, int);
void     Gettimeofday(struct timeval *, void *);
int      Ioctl(int, int, void *);
pid_t    Fork(void);
void    *Malloc(size_t);
int  Mkstemp(char *);
void    *Mmap(void *, size_t, int, int, int, off_t);
int      Open(const char *, int, mode_t);
void     Pipe(int *fds);
ssize_t  Read(int, void *, size_t);
void     Sigaddset(sigset_t *, int);
void     Sigdelset(sigset_t *, int);
void     Sigemptyset(sigset_t *);
void     Sigfillset(sigset_t *);
int      Sigismember(const sigset_t *, int);
void     Sigpending(sigset_t *);
void     Sigprocmask(int, const sigset_t *, sigset_t *);
char    *Strdup(const char *);
long     Sysconf(int);
void     Sysctl(int *, u_int, void *, size_t *, void *, size_t);
void     Unlink(const char *);
pid_t    Wait(int *);
pid_t    Waitpid(pid_t, int *, int);
void     Write(int, void *, size_t);

            /* prototypes for our stdio wrapper functions: see {Sec errors} */
void     Fclose(FILE *);
FILE    *Fdopen(int, const char *);
char    *Fgets(char *, int, FILE *);
FILE    *Fopen(const char *, const char *);
void     Fputs(const char *, FILE *);

//#define PNBX(fmt , ...)  err_quitx( __FILE__, __LINE__, __func__, fmt , ## __VA_ARGS__)
