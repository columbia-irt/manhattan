#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "sined.h"
#include "netmgr.h"
#include "conmgr.h"
#include "locmgr.h"
#include "socks.h"
#include "pe.h"

volatile int exit_flag = 0;
int exit_pipe[2] = {0};

int pids[PID_NUM] = {0};
const char *cmds[PID_NUM] = {
	CMD_MIHF,
	CMD_MIIS_SERVER,
	CMD_LINK_SAP,
	CMD_LINK_SAP,
	CMD_LINK_SAP,
	CMD_MIH_USR,
	CMD_HIP_DAEMON,
	CMD_MIPV6_DAEMON
};
char *args[PID_NUM][4] = {
	{ CMD_MIHF, "--conf.file="CONF_MIHF_PATH, NULL },
	{ CMD_MIIS_SERVER, "--database="CONF_MIIS_DATABASE, NULL },
	{ CMD_LINK_SAP, "--conf.file="CONF_LINK_ETH_PATH, NULL },
	{ CMD_LINK_SAP, "--conf.file="CONF_LINK_WIFI_PATH, NULL },
	{ CMD_LINK_SAP, "--conf.file="CONF_LINK_LTE_PATH, NULL },
	{ CMD_MIH_USR, "--conf.file="CONF_MIH_USR_PATH, CMD_MIH_USR_ARG, NULL },
	{ CMD_HIP_DAEMON, "-v", NULL },
	{ CMD_MIPV6_DAEMON, "-c", CONF_MIPV6_PATH, NULL }
};
const char *logs[PID_NUM] = {
	LOG_MIHF_PATH,
	LOG_MIIS_SERVER_PATH,
	LOG_LINK_ETH_PATH,
	LOG_LINK_WIFI_PATH,
	LOG_LINK_LTE_PATH,
	LOG_MIH_USR_PATH,
	LOG_HIP_PATH,
	LOG_MIPV6_PATH
};

void sine_exit()
{
	int i;

	log_info("cleaning up...");

	exit_flag = 1;
	//pthread_kill(pthread_netmgr, SIGUSR1);

	if (write(exit_pipe[1], "", 1) <= 0) {
		log_err("Failed to send exit signals, force exiting...");
		exit(1);
	}
	//close is enough for exiting signal
	//close(exit_pipe[1]);	/* write end */
	//close(exit_pipe[0]);	/* read end */

	for (i = PID_NUM - 1; i >= 0; --i) {
		if (pids[i]) {
			log_info("killing %s", cmds[i]);
			kill(pids[i], SIGINT);
			pids[i] = 0;
		}
	}
}

static void sig_handler(int signum)
{
	debug("signal %d received", signum);

	sine_exit();
}

int main()
{
	struct sigaction sa;
	pid_t pid;
	int i;

	pthread_t pthread_netmgr = pthread_self();
	pthread_t pthread_conmgr = pthread_self();
	pthread_t pthread_locmgr = pthread_self();
	pthread_t pthread_socks = pthread_self();
	pthread_t pthread_pe = pthread_self();

	for (i = 0; i < PID_NUM; ++i) {
		if (i == PID_MIH_USR)
			sleep(WAIT_TIME);

		pids[i] = fork();
		check(pids[i] >= 0, "fork for %s", cmds[i]);
		if (!pids[i]) {
			int fd = open("/dev/null", O_RDWR);
			if (fd < 0 || dup2(fd, STDIN_FILENO) < 0) {
				log_err("Redirect stdin for %s", cmds[i]);
				exit(1);
			}
			close(fd);
			//close(STDIN_FILENO);
			fd = open(logs[i], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
			if (fd < 0) {
				log_err("Open log file for %s", cmds[i]);
				exit(1);
			}
			if (dup2(fd, STDOUT_FILENO) < 0) {
				log_err("Redirect stdout for %s", cmds[i]);
				exit(1);
			}
			if (dup2(fd, STDERR_FILENO) < 0) {
				log_err("Redirect stderr for %s", cmds[i]);
				exit(1);
			}
			close(fd);

			execvp(cmds[i], args[i]);
			log_err("Failed to execute command %s", cmds[i]);
			exit(1);
		}
		log_info("executing %s (%d)", cmds[i], pids[i]);
	}

	//getchar();
	sleep(WAIT_TIME);

	check(!pipe(exit_pipe), "Failed to create exit_pipe");

	sa.sa_handler = sig_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP;

	check(!sigaction(SIGINT, &sa, NULL), "set handler for SIGINT");
	check(!sigaction(SIGQUIT, &sa, NULL), "set handler for SIGQUIT");
	check(!sigaction(SIGTERM, &sa, NULL), "set handler for SIGTERM");
	check(!sigaction(SIGSEGV, &sa, NULL), "set handler for SIGSEGV");
	//signal(SIGINT, sig_handler);
	//signal(SIGTERM, sig_handler);
	//signal(SIGSEGV, sig_handler);

	//openlog("sine", LOG_PID | LOG_PERROR, LOG_DAEMON);
	check(!init_network_manager(&pthread_netmgr),
		"Failed to initialize network manager");
	check(!init_connection_manager(&pthread_conmgr),
		"Failed to initialize connection manager");
	check(!init_location_manager(&pthread_locmgr),
		"Failed to initialize location manager");
	check(!init_socks_server(&pthread_socks),
		"Failed to initialize socks server");
	check(!init_policy_engine(&pthread_pe),
		"Failed to initialize policy engine");

	//syslog(LOG_INFO, "hello world!\n");
	log_info("Initialization succeeded!");

	//while (!exit_flag);

	/* terminate when any of the children has died */
	pid = wait(NULL);
	for (i = 0; i < PID_NUM; ++i) {
		if (pid == pids[i]) {
			log_err("%s has stopped working!", cmds[i]);
			break;
		}
	}

error:
	if (!exit_flag)
		sine_exit();

	if (!pthread_equal(pthread_netmgr, pthread_self()))
		pthread_join(pthread_netmgr, NULL);
	if (!pthread_equal(pthread_conmgr, pthread_self()))
		pthread_join(pthread_conmgr, NULL);
	if (!pthread_equal(pthread_locmgr, pthread_self()))
		pthread_join(pthread_locmgr, NULL);
	if (!pthread_equal(pthread_socks, pthread_self()))
		pthread_join(pthread_socks, NULL);
	if (!pthread_equal(pthread_pe, pthread_self()))
		pthread_join(pthread_pe, NULL);

	//closelog();
	close(exit_pipe[1]);	/* write end */
	close(exit_pipe[0]);	/* read end */

	return 0;
}
