/**------------------------------------------------------------------------------------------------
 * ?                                           ABOUT
 * @author         :  Nguyen Dinh Hoan
 * @email          :  hoann.wk@gmail.com
 * @repo           :  https://github.com/hoan9x/IPC-in-Linux
 * @createdOn      :  08-Oct-2024
 * @description    :  This example demonstrates a UNIX domain socket server handling multiple clients
 *                    using pselect()
 *------------------------------------------------------------------------------------------------**/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

/* LOG macro function */
#define LOG_INFO(format, ...) do { printf("[SERVER_INFO] " format "\n", ##__VA_ARGS__); } while (0)
#define LOG_ERROR(format, ...) do { printf("[SERVER_ERROR] " format "\n", ##__VA_ARGS__); } while (0)

#define DEFAULT_SOCKET_PATH "/tmp/ipc-demo.sock"
#define BUFFER_SIZE 128
#define MAX_NUMBER_PENDING_CONNECTIONS 10
#define MAX_CLIENT_SUPPORTED 32

#define IF_FAIL_THEN_EXIT(EXP, RELEASE, MSG, ...) ({ if (EXP) { LOG_ERROR(MSG, ##__VA_ARGS__); cleanupAndExitError(RELEASE); } })

/* Flag to enable/disable pselect() use case on timeout */ 
#define USE_CASE_PSELECT_TIMEOUT 1

/* An array of fd (file descriptors also called as data sockets) */
int arrayFdSet[MAX_CLIENT_SUPPORTED];

/* Signal handler function */
volatile sig_atomic_t isSignalReceived = false;
volatile sig_atomic_t signalNumber = 0;
void handleSignal(const int sigNum)
{
    isSignalReceived = true;
    signalNumber = sigNum;
    switch (sigNum)
    {
        case SIGINT:
            LOG_INFO("Signal SIGINT [%i] (Ctrl+C) received", sigNum);
            break;
        case SIGTERM:
            LOG_INFO("Signal SIGTERM [%i] (default `kill` or `killall`) received", sigNum);
            break;
        case SIGTSTP:
            LOG_INFO("Signal SIGTSTP [%i] (Ctrl+Z) received", sigNum);
            break;
        default:
            LOG_INFO("Signal [%i] received", sigNum);
            break;
    }
}

/* Function to clean up resources and exit */
void cleanupAndExitError(const char *socketPath)
{
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
    {
        if (arrayFdSet[i] != -1)
        {
            close(arrayFdSet[i]);
        }
    }
    if (socketPath)
    {
        /* Remove the socket file */
        unlink(socketPath);
    }

    exit(EXIT_FAILURE);
}

static void initArrayFdSet()
{
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
    {
        arrayFdSet[i] = -1;
    }
}

static void addToArrayFdSet(int fdNum)
{
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
    {
        if (arrayFdSet[i] != -1)
            continue;
        arrayFdSet[i] = fdNum;
        break;
    }
}

static void removeFromArrayFdSet(int fdNum)
{
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
    {
        if (arrayFdSet[i] != fdNum)
            continue;
        arrayFdSet[i] = -1;
        break;
    }
}

/* Clone the arrayFdSet to fd_set data structure */
static void cloneFdSetStruct(fd_set *ptrFdSet)
{
    FD_ZERO(ptrFdSet);
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
    {
        if (arrayFdSet[i] != -1)
        {
            FD_SET(arrayFdSet[i], ptrFdSet);
        }
    }
}

static int getMaxFromArrayFdSet()
{
    int i = 0;
    int max = -1;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
    {
        if (arrayFdSet[i] > max)
            max = arrayFdSet[i];
    }
    return max;
}

