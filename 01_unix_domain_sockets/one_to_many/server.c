/**------------------------------------------------------------------------------------------------
 * ?                                           ABOUT
 * @author         :  Nguyen Dinh Hoan
 * @email          :  hoann.wk@gmail.com
 * @repo           :  https://github.com/hoan9x/IPC-in-Linux
 * @createdOn      :  08-Oct-2024
 * @description    :  This example demonstrates a UNIX domain socket server handling multiple clients
 *                    using select()
 *------------------------------------------------------------------------------------------------**/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>

/* LOG macro function */
#define LOG_INFO(format, ...) do { printf("[SERVER_INFO] " format "\n", ##__VA_ARGS__); } while (0)
#define LOG_ERROR(format, ...) do { printf("[SERVER_ERROR] " format "\n", ##__VA_ARGS__); } while (0)

#define DEFAULT_SOCKET_PATH "/tmp/ipc-demo.sock"
#define BUFFER_SIZE 128
#define MAX_NUMBER_PENDING_CONNECTIONS 10
#define MAX_CLIENT_SUPPORTED 32

#define IF_FAIL_THEN_EXIT(EXP, RELEASE, MSG, ...) ({ if (EXP) { LOG_ERROR(MSG, ##__VA_ARGS__); cleanupAndExitError(RELEASE); } })

/* Flag to enable/disable select() use case on timeout */ 
#define USE_CASE_SELECT_TIMEOUT 1

/* An array of fd (file descriptors also called as data sockets) */
int arrayFdSet[MAX_CLIENT_SUPPORTED];

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

static void addToArrayFdSet(int skt_fd)
{
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
    {
        if (arrayFdSet[i] != -1)
            continue;
        arrayFdSet[i] = skt_fd;
        break;
    }
}

static void removeFromArrayFdSet(int skt_fd)
{
    int i = 0;
    for (; i < MAX_CLIENT_SUPPORTED; i++)
    {
        if (arrayFdSet[i] != skt_fd)
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
    /* Initialize socket path from application input parameter or default value */
    const char *socketPath = (argc>1)?argv[1]:DEFAULT_SOCKET_PATH;
    struct sockaddr_un structSocketInfo;
    /* Initialize socket info structure */
    memset(&structSocketInfo, 0, sizeof(struct sockaddr_un));
    structSocketInfo.sun_family = AF_UNIX;
    strncpy(structSocketInfo.sun_path, socketPath, sizeof(structSocketInfo.sun_path)-1);
    fd_set rfds; /* read fds */
#if (USE_CASE_SELECT_TIMEOUT)
    struct timeval tv2Set, tv2Print;
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
        LOG_INFO("##### Waiting on select()");

#if (USE_CASE_SELECT_TIMEOUT)
        /**
         * select() may update the timeout argument to indicate how much time was left,
         * so you must re-init it before call select()
         **/
        tv2Set.tv_sec = 5;
        tv2Set.tv_usec = 0;
        tv2Print = tv2Set;
        /* Call select(), the server will block until there is a connection or data request or timeout */
        ret = select(getMaxFromArrayFdSet()+1, &rfds, NULL, NULL, /*timeout*/&tv2Set);
#else
        /* Call select(), the server will block until there is a connection or data request on any FDs */
        ret = select(getMaxFromArrayFdSet()+1, &rfds, NULL, NULL, NULL);
#endif
        if (ret < 0)
        {
            LOG_ERROR("select() return error");
            cleanupAndExitError(socketPath);
        }
#if (USE_CASE_SELECT_TIMEOUT)
        else if (0 == ret)
        {
            LOG_INFO("select() timeout and no data within %ld(s) and %ld(us)", tv2Print.tv_sec, tv2Print.tv_usec);
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
