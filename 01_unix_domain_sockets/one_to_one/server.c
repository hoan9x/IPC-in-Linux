#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/* LOG macro function */
#define LOG_INFO(format, ...) do { printf("[SERVER_INFO] " format "\n", ##__VA_ARGS__); } while (0)
#define LOG_ERROR(format, ...) do { printf("[SERVER_ERROR] " format "\n", ##__VA_ARGS__); } while (0)

#define DEFAULT_SOCKET_PATH "/tmp/DemoSocket"
#define BUFFER_SIZE 128
#define MAX_NUMBER_PENDING_CONNECTIONS 1

/* Global variable to control the loop */
volatile bool isKeepRunning = true;

/* Signal handler function for SIGINT (Ctrl+C) */
void handleSigint(int sig)
{
    LOG_INFO("Signal %d (Ctrl+C)", sig);
    isKeepRunning = false;
}

/* Function to clean up resources and exit */
void cleanupAndExitError(int connSocket, int dataSocket, const char *socketPath)
{
    if (-1 != dataSocket)
    {
        /* Close data socket if it is open */
        close(dataSocket);
    }
    if (-1 != connSocket)
    {
        /* Close connection socket (master socket file descriptor) if it is open */
        close(connSocket);
    }
    if (socketPath)
    {
        /* Remove the socket file */
        unlink(socketPath);
    }

    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    /* Register signal handler for SIGINT (Ctrl+C) */
    signal(SIGINT, handleSigint);
    LOG_INFO("Press Ctrl+C to set the shutdown flag...");

    struct sockaddr_un structSocketInfo;
    int connSocket = -1, dataSocket = -1, ret;
    char buffer[BUFFER_SIZE];
    /* Initialize socket path from application input parameter or default value */
    const char *socketPath = (argc>1)?argv[1]:DEFAULT_SOCKET_PATH;

    /* Remove the socket if it exists */
    unlink(socketPath);

    /* Create connection socket (master socket file descriptor) */
    connSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == connSocket)
    {
        LOG_ERROR("Creating a connection socket failed");
        cleanupAndExitError(-1, -1, socketPath);
    }
    LOG_INFO("Connection socket created");

    /* Initialize socket info structure */
    memset(&structSocketInfo, 0, sizeof(struct sockaddr_un));
    structSocketInfo.sun_family = AF_UNIX;
    strncpy(structSocketInfo.sun_path, socketPath, sizeof(structSocketInfo.sun_path)-1);

    /* Bind connection socket to path */
    ret = bind(connSocket, (const struct sockaddr *)&structSocketInfo, sizeof(struct sockaddr_un));
    if (-1 == ret)
    {
        LOG_ERROR("Bind connection socket to path failed");
        cleanupAndExitError(connSocket, -1, socketPath);
    }
    LOG_INFO("Bind connection socket to path [%s] succeeded", socketPath);

    /* Listen for incoming connections */
    ret = listen(connSocket, MAX_NUMBER_PENDING_CONNECTIONS);
    if (-1 == ret)
    {
        LOG_ERROR("Listening on socket failed");
        cleanupAndExitError(connSocket, -1, socketPath);
    }
    LOG_INFO("Listening for incoming connections...");

    /* Main server loop */
    while (isKeepRunning)
    {
        LOG_INFO("##### Waiting on accept()");
        dataSocket = accept(connSocket, NULL, NULL);
        if (-1 == dataSocket)
        {
            LOG_ERROR("accept() return error");
            cleanupAndExitError(connSocket, dataSocket, socketPath);
        }
        LOG_INFO("Connection established");

        /* ------------------------------------------
         Now the server and client can exchange data
         -------------------------------------------*/
        while (true)
        {
            /* Initialize the buffer array to 0 */
            memset(buffer, 0, BUFFER_SIZE);

            /* Read data from the client */
            LOG_INFO("Waiting for data from the client using read()");
            ret = read(dataSocket, buffer, BUFFER_SIZE);
            if (-1 == ret)
            {
                LOG_ERROR("read() return error");
                cleanupAndExitError(connSocket, dataSocket, socketPath);
            }
            else if (/*EOF*/0 == ret)
            {
                /* Once the client has closed the socket, the server will received the EOF message */
                LOG_INFO("Received EOF message");
                break;
            }
            else
            {
                LOG_INFO("Received data: [%.*s]", /*number read*/ret, buffer);

                /* Prepare data to send back to client */
                memset(buffer, 0, BUFFER_SIZE);
                snprintf(buffer, BUFFER_SIZE, ">>>>>Server return<<<<<");

                LOG_INFO("Send data to client: [%s]", buffer);
                ret = write(dataSocket, buffer, strlen(buffer)+1);
                if (-1 == ret)
                {
                    LOG_ERROR("Sending back to client data failed");
                    cleanupAndExitError(connSocket, dataSocket, socketPath);
                }
                LOG_INFO("Sending back to client data succeeded");
            }
        }

        /* Close data socket after communication is done */
        close(dataSocket);
    }

    /* Perform clean up */
    close(connSocket);
    unlink(socketPath);
    LOG_INFO("Server is down");

    return EXIT_SUCCESS;
}
