#include <pthread.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include "udpServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdatomic.h>
#include <stdbool.h>
#include "sampler.h"
#define PORT 12345
#define MAX_LEN 1500

static pthread_t tUdpServerPID;
static int socketDescriptor;
static ServerCommands prevCommandType = UNKNOWN; // inital value changed during execution
static char messageRx[MAX_LEN];                  // receive message

static int initReceiver();
static char *processCommand(const char *command);
static void *receiveRequest();
static const char *getCommandFromEnum(ServerCommands cmd);

int *continueFlag2;

int initUdpServer(int *flag)
{
    continueFlag2 = flag;

    if (initReceiver() < 0)
    {
        perror("Error creating UDP Socket\n");
        return -1;
    }
    if (pthread_create(&tUdpServerPID, NULL, receiveRequest, NULL) != 0)
    {
        perror("Error creating UDP Thread\n");
        return -1;
    }
    // printf("Listening on UDP port %s:\n", PORT); //fordebugging

    return 0;
}
static void *receiveRequest()
{
    char *reply;
    int terminateIdx;
    int bytesRx;
    while (continueFlag2)
    { // Continuously listen for incoming UDP packets
        struct sockaddr_in sinRemote;
        unsigned int sin_len = sizeof(sinRemote);

        bytesRx = recvfrom(socketDescriptor, messageRx, MAX_LEN - 1, 0, (struct sockaddr *)&sinRemote, &sin_len);

        if (bytesRx == -1)
        {
            perror("Error receiving UDP packet");
            fflush(stdout);

            continue; // Continue to listen for the next packet
        }
     
        terminateIdx = (bytesRx < MAX_LEN) ? bytesRx : MAX_LEN - 1;

        messageRx[terminateIdx] = '\0'; // Null-terminate the received message

        // Process the received command:
        reply = processCommand(messageRx); // reply is a dynamically allocated string that needs to be freed after being sent
        if (reply != NULL)
        {

            // strncpy(messageTx, reply, MAX_LEN);
            sendto(socketDescriptor, reply, strlen(reply), 0, (struct sockaddr *)&sinRemote, sin_len);
            if (strcmp(reply, "Program terminating.\n") == 0)
            { // shutdown
                free(reply);
                *continueFlag2 = 0;
                break;
            }
            free(reply);
             
        }
        
    }

    return NULL;
}
static int initReceiver()
{
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
    socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

    if (socketDescriptor == -1)
    {
        close(socketDescriptor);
        perror("ERROR in INIT RECEIVER Socket creation");
        return -1;
    }

    if (bind(socketDescriptor, (struct sockaddr *)&sin, sizeof(sin)))
    {
        close(socketDescriptor);
        perror("ERROR in INIT RECEIVER Socket creation");
        return -1;
    }
    //printf("Listening on UDP port %d:\n", PORT);
    return 0;
}

