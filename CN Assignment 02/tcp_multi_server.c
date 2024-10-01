#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#define PORT 9110
#define MAX_CLIENTS 10
#define PROC_DIR "/proc"
#define STAT_FILE "stat"

typedef struct
{
    int pid;
    char name[256];          
    unsigned long utime;     
    unsigned long stime;     
    unsigned long total_time; 
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

void *handleClient(void *socketDesc)
{
    int client_socket = *(int *)socketDesc;

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

   // Send message to client.
    if(send(client_socket,msg,sizeof(msg), 0) == -1)
    {
        perror("Failed to send message");
        exit(1);
    }

    // Receive message from client.
    char buf[1024];
    if(recv(client_socket,&buf,sizeof(buf),0) == -1)
    {
        perror("Failed to receive message");
        exit(1);
    }

    printf("Client: %s\n",buf);

    close(client_socket);
    free(socketDesc);
}

int main()
{
    // Create socket.
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int client_socket;
    socklen_t clientLen;

    // Define server address.
    struct sockaddr_in server_address,client_addr;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to specified IP.
    if(bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1)
    {
        perror("Failed to bind");
        exit(1);
    }

    // Listen for clients.
    if(listen(server_socket, MAX_CLIENTS) == -1)
    {
        perror("Failed to listen");
        exit(1);
    }

    // Size of client struct.
    clientLen = sizeof(struct sockaddr_in);

    // Accept client connection.
    while(client_socket = accept(server_socket, (struct sockaddr_in *) &client_addr, &clientLen))
    {
        if(client_socket == -1)
        {
            perror("Failed to accept");
            exit(1);
        }
        
        printf("Connection accepted\n");
        int *new_sock;
        new_sock = malloc(sizeof(int));
        *new_sock = client_socket;

        // Create a new thread for every client.
        pthread_t tid;
        if(pthread_create(&tid, NULL, handleClient, (void *)new_sock) == -1)
        {
            perror("Error creating thread");
            exit(1);
        }

        if(pthread_detach(tid) == -1)
        {
            perror("Error detatching thread");
            exit(1);
        }

    }

    // Close the server socket.
    close(server_socket);

}
