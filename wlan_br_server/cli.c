#define _GNU_SOURCE
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <ctype.h>
#include "libcli.h"
#include "cli.h"
#include "common.h"
#include "wlan_br_forward.h"
#include "statistics.h"
#define CLITEST_PORT 8000
#define MODE_CONFIG_INT 10
#ifdef __GNUC__
#define UNUSED(d) d __attribute__((unused))
#else
#define UNUSED(d) d
#endif

unsigned int regular_count = 0;
unsigned int debug_regular = 0;

static int cli_sock = -1;
extern epoll_stat_info_t epoll_stat_info;
int cmd_test(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    int i;
    cli_print(cli, "called %s with \"%s\"", __func__, command);
    cli_print(cli, "%d arguments:", argc);
    for (i = 0; i < argc; i++)
        cli_print(cli, "        %s", argv[i]);

    return CLI_OK;
}

int cmd_set(struct cli_def *cli, UNUSED(const char *command), char *argv[], int argc)
{
    if (argc < 2 || strcmp(argv[0], "?") == 0)
    {
        cli_print(cli, "Specify a variable to set");
        return CLI_OK;
    }

    if (strcmp(argv[1], "?") == 0)
    {
        cli_print(cli, "Specify a value");
        return CLI_OK;
    }

    if (strcmp(argv[0], "regular_interval") == 0)
    {
        unsigned int sec = 0;
        if (!argv[1] && !*argv[1])
        {
            cli_print(cli, "Specify a regular callback interval in seconds");
            return CLI_OK;
        }
        sscanf(argv[1], "%u", &sec);
        if (sec < 1)
        {
            cli_print(cli, "Specify a regular callback interval in seconds");
            return CLI_OK;
        }
        cli->timeout_tm.tv_sec = sec;
        cli->timeout_tm.tv_usec = 0;
        cli_print(cli, "Regular callback interval is now %d seconds", sec);
        return CLI_OK;
    }

    cli_print(cli, "Setting \"%s\" to \"%s\"", argv[0], argv[1]);
    return CLI_OK;
}

int cmd_config_int_exit(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc))
{
    cli_set_configmode(cli, MODE_CONFIG, NULL);
    return CLI_OK;
}

int check_auth(const char *username, const char *password)
{
    if (strcasecmp(username, "admin") != 0)
        return CLI_ERROR;
    if (strcasecmp(password, "admin") != 0)
        return CLI_ERROR;
    return CLI_OK;
}

int regular_callback(struct cli_def *cli)
{
    regular_count++;
    if (debug_regular)
    {
        cli_print(cli, "Regular callback - %u times so far", regular_count);
        cli_reprompt(cli);
    }
    return CLI_OK;
}

int check_enable(const char *password)
{
    return !strcasecmp(password, "admin");
}

int idle_timeout(struct cli_def *cli)
{
    cli_print(cli, "Custom idle timeout");
    return CLI_QUIT;
}

int cmd_show_epoll_stat(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    cli_print(cli, "epoll recv: %-15llu timeout: %-15llu error: %llu",
              epoll_stat_info.epool_recv, epoll_stat_info.epoll_timeout, epoll_stat_info.epoll_error);
    return CLI_OK;
}

int cmd_clean_epoll_stat(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    memset(&epoll_stat_info, 0, sizeof(epoll_stat_info));
    cli_print(cli, "clean success.");
    return CLI_OK;
}

static int is_number(const char *str)
{
    while (*str)
    {
        if (!isdigit((unsigned char)*str))
        {
            return 0;
        }
        str++;
    }
    return 1;
}

int cmd_set_log_level(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    memset(&epoll_stat_info, 0, sizeof(epoll_stat_info));
    if (argc < 1 || !argv[0] || !is_number(argv[0]))
    {
        cli_print(cli, "[Error] please input log level.(1: info, 2: debug)");
        return CLI_ERROR;
    }
    log_level = atoi(argv[0]);
    cli_print(cli, " set log level %d success.", log_level);
    return CLI_OK;
}

static void cli_print_fn(void *handle, const char *str)
{
    cli_print((struct cli_def *)handle, "%s", str);
}

