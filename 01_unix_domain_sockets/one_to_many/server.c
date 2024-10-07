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

#define DEFAULT_SOCKET_PATH "/tmp/DemoSocket"
#define BUFFER_SIZE 128
#define MAX_NUMBER_PENDING_CONNECTIONS 10
#define MAX_CLIENT_SUPPORTED 32

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

/* Get the max value among all FDs which server is monitoring */
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
    struct sockaddr_un structSocketInfo;
    fd_set structFdSet;
    int connSocket = -1, dataSocket = -1, ret, commSocketFd, i;
    char buffer[BUFFER_SIZE];
    /* Initialize socket path from application input parameter or default value */
    const char *socketPath = (argc>1)?argv[1]:DEFAULT_SOCKET_PATH;

    initArrayFdSet();
    /* Add socket descriptor 0 to monitor stdin */
    addToArrayFdSet(0);

    /* Remove the socket if it exists */
    unlink(socketPath);

    /* Create connection socket (master socket file descriptor) */
    connSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == connSocket)
    {
        LOG_ERROR("Creating a connection socket failed");
        cleanupAndExitError(socketPath);
    }
    LOG_INFO("Connection socket created (%d)", connSocket);

    /* Initialize socket info structure */
    memset(&structSocketInfo, 0, sizeof(struct sockaddr_un));
    structSocketInfo.sun_family = AF_UNIX;
    strncpy(structSocketInfo.sun_path, socketPath, sizeof(structSocketInfo.sun_path)-1);

    /* Bind connection socket to path */
    ret = bind(connSocket, (const struct sockaddr *)&structSocketInfo, sizeof(struct sockaddr_un));
    if (-1 == ret)
    {
        LOG_ERROR("Bind connection socket to path failed");
        cleanupAndExitError(socketPath);
    }
    LOG_INFO("Bind connection socket to path [%s] succeeded", socketPath);

    /**
     * Listen for incoming connections, the second parameter means that while a request is being processed,
     * MAX_NUMBER_PENDING_CONNECTIONS requests can wait.
     */
    ret = listen(connSocket, MAX_NUMBER_PENDING_CONNECTIONS);
    if (-1 == ret)
    {
        LOG_ERROR("Listening on socket failed");
        cleanupAndExitError(socketPath);
    }
    LOG_INFO("Listening for incoming connections...");

    /* Add connection socket to array set of FDs */
    addToArrayFdSet(connSocket);

    /* Main server loop */
    for (;;)
    {
        cloneFdSetStruct(&structFdSet);
        LOG_INFO("##### Waiting on select()");

        /* Call select(), the server will block until there is a connection or data request on any FDs */
        select(getMaxFromArrayFdSet()+1, &structFdSet, NULL, NULL, NULL);

        if (FD_ISSET(connSocket, &structFdSet))
        {
            LOG_INFO("New connection received, accepting the connection");
            dataSocket = accept(connSocket, NULL, NULL);
            if (-1 == dataSocket)
            {
                LOG_ERROR("accept() return error");
                cleanupAndExitError(socketPath);
            }
            LOG_INFO("Connection established (%d)", dataSocket);
            addToArrayFdSet(dataSocket);
        }
        else if (FD_ISSET(0, &structFdSet))
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
                if (FD_ISSET(arrayFdSet[i], &structFdSet))
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
