/**------------------------------------------------------------------------------------------------
 * ?                                           ABOUT
 * @author         :  Nguyen Dinh Hoan
 * @email          :  hoann.wk@gmail.com
 * @repo           :  https://github.com/hoan9x/IPC-in-Linux
 * @createdOn      :  08-Oct-2024
 * @description    :  This example demonstrates a UNIX domain socket server handling multiple clients
 *                    using poll()
 *------------------------------------------------------------------------------------------------**/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>

/* LOG macro function */
#define LOG_INFO(format, ...) do { printf("[SERVER_INFO] " format "\n", ##__VA_ARGS__); } while (0)
#define LOG_ERROR(format, ...) do { printf("[SERVER_ERROR] " format "\n", ##__VA_ARGS__); } while (0)

#define DEFAULT_SOCKET_PATH "/tmp/ipc-demo.sock"
#define BUFFER_SIZE 128
#define MAX_NUMBER_PENDING_CONNECTIONS 10
#define MAX_CLIENT_SUPPORTED 32

#define IF_FAIL_THEN_EXIT(EXP, RELEASE, MSG, ...) ({ if (EXP) { LOG_ERROR(MSG, ##__VA_ARGS__); cleanupAndExitError(RELEASE); } })

/* An array of fd (file descriptors also called as data sockets) */
struct pollfd arrayFdSet[MAX_CLIENT_SUPPORTED];

/* Function to clean up resources and exit */
void cleanupAndExitError(const char *socketPath)
{
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
    {
        if (arrayFdSet[i].fd != -1)
        {
            close(arrayFdSet[i].fd);
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
        arrayFdSet[i].fd = -1;
    }
}

static void addToArrayFdSet(int fdNum, short events)
{
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
    {
        if (arrayFdSet[i].fd != -1)
            continue;
        arrayFdSet[i].fd = fdNum;
        arrayFdSet[i].events = events;
        break;
    }
}

static void removeFromArrayFdSet(int fdNum)
{
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
    {
        if (arrayFdSet[i].fd != fdNum)
            continue;
        arrayFdSet[i].fd = -1;
        break;
    }
}

int main(int argc, char *argv[])
{
    /* Initialize socket path from application input parameter or default value */
    const char *socketPath = (argc>1)?argv[1]:DEFAULT_SOCKET_PATH;
    struct sockaddr_un structSocketInfo;
    /* Initialize socket info structure */
    memset(&structSocketInfo, 0, sizeof(struct sockaddr_un));
    structSocketInfo.sun_family = AF_UNIX;
    strncpy(structSocketInfo.sun_path, socketPath, sizeof(structSocketInfo.sun_path)-1);
    int connSocket = -1, dataSocket = -1, ret, commSocketFd, i;
    char buffer[BUFFER_SIZE];
    struct pollfd fd2PollTmp = {0};

    initArrayFdSet();
    /* Add socket descriptor STDIN_FILENO=0 to monitor stdin */
    addToArrayFdSet(STDIN_FILENO, POLLIN);
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
    addToArrayFdSet(connSocket, POLLIN);
    /* Main server loop */
    for (;;)
    {
        LOG_INFO("##### Waiting on poll()");

        ret = poll(arrayFdSet, MAX_CLIENT_SUPPORTED, -1);
        IF_FAIL_THEN_EXIT(ret < 0, socketPath, "poll() return error");

        /* Check file descriptors with events */
        i = 0;
        for (; i < MAX_CLIENT_SUPPORTED; ++i)
        {
            if (arrayFdSet[i].revents & POLLIN)
            {
                if (connSocket==arrayFdSet[i].fd)
                {
                    LOG_INFO("New connection received, accepting the connection");
                    dataSocket = accept(connSocket, NULL, NULL);
                    IF_FAIL_THEN_EXIT(dataSocket < 0, socketPath, "accept() return error");
                    LOG_INFO("Connection established (%d)", dataSocket);
                    addToArrayFdSet(dataSocket, POLLIN);
                }
                else if (0==arrayFdSet[i].fd)
                {
                    /* Input from console stdin */
                    memset(buffer, 0, BUFFER_SIZE);
                    ret = read(0, buffer, BUFFER_SIZE);
                    LOG_INFO("Input read from stdin's fd[0]: [%.*s]", /*number read*/ret, buffer);
                }
                else
                {
                    /* Data arrives on the client's FD */
                    commSocketFd = arrayFdSet[i].fd;

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
            if (arrayFdSet[i].revents & POLLRDNORM)
            {
                LOG_INFO("fd[%d] return event: POLLRDNORM", arrayFdSet[i].fd);
            }
            if (arrayFdSet[i].revents & POLLRDBAND)
            {
                LOG_INFO("fd[%d] return event: POLLRDBAND", arrayFdSet[i].fd);
            }
            if (arrayFdSet[i].revents & POLLPRI)
            {
                LOG_INFO("fd[%d] return event: POLLPRI", arrayFdSet[i].fd);
            }
            if (arrayFdSet[i].revents & POLLOUT)
            {
                LOG_INFO("fd[%d] return event: POLLOUT", arrayFdSet[i].fd);
            }
            if (arrayFdSet[i].revents & POLLWRNORM)
            {
                LOG_INFO("fd[%d] return event: POLLWRNORM", arrayFdSet[i].fd);
            }
            if (arrayFdSet[i].revents & POLLWRBAND)
            {
                LOG_INFO("fd[%d] return event: POLLWRBAND", arrayFdSet[i].fd);
            }
            if (arrayFdSet[i].revents & POLLERR)
            {
                LOG_INFO("fd[%d] return event: POLLERR", arrayFdSet[i].fd);
            }
            if (arrayFdSet[i].revents & POLLHUP)
            {
                LOG_INFO("fd[%d] return event: POLLHUP", arrayFdSet[i].fd);
            }
            if (arrayFdSet[i].revents & POLLNVAL)
            {
                LOG_INFO("fd[%d] return event: POLLNVAL", arrayFdSet[i].fd);
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
