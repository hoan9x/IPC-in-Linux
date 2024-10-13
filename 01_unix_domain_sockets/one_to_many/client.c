/**------------------------------------------------------------------------------------------------
 * ?                                           ABOUT
 * @author         :  Nguyen Dinh Hoan
 * @email          :  hoann.wk@gmail.com
 * @repo           :  https://github.com/hoan9x/IPC-in-Linux
 * @createdOn      :  08-Oct-2024
 * @description    :  This example demonstrates a client exchanging data over a UNIX domain socket
 *------------------------------------------------------------------------------------------------**/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/* LOG macro function */
#define LOG_INFO(format, ...) do { printf("[CLIENT_INFO] " format "\n", ##__VA_ARGS__); } while (0)
#define LOG_ERROR(format, ...) do { printf("[CLIENT_ERROR] " format "\n", ##__VA_ARGS__); } while (0)

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
void cleanupAndExitError(int dataSocket)
{
    if (-1 != dataSocket)
    {
        /* Close data socket if it is open */
        close(dataSocket);
    }

    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    /* Register signal handler for SIGINT (Ctrl+C) */
    signal(SIGINT, handleSigint);
    LOG_INFO("Press Ctrl+C to set the shutdown flag...");

    struct sockaddr_un structSocketInfo;
    int dataSocket = -1, ret;
    char buffer[BUFFER_SIZE];
    /* Initialize socket path from application input parameter or default value */
    const char *socketPath = (argc>1)?argv[1]:DEFAULT_SOCKET_PATH;

    /* Create data socket */
    dataSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == dataSocket)
    {
        LOG_ERROR("Creating a data socket failed");
        cleanupAndExitError(dataSocket);
    }
    LOG_INFO("Data socket created");

    /* Initialize socket info structure */
    memset(&structSocketInfo, 0, sizeof(struct sockaddr_un));
    structSocketInfo.sun_family = AF_UNIX;
    strncpy(structSocketInfo.sun_path, socketPath, sizeof(structSocketInfo.sun_path)-1);

    LOG_INFO("Request connection from socket path: [%s]", socketPath);
    ret = connect(dataSocket, (const struct sockaddr *)&structSocketInfo, sizeof(struct sockaddr_un));
    if (-1 == ret)
    {
        LOG_ERROR("Connection request failed, server is down");
        cleanupAndExitError(dataSocket);
    }

    /**------------------------------------------------------------------------
     *                Now the server and client can exchange data
     *------------------------------------------------------------------------**/
    for (int index = 0; isKeepRunning; ++index)
    {
        /* Prepare data to send to server */
        memset(buffer, 0, BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, ">>>>>Client data (%d)<<<<<", index);

        LOG_INFO("Send data to server: [%s]", buffer);
        ret = write(dataSocket, buffer, strlen(buffer)+1);
        if (-1 == ret)
        {
            LOG_ERROR("Send data to server failed");
            cleanupAndExitError(dataSocket);
        }
        /* Sleep for 3s */
        sleep(3);
    }

    /* Close socket */
    close(dataSocket);
    LOG_INFO("Client is down");

    return  EXIT_SUCCESS;
}