static const char *getCommandFromEnum(ServerCommands cmd)
{
    switch (cmd)
    {
    case HELP:
        return "help";
    case COUNT:
        return "count";
    case LENGTH:
        return "length";
    case DIPS:
        return "dips";
    case HISTORY:
        return "history";
    case STOP:
        return "stop";
    default:
        return "";
    }
}
// This function is AI generated based on our specific requirements
// could have written it myself but would have taken much longer :3
char *doubleArrayToString(double *arr, int size, const char *delimiter)
{
    // Calculate total length of the resulting string
    int totalLength = 0;
    for (int i = 0; i < size; i++)
    {
        // Add length of each double value and delimiter
        totalLength += snprintf(NULL, 0, "%lf", arr[i]);
        if (i < size - 1)
        {
            totalLength += strlen(delimiter);
        }
    }

    // Allocate memory for the resulting string
    char *str = (char *)malloc((totalLength + (size)*strlen(delimiter) + 1) * sizeof(char));
    if (str == NULL)
    {
        perror("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Concatenate double values into the string
    int offset = 0;
    for (int i = 0; i < size; i++)
    {
        // Convert double to string and concatenate
        offset += sprintf(str + offset, "%lf", arr[i]);
        if (i < size - 1)
        {
            // Add delimiter if not the last element
            strcat(str + offset, delimiter);
            offset += strlen(delimiter);
        }
    }
    strcat(str, "\n");
    return str;
}
static char *processCommand(const char *command)
{                                       // must be called inside of thread
    ServerCommands selectedCommandType; // Command that the user asked for
    // If the command is NULL or contains only whitespace
    char *result = malloc(450 * sizeof(char)); // Allocate memory for result slightly extra for good measure :)

    if (command == NULL || *command == '\0' || strspn(command, " \t\n"))
    {
        // If this is the first command or no previous command, treat as unknown
        if (prevCommandType == UNKNOWN)
        {

            snprintf(result, 100, "Type 'help' for a list of supported commands.\n");
            return result;
        }
        // Repeat the previous command if it wasn't unknown
        command = getCommandFromEnum(prevCommandType);
    }

    // Remove leading whitespace
    while (isspace(*command))
    {
        command++;
    }
    // Remove trailing whitespace
    int len = strlen(command);
    while (len > 0 && isspace(command[len - 1]))
    {
        len--;
    }
    char cleanedCommand[len + 1];
    strncpy(cleanedCommand, command, len);
    cleanedCommand[len] = '\0';


    // Determine the command based on the received string
    if (strcasecmp(cleanedCommand, "help") == 0 || strcasecmp(cleanedCommand, "?") == 0)
    {
        selectedCommandType = HELP;
        prevCommandType = selectedCommandType;
        strcpy(result, "Accepted command examples:\ncount -- get the total number of samples taken.\nlength -- get the number of samples taken in the previously completed\nsecond.\ndips -- get the number of dips in the previously completed second.\nhistory -- get all the samples in the previously completed second.\nstop -- cause the server program to end.\n<enter> -- repeat last command.\n");
    }
    else if (strcasecmp(cleanedCommand, "count") == 0)
    {
        selectedCommandType = COUNT;
        prevCommandType = selectedCommandType;
        long long samplesTakenSoFar = Sampler_getNumSamplesTaken(); // Convert to string and concat
        snprintf(result, 100, "# samples taken total: %lld\n", samplesTakenSoFar);
    }
    else if (strcasecmp(cleanedCommand, "length") == 0)
    {
        selectedCommandType = LENGTH;
        prevCommandType = selectedCommandType;
        int samples = Sampler_getHistorySize();
        snprintf(result, 100, "# samples taken last second: %d\n", samples);
    }
    else if (strcasecmp(cleanedCommand, "dips") == 0)
    {
        selectedCommandType = DIPS;
        prevCommandType = selectedCommandType;
        snprintf(result, 100, "# Dips: %d\n", getDips());
    }
    else if (strcasecmp(cleanedCommand, "history") == 0)
    {
        selectedCommandType = HISTORY;
        prevCommandType = selectedCommandType;
        int size;
        double *historyArr = Sampler_getHistory(&size);

        char delimiter[] = ", ";
        char *historyStr = doubleArrayToString(historyArr, size, delimiter);
        free(result); // result is recalculated to be a completely different array so we just point to the new array instead
        result = historyStr;
        free(historyArr);
    }
    else if (strcasecmp(cleanedCommand, "stop") == 0)
    {
        selectedCommandType = STOP;
        prevCommandType = selectedCommandType;
        snprintf(result, 100, "Program terminating.\n");
    }
    else
    {
        selectedCommandType = UNKNOWN;
        prevCommandType = selectedCommandType;
        snprintf(result, 100, "Unknown command. Type 'help' for a list of supported commands.\n");
    }

    return result;
}

// Joins threads and closes socket
int shutdownUdpServer()
{
    // JOIN THREADS YO

    if (pthread_join(tUdpServerPID, NULL) != 0)
    {
        perror("Error joining receiver\n");
        return -1;
    }
    if (close(socketDescriptor) != 0)
    {
        perror("Error closing SocketDescriptor\n");
        return -1;
    }
    return 1;
}
