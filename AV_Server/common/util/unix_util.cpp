#include "unix_util.h"

#if defined(__linux__)
# if !defined(HAVE_PR_SET_NAME)
#  include <linux/version.h>
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
#include <sys/prctl.h>
#define HAVE_PR_SET_NAME 1
#endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
#else
#endif // !defined(HAVE_PR_SET_NAME)
#endif // defined(linux)

#define NGX_SETPROCTITLE_USES_ENV  1
#define NGX_SETPROCTITLE_PAD      '\0'

extern char **environ;

namespace Util
{

void InstallSignal(mts_sig_handler pCallback)
{
    signal(SIGHUP  ,SIG_IGN );   /* hangup, generated when terminal disconnects */
    signal(SIGINT  ,SIG_IGN );   /* interrupt, generated from terminal special char */
    signal(SIGQUIT ,SIG_IGN );   /* (*) quit, generated from terminal special char */
    signal(SIGILL  ,SIG_IGN );   /* (*) illegal instruction (not reset when caught)*/
    signal(SIGTRAP ,SIG_IGN );   /* (*) trace trap (not reset when caught) */
    signal(SIGABRT ,SIG_IGN );   /* (*) abort process */
#ifdef D_AIX
    signal(SIGEMT  ,SIG_IGN );   /* EMT intruction */
#endif
    signal(SIGFPE  ,SIG_IGN );   /* (*) floating point exception */
    signal(SIGKILL ,SIG_IGN );   /* kill (cannot be caught or ignored) */
    signal(SIGBUS  ,SIG_IGN );   /* (*) bus error (specification exception) */
    signal(SIGSEGV ,SIG_IGN );   /* (*) segmentation violation */
    signal(SIGSYS  ,SIG_IGN );   /* (*) bad argument to system call */
    signal(SIGPIPE ,SIG_IGN );   /* write on a pipe with no one to read it */
    signal(SIGALRM ,SIG_IGN );   /* alarm clock timeout */
    signal(SIGTERM ,pCallback);  /* software termination signal */
    signal(SIGURG  ,SIG_IGN );   /* (+) urgent contition on I/O channel */
    signal(SIGSTOP ,SIG_IGN );   /* (@) stop (cannot be caught or ignored) */
    signal(SIGTSTP ,SIG_IGN );   /* (@) interactive stop */
    signal(SIGCONT ,SIG_IGN );   /* (!) continue (cannot be caught or ignored) */
    signal(SIGCHLD ,SIG_IGN);    /* (+) sent to parent on child stop or exit */
    signal(SIGTTIN ,SIG_IGN);    /* (@) background read attempted from control terminal*/
    signal(SIGTTOU ,SIG_IGN);    /* (@) background write attempted to control terminal */
    signal(SIGIO   ,SIG_IGN);    /* (+) I/O possible, or completed */
    signal(SIGXCPU ,SIG_IGN);    /* cpu time limit exceeded (see setrlimit()) */
    signal(SIGXFSZ ,SIG_IGN);    /* file size limit exceeded (see setrlimit()) */

#ifdef D_AIX
    signal(SIGMSG  ,SIG_IGN);    /* input data is in the ring buffer */
#endif

    signal(SIGWINCH,SIG_IGN);    /* (+) window size changed */
    signal(SIGPWR  ,SIG_IGN);    /* (+) power-fail restart */
    signal(SIGUSR1 ,SIG_IGN);   /* user defined signal 1 */
    signal(SIGUSR2 ,SIG_IGN);   /* user defined signal 2 */
    signal(SIGPROF ,SIG_IGN);    /* profiling time alarm (see setitimer) */

#ifdef D_AIX
    signal(SIGDANGER,SIG_IGN);   /* system crash imminent; free up some page space */
#endif

    signal(SIGVTALRM,SIG_IGN);   /* virtual time alarm (see setitimer) */

#ifdef D_AIX
    signal(SIGMIGRATE,SIG_IGN);  /* migrate process */
    signal(SIGPRE  ,SIG_IGN);    /* programming exception */
    signal(SIGVIRT ,SIG_IGN);    /* AIX virtual time alarm */
    signal(SIGALRM1,SIG_IGN);    /* m:n condition variables - RESERVED - DON'T USE */
    signal(SIGWAITING,SIG_IGN);  /* m:n scheduling - RESERVED - DON'T USE */
    signal(SIGCPUFAIL ,SIG_IGN); /* Predictive De-configuration of Processors - */
    signal(SIGKAP,SIG_IGN);      /* keep alive poll from native keyboard */
    signal(SIGRETRACT,SIG_IGN);  /* monitor mode should be relinguished */
    signal(SIGSOUND  ,SIG_IGN);  /* sound control has completed */
    signal(SIGSAK    ,SIG_IGN);  /* secure attention key */
#endif
}

int SetCoreMax()
{
    struct rlimit rlim_new;
    struct rlimit rlim;
    /*
     *  First try raising to infinity; if that fails, try bringing
     *  the soft limit to the hard.
    */
    if (getrlimit(RLIMIT_CORE, &rlim) == 0)
    {
        rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_CORE, &rlim_new)!= 0) 
        {
            /* failed. try raising just to the old max */
            rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
            (void)setrlimit(RLIMIT_CORE, &rlim_new);
        }
    }
	/*
	 * getrlimit again to see what we ended up with. Only fail if
	 * the soft limit ends up 0, because then no core files will be
	 * created at all.
	 */
	 if ((getrlimit(RLIMIT_CORE, &rlim) != 0) || rlim.rlim_cur == 0)
     {
        fprintf(stderr, "failed to ensure corefile creation\n");
        return -1;
     }
     return 0;
}

