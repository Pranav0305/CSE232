#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

#define PROC_DIR "/proc"
#define STAT_FILE "stat"

typedef struct
{
    int pid;
    char name[256];          // Process name
    unsigned long utime;     // User mode CPU time
    unsigned long stime;     // Kernel mode CPU time
    unsigned long total_time; // Total CPU time (utime + stime)
} ProcessInfo;

ProcessInfo most_cpu_process, secondCPU_process;

// Function to read CPU time for a given process
int get_cpu_usage(int pid, ProcessInfo *info) 
{
    char filepath[256];
    FILE *file;
    
    // Build the path to the /proc/[PID]/stat file
    snprintf(filepath, sizeof(filepath), PROC_DIR "/%d/" STAT_FILE, pid);
    
    // Open the /proc/[PID]/stat file
    file = fopen(filepath, "r");
    if (!file) {
        return -1; 
    }
    
    fscanf(file, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %lu %lu", &info->pid, info->name, &info->utime, &info->stime);

    // Total time = user time + system time
    info->total_time = info->utime + info->stime;
    fclose(file);
    return 0;
}

// Function to find the PID of the most CPU-consuming process
int CPU_conusmer() 
{
    int found = -1,maxPID;
    DIR *proc_dir;
    struct dirent *entry, *entry1;
    ProcessInfo current_process;

    // Open the /proc directory
    proc_dir = opendir(PROC_DIR);
    if (!proc_dir) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    // Iterate over all entries in /proc
    while ((entry = readdir(proc_dir)) != NULL) 
    {
        // Check if the directory name is a PID (numeric)
        if (!isdigit(entry->d_name[0]))
            continue;

        int pid = atoi(entry->d_name);

        // Get the CPU usage for this process
        if (get_cpu_usage(pid, &current_process) == 0) 
        {
            if (found == -1 || current_process.total_time >= most_cpu_process.total_time) 
            {
                found = 1;
                most_cpu_process = current_process;
            }
        }

    }

    closedir(proc_dir);

  // Open the /proc directory
    proc_dir = opendir(PROC_DIR);
    if (!proc_dir) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    maxPID = most_cpu_process.pid;

    ProcessInfo current_process1;
    found = -1;

    while ((entry1 = readdir(proc_dir)) != NULL) 
    {
        // Check if the directory name is a PID (numeric)
        if (!isdigit(entry1->d_name[0]))
            continue;

        int pid = atoi(entry1->d_name);
        // printf("%d\n",pid);

        if(pid == maxPID)   
            continue;

        // Get the CPU usage for this process
        if (get_cpu_usage(pid, &current_process1) == 0) 
        {
            // printf("%s\n", current_process1.name);
            if (found == -1 || current_process1.total_time >= secondCPU_process.total_time) 
            {
                found = 1;
                secondCPU_process = current_process1;
            }
        }
    }

    closedir(proc_dir);

    return most_cpu_process.pid;
}
int main()
{
    int server_fd, new_socket, client_sockets[MAX_CLIENTS], max_sd, sd;
    struct sockaddr_in address;
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    // Initialize client sockets
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_sockets[i] = 0;
    }

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d\n", PORT);

    int addrlen = sizeof(address);

    while (1)
    {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add client sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            sd = client_sockets[i];
            if (sd > 0)
            {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd)
            {
                max_sd = sd;
            }
        }

        // Wait for activity on any of the sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR))
        {
            perror("Select error");
        }

        // If something happened on the server socket, it's an incoming connection
        if (FD_ISSET(server_fd, &readfds))
        {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("New connection, socket fd is %d\n", new_socket);

            // Add new socket to the client_sockets array
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_sockets[i] == 0)
                {
                    client_sockets[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        // Check all client sockets
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds))
            {
                // Check if it was for closing, and also read the incoming message
                int valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0)
                {
                    // Client disconnected
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("Host disconnected, IP %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    close(sd);
                    client_sockets[i] = 0;
                }
                else
                {
                    // Find top CPU consuming processes.
                    int mostCPU_pid = CPU_conusmer();
                    // int secondCPU_pid = CPU_conusmer(mostCPU_pid);

                    char msg[1024];
                    snprintf(msg, sizeof(msg),
                                "Most CPU consuming process:\n"
                                "PID: %d\n"
                                "Name: %s\n"
                                "User time: %ld\n"
                                "Kernel time: %ld\n"
                                "Total time: %ld\n\n"
                                "Second most CPU consuming process:\n"
                                "PID: %d\n"
                                "Name: %s\n"
                                "User time: %ld\n"
                                "Kernel time: %ld\n"
                                "Total time: %ld\n",
                                most_cpu_process.pid, most_cpu_process.name, most_cpu_process.utime, most_cpu_process.stime, most_cpu_process.total_time,
                                secondCPU_process.pid, secondCPU_process.name, secondCPU_process.utime, secondCPU_process.stime, secondCPU_process.total_time);

                    buffer[valread] = '\0';
                    printf("Received: %s\n", buffer);
                    // Send message to the client
                    send(sd, msg, strlen(msg), 0);
                }
            }
        }
    }

    return 0;
}