int main(int argc, char *argv[])
{
    sigset_t sigList;
    /* Prepare a list of block all signals */
    sigfillset(&sigList);
    /* Allowing only specific fllowing signals */
    sigdelset(&sigList, SIGINT);
    sigdelset(&sigList, SIGTSTP);
    sigdelset(&sigList, SIGQUIT);
    sigdelset(&sigList, SIGTERM);
    /* Apply list signal to block for this process */
    sigprocmask(SIG_BLOCK, &sigList, NULL);

    struct sigaction sa;
    sa.sa_handler = handleSignal;
    /* Restart interrupted system calls */
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    /* Register signal handler for SIGINT (Ctrl+C) */
    sigaction(SIGINT, &sa, NULL);
    /* Register signal handler for SIGTSTP (Ctrl+Z) */
    sigaction(SIGTSTP, &sa, NULL);
    /* Register signal handler for SIGTSTP (Ctrl+\) */
    sigaction(SIGQUIT, &sa, NULL);
    /* Register signal handler for SIGTERM */
    sigaction(SIGTERM, &sa, NULL);
    LOG_INFO("Press Ctrl+C to send SIGINT, Ctrl+Z to send SIGTSTP, Ctrl+\\ to send SIGQUIT, `kill -SIGTERM <pid>` to send SIGTERM");

    /* Initialize socket path from application input parameter or default value */
    const char *socketPath = (argc>1)?argv[1]:DEFAULT_SOCKET_PATH;
    struct sockaddr_un structSocketInfo;
    /* Initialize socket info structure */
    memset(&structSocketInfo, 0, sizeof(struct sockaddr_un));
    structSocketInfo.sun_family = AF_UNIX;
    strncpy(structSocketInfo.sun_path, socketPath, sizeof(structSocketInfo.sun_path)-1);
    fd_set rfds; /* read fds */
    /* Initialize sigmask to pselect() */
    sigset_t sigmask2Set;
    /**----------------------------------------------
     * ### To block a specific signal, use 'sigemptyset()' and 'sigaddset()', for example:
     * sigemptyset(&sigmask2Set);           Init an empty singal set, it means unblock all signals
     * sigaddset(&sigmask2Set, SIGINT);     Block SIGINT
     * sigaddset(&sigmask2Set, SIGTERM);    Block SIGTERM
     * ### To block all signals except some specific signals, use 'sigfillset()' and 'sigdelset()', for example:
     * sigfillset(&sigmask2Set);            Block all signals
     * sigdelset(&sigmask2Set, SIGINT);     Unblock SIGINT
     * sigdelset(&sigmask2Set, SIGTERM);    Unblock SIGTERM
     *---------------------------------------------**/
    sigfillset(&sigmask2Set);
    sigdelset(&sigmask2Set, SIGINT);
    sigdelset(&sigmask2Set, SIGTSTP);
    sigdelset(&sigmask2Set, SIGQUIT);
    sigdelset(&sigmask2Set, SIGTERM);
#if (USE_CASE_PSELECT_TIMEOUT)
    struct timespec ts2Set;
    ts2Set.tv_sec = 3;
    ts2Set.tv_nsec = 0;
#endif
    int connSocket = -1, dataSocket = -1, ret, commSocketFd, i;
    char buffer[BUFFER_SIZE];

    initArrayFdSet();
    /* Add socket descriptor STDIN_FILENO=0 to monitor stdin */
    addToArrayFdSet(STDIN_FILENO);
    /* Remove the socket if it exists */
    unlink(socketPath);

    /* Create connection socket (master socket file descriptor) */
    connSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    IF_FAIL_THEN_EXIT(connSocket < 0, socketPath, "Creating a connection socket failed");
    LOG_INFO("Connection socket created (%d)", connSocket);

    /* Bind connection socket to path */
    ret = bind(connSocket, (const struct sockaddr *)&structSocketInfo, sizeof(struct sockaddr_un));
    IF_FAIL_THEN_EXIT(ret < 0, socketPath, "Bind connection socket to path failed");
    LOG_INFO("Bind connection socket to path [%s] succeeded", socketPath);

    /**
     * Listen for incoming connections, the second parameter means that while a request is being processed,
     * MAX_NUMBER_PENDING_CONNECTIONS requests can wait.
     **/
    ret = listen(connSocket, MAX_NUMBER_PENDING_CONNECTIONS);
    IF_FAIL_THEN_EXIT(ret < 0, socketPath, "Listening on socket failed");
    LOG_INFO("Listening for incoming connections...");

    /* Add connection socket to array set of FDs */
    addToArrayFdSet(connSocket);
    /* Main server loop */
    for (;;)
    {
        cloneFdSetStruct(&rfds);
        LOG_INFO("##### Waiting on pselect()");

#if (USE_CASE_PSELECT_TIMEOUT)
        /* Call pselect(), the server will block until there is a connection or data request or timeout or signal received */
        ret = pselect(getMaxFromArrayFdSet()+1, &rfds, NULL, NULL, /*timeout*/&ts2Set, &sigmask2Set);
#else
        /* Call pselect(), the server will block until there is a connection or data request on any FDs or signal received */
        ret = pselect(getMaxFromArrayFdSet()+1, &rfds, NULL, NULL, NULL, &sigmask2Set);
#endif
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                LOG_ERROR("pselect() return interrupted system call");
                if (isSignalReceived)
                {
                    isSignalReceived = false;
                    if (SIGINT==signalNumber)
                    {
                        LOG_INFO("Shutdown due to signal [%d]", signalNumber);
                        break;
                    }
                    else continue;
                }
            }
            else
            {
                LOG_ERROR("pselect() return error");
                cleanupAndExitError(socketPath);
            }
        }