void daemonize(const char* pCmd, int nochdir, int noclose)
{
    struct sigaction sa;
    pid_t pid;
    SetCoreMax(); //set dump core max
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
        fprintf(stderr, "Failed to ignore SIGHUP");
        exit(EXIT_FAILURE);
    }

    if ((pid = fork()) < 0)
    {
        fprintf(stderr,"fork() fail");
        exit(EXIT_FAILURE);
    }
    else if (pid != 0)
    {
        exit(EXIT_SUCCESS);
    }
    if (setsid() == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (nochdir == 0) 
    {
        if (chdir("/") != 0)
        {
            perror("chdir");
            exit(EXIT_FAILURE);
        }
    }

    int fd = 0;
    if (noclose == 0 && (fd = open("/dev/null", O_RDWR, 0)) != -1)
    {
        if (dup2(fd, STDIN_FILENO) < 0) 
        {
            perror("dup2 stdin");
            exit(EXIT_FAILURE);
        }
        if(dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 stdout");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDERR_FILENO) < 0)
        {
            perror("dump2 stderr");
            exit(EXIT_FAILURE);
        }
        if (fd > STDERR_FILENO)
        {
            if (close(fd) < 0)
            {
                perror("close");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int SetOpenFileNums(const int iMaxOpenFilesNums)
{
	if (iMaxOpenFilesNums <= 3)
	{
		return -1;
	}

	struct rlimit rlim;
	int iret = getrlimit(RLIMIT_NOFILE, &rlim);
	if (iret != 0)
	{
		return (errno);
	}

	rlim.rlim_cur = iMaxOpenFilesNums;
	rlim.rlim_max = iMaxOpenFilesNums;
	iret = setrlimit(RLIMIT_NOFILE, &rlim);
	if (iret != 0)
	{
		return (errno);
	}
	return 0;
}

ProcTitle::ProcTitle(int argc, char** argv)
    : m_pOsArgvLast(NULL), m_iArgBufSize(0), 
    m_pArgv( argv ), m_bInit (false), m_pEnv(NULL)
{
    if (m_bInit == false)
    {
        m_bInit = this->Init();
    }
}
ProcTitle::~ProcTitle()
{
    if (m_pEnv)
    {
        //free(m_pEnv);
        m_pEnv = NULL;
    }

}
bool  ProcTitle::Init()
{
	//char *p;
	size_t size;
	int i;

	size = 0;
	for (i = 0; environ[i]; i++)
	{
		size += strlen(environ[i]) + 1;
	}
	m_iArgBufSize = size;

	m_pEnv = (char*) malloc(size);
	if (m_pEnv == NULL)
	{
		return false;
	}

	m_pOsArgvLast = m_pArgv[0];

	for (i = 0; m_pArgv[i]; i++)
	{
		if (m_pOsArgvLast == m_pArgv[i])
		{
			m_pOsArgvLast = m_pArgv[i] + strlen(m_pArgv[i]) + 1;
		}
        m_iArgBufSize += (strlen(m_pArgv[i]) + 1);
	}
	m_iArgBufSize --;

	for (i = 0; environ[i]; i++)
	{
		if (m_pOsArgvLast == environ[i])
		{
			size = strlen(environ[i]) + 1;
			m_pOsArgvLast = environ[i] + size;
			snprintf(m_pEnv, size, "%s", environ[i]);
			environ[i] = (char *) m_pEnv;
			m_pEnv += size;
		}
	}

	m_pOsArgvLast--;
	return true;
}

void ProcTitle::SetTitle(const char* pTitle)
{
    if (m_bInit == false || pTitle == NULL)
    {
        return ;
    }
    m_pArgv[1] = NULL;
	int new_title_len = strlen(pTitle);
	if (new_title_len < m_iArgBufSize)
	{
	    memset(m_pArgv[0], 0, new_title_len+1);
		sprintf(m_pArgv[0], "%s", pTitle);
	}
	else
	{
		memcpy(m_pArgv[0], pTitle, m_iArgBufSize);
		m_pArgv[0][m_iArgBufSize] = 0;
	}

#ifdef HAVE_PR_SET_NAME
	prctl(PR_SET_NAME, pTitle, 0, 0, 0);
#endif
}

const char* ProcTitle::GetTitle()
{
    if (m_bInit == false)
    {
        return NULL;
    }
    static char proctitle[2048];
	char linkname[64]; /* /proc/<pid>/exe */
	pid_t pid;

	/* Get our PID and build the name of the link in /proc */
	pid = getpid();

	if (snprintf(linkname, sizeof(linkname), "/proc/%i/cmdline", pid) < 0)
	{
		abort();
	}

	int cmdline_fd = open(linkname, O_RDONLY);
	if(cmdline_fd < 0)
	{
		return NULL;
	}

	size_t ret = read(cmdline_fd, proctitle, sizeof(proctitle)-1);

	/* Report insufficient buffer size */
	if (ret == (sizeof(proctitle)-1))
	{
		errno = ERANGE;
		return NULL;
	}

	/* Ensure proper NUL termination */
	proctitle[ret] = 0;
	close(cmdline_fd);
	return proctitle;
}

}