int cmd_show_fdb_list(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    wlan_br_dump_fdb((void *)cli, cli_print_fn);
    return CLI_OK;
}

int cmd_show_ip_stat(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    wlan_ip_dump_static((void *)cli, cli_print_fn);
    return CLI_OK;
}

void run_child(int x)
{
    struct cli_command *show_c;
    struct cli_def *cli;

    cli = cli_init();
    cli_set_banner(cli, "-------wlan bridge server environment-------");
    cli_set_hostname(cli, "cli");
    cli_telnet_protocol(cli, 1);
    cli_regular(cli, regular_callback);

    // change regular update to 5 seconds rather than default of 1 second
    cli_regular_interval(cli, 5);

    // set 1800 second idle timeout
    cli_set_idle_timeout_callback(cli, 1800, idle_timeout);
    cli_register_command(cli, NULL, "test", cmd_test, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);

    show_c = cli_register_command(cli, NULL, "show", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, show_c, "epoll_stat", cmd_show_epoll_stat, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                         "Show epoll statistics");
    cli_register_command(cli, show_c, "fdb", cmd_show_fdb_list, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                         "Show fdb list");
    cli_register_command(cli, show_c, "ip_stat", cmd_show_ip_stat, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                         "Show ip statistics");

    cli_register_command(cli, show_c, "counters", cmd_test, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                         "Show the counters that the system uses");

    struct cli_command *set_cli;
    set_cli = cli_register_command(cli, NULL, "set", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, set_cli, "epoll_stat_clean", cmd_clean_epoll_stat, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                         "clean epoll statistics");
    cli_register_command(cli, set_cli, "log_level", cmd_set_log_level, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                         "set log level 1: info, 2: debug");
    cli_register_command(cli, set_cli, "counters", cmd_set, PRIVILEGE_UNPRIVILEGED, MODE_EXEC,
                         "set the counters that the system uses");

    cli_register_command(cli, NULL, "exit", cmd_config_int_exit, PRIVILEGE_PRIVILEGED, MODE_CONFIG_INT,
                         "Exit from interface configuration");

    cli_set_auth_callback(cli, check_auth);
    cli_set_enable_callback(cli, check_enable);

    cli_loop(cli, x);
    cli_done(cli);
}

static void *cli_client_thread(void *arg)
{
    int connect_fd = *(int *)arg;
    char thread_name[64] = {0};
    sprintf(thread_name, "cli_client%d", connect_fd);
    pthread_setname_np(pthread_self(), thread_name);
    run_child(connect_fd);
    close(connect_fd);
    return NULL;
}

static void cli_create_client_thread(int connect_fd, struct sockaddr_in *client)
{
    pthread_t thread;
    int ret = pthread_create(&thread, NULL, cli_client_thread, (void *)&connect_fd);
    if (ret)
    {
        LOG_ERR("pthread_create error!");
        return;
    }
    printf("client: %s:%d connecting...", inet_ntoa((struct in_addr)(client)->sin_addr), ntohs((client)->sin_port));
}

static void *cli_main_thread(void *arg)
{
    pthread_setname_np(pthread_self(), "cli");
    if ((cli_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return NULL;
    }
    int on = 1;
    struct sockaddr_in addr;
    if (setsockopt(cli_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
    {
        perror("setsockopt");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(CLITEST_PORT);
    if (bind(cli_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return NULL;
    }

    if (listen(cli_sock, 50) < 0)
    {
        perror("listen");
        return NULL;
    }

    while (1)
    {
        struct sockaddr_in client_addr = {0};
        socklen_t len = sizeof(struct sockaddr_in);
        int connect_fd = accept(cli_sock, (struct sockaddr *)&client_addr, &len);
        if (connect_fd <= 0)
        {
            LOG_ERR("accept error, errno: %d", errno);
            return NULL;
        }

        cli_create_client_thread(connect_fd, &client_addr);
    }
    return NULL;
}

void cli_main(void)
{
    pthread_t thread;
    int ret = pthread_create(&thread, NULL, cli_main_thread, NULL);
    if (ret)
    {
        LOG_ERR("pthread_create error!");
        return;
    }
}