#if (USE_CASE_PSELECT_TIMEOUT)
        else if (0 == ret)
        {
            LOG_INFO("pselect() timeout and no data within %ld(s) and %ld(ns)", ts2Set.tv_sec, ts2Set.tv_nsec);
            continue;
        }
#endif

        if (FD_ISSET(connSocket, &rfds))
        {
            LOG_INFO("New connection received, accepting the connection");
            dataSocket = accept(connSocket, NULL, NULL);
            IF_FAIL_THEN_EXIT(dataSocket < 0, socketPath, "accept() return error");
            LOG_INFO("Connection established (%d)", dataSocket);
            addToArrayFdSet(dataSocket);
        }
        else if (FD_ISSET(0, &rfds))
        {
            /* Input from console stdin */
            memset(buffer, 0, BUFFER_SIZE);
            ret = read(0, buffer, BUFFER_SIZE);
            LOG_INFO("Input read from stdin's fd[0]: [%.*s]", /*number read*/ret, buffer);
        }
        else
        {
            /* Data arrives on one of the client's FDs, so check and get that FD */
            i = 0; commSocketFd = -1;
            for (; i < MAX_CLIENT_SUPPORTED; ++i)
            {
                if (FD_ISSET(arrayFdSet[i], &rfds))
                {
                    commSocketFd = arrayFdSet[i];
                    break;
                }
            }

            /* Prepare the buffer to receive the data */
            memset(buffer, 0, BUFFER_SIZE);
            LOG_INFO("Waiting for data from the client's fd[%d] using read()", commSocketFd);
            ret = read(commSocketFd, buffer, BUFFER_SIZE);
            if (-1 == ret)
            {
                LOG_ERROR("read() fd[%d] return error", commSocketFd);
                cleanupAndExitError(socketPath);
            }
            else if (/*EOF*/0 == ret)
            {
                /* Once the client has closed the socket, the server will received the EOF message */
                LOG_INFO("Received EOF message");
                removeFromArrayFdSet(commSocketFd);
                close(commSocketFd);
            }
            else
            {
                LOG_INFO("Received data from fd[%d]: [%.*s]", commSocketFd, /*number read*/ret, buffer);
            }
        }
    }

    /* Perform clean up */
    close(connSocket);
    removeFromArrayFdSet(connSocket);
    unlink(socketPath);
    LOG_INFO("Server is down");

    return EXIT_SUCCESS;
}
