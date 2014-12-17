#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#include <stdarg.h>		/* ANSI C header file */
#include <syslog.h>		/* for syslog() */
#include <execinfo.h>


#define	MAXLINE	4096		/* max text line length */

int daemon_proc;		/* set nonzero by daemon_init() */

static void err_doit(int, int, const char *, va_list);
static void print_func_stack();

/* Nonfatal error related to system call
 * Print message and return */

		void
err_ret(const char *fmt, ...)
{
		va_list		ap;

		va_start(ap, fmt);
		err_doit(1, LOG_INFO, fmt, ap);
		va_end(ap);
		return;
}

/* Fatal error related to system call
 * Print message and terminate */

		void
err_sys(const char *fmt, ...)
{
		va_list		ap;

		va_start(ap, fmt);
		err_doit(1, LOG_ERR, fmt, ap);
		va_end(ap);
//		print_func_stack();
		exit(1);
}

/* Fatal error related to system call
 * Print message, dump core, and terminate */

		void
err_dump(const char *fmt, ...)
{
		va_list		ap;

		va_start(ap, fmt);
		err_doit(1, LOG_ERR, fmt, ap);
		va_end(ap);
		abort();		/* dump core and terminate */
		exit(1);		/* shouldn't get here */
}

/* Nonfatal error unrelated to system call
 * Print message and return */

		void
err_msg(const char *fmt, ...)
{
		va_list		ap;

		va_start(ap, fmt);
		err_doit(0, LOG_INFO, fmt, ap);
		va_end(ap);
		return;
}



 void  err_quitx(const char* file , int line , const char * func , const char *fmt, ...)
 {
         va_list     ap;
	 char buf[1024];

         va_start(ap, fmt);
	 vsnprintf(buf, 1023, fmt, ap);   /* safe */
	 printf(buf);
	 printf("\t--file : %s , %s() , line : %d\n",file , func , line);
	 fflush(stdout);

         va_end(ap);
 }


/* Fatal error unrelated to system call
 * Print message and terminate */

		void
err_quit(const char *fmt, ...)
{
		va_list		ap;

		va_start(ap, fmt);
		err_doit(0, LOG_ERR, fmt, ap);
		va_end(ap);
		print_func_stack();
		exit(1);
}


/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */

		static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
		int	errno_save, n;
		char	buf[MAXLINE + 1];

		errno_save = errno;		/* value caller might want printed */
#ifdef	HAVE_VSNPRINTF
		vsnprintf(buf, MAXLINE, fmt, ap);	/* safe */
#else
		vsprintf(buf, fmt, ap);			/* not safe */
#endif
		n = strlen(buf);
		if (errnoflag)
				snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
		strcat(buf, "\n");

		if (daemon_proc) {
				syslog(level,"%s",buf);
		} else {
				fflush(stdout);		/* in case stdout and stderr are the same */
				fputs(buf, stderr);
				fflush(stderr);
		}
		return;
}

static void print_func_stack()
{
		int j, nptrs;
#define SIZE 100
		void *buffer[100];
		char **strings;

		nptrs = backtrace(buffer, SIZE);
		printf("backtrace() returned %d addresses\n", nptrs);

		/* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
		   would produce similar output to the following: */

		strings = backtrace_symbols(buffer, nptrs);
		if (strings == NULL)
		{
				perror("backtrace_symbols");
				exit(-1);//EXIT_FAILURE);
		}

		for (j = 0; j < nptrs; j++)
				printf("%s\n", strings[j]);

		free(strings);
}
