#include "server.h"

struct file_node* fhead = NULL;
struct node* head = NULL;


pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[])
{

    int opt = 0;
    int serving_port = 0;
    int command_port = 0;
    int num_of_threads = 0;
    char root_dir[128];

    // run with ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir


    while ((opt = getopt(argc, argv, "p:c:t:d:")) != -1)
    {
        switch(opt)
        {
        case 'p':
            serving_port = atoi(optarg);
            break;
        case 'c':
            command_port = atoi(optarg);
            break;
        case 't':
            num_of_threads = atoi(optarg);
            break;
        case 'd':
            strcpy(root_dir, optarg);
            break;
        }
    }

    pthread_t *thread_ids = malloc(num_of_threads * (sizeof(pthread_t)));
    int i;
    struct file_node* file;

    //Create thread pool
    for ( i = 0 ; i < num_of_threads ; i ++)
    {
        pthread_create (&thread_ids[i], NULL, thread_f, NULL);
    }

    char http_header[BUFFER_SIZE];

    int server_socket_fd, client_socket_fd;
    int socket_fds[2];

    //Create socket

    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }
    socket_fds[0] = server_socket_fd;

    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }
    socket_fds[1] = server_socket_fd;

    struct sockaddr_in address, caddr;
    socklen_t clientlen = sizeof(caddr);



    //Define server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(serving_port);

    if (bind(socket_fds[0], (struct sockaddr *) &address, sizeof(address)) < 0)
    {
        perror("Error in bind");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address1;

    //Define server address
    address1.sin_family = AF_INET;
    address1.sin_addr.s_addr = INADDR_ANY;
    address1.sin_port = htons(command_port);

    if (bind(socket_fds[1], (struct sockaddr *) &address1, sizeof(address1)) < 0)
    {
        perror("Error in bind");
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fds[0], 5) < 0)
    {
        perror("Error in listen");
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fds[1], 5) < 0)
    {
        perror("Error in listen");
        exit(EXIT_FAILURE);
    }

    char request[1024];
    char path[1024];
    char dir[1024];
    char method[3];
    struct node *p;
    int fd;
    int total_pages = 0, total_bytes = 0;

    fd_set readfds;
    int maxfd, sockfd;
    int status;

    clock_t begin = clock();

    while(1)
    {
        sleep(1);
        puts("Waiting for connections...");

        strcpy(dir, root_dir);

        FD_ZERO(&readfds);
        maxfd = -1;
        for (i = 0; i < 2; i++)
        {
            FD_SET(socket_fds[i], &readfds);
            if (socket_fds[i] > maxfd)
                maxfd = socket_fds[i];
        }
        if(status = (select(maxfd + 1, &readfds, NULL, NULL, NULL)) < 0)
        {
            perror("Error in select");
            exit(EXIT_FAILURE);
        }

        sockfd = -1;

        for (i = 0; i < 2; i++)
            if (FD_ISSET(socket_fds[i], &readfds))
            {
                sockfd = socket_fds[i];
                break;
            }
        if (sockfd == -1)
            exit(EXIT_FAILURE);
        else
        {
            clientlen = sizeof(caddr);
            if ((client_socket_fd = accept(sockfd, (struct sockaddr *) &caddr, &clientlen)) < 0)
            {
                perror("Error in accept");
                exit(EXIT_FAILURE);
            }
            getsockname(client_socket_fd, (struct sockaddr *) &caddr, &clientlen);
            printf("Client connected to port: %d\n", ntohs(caddr.sin_port));
        }
        recv(client_socket_fd, &request, sizeof(request), 0);

        sscanf(request,"%s%s", method, path);

        if(ntohs(caddr.sin_port) == command_port)
        {

            if (!strcmp(method, "STATS"))
            {
                clock_t end = clock();
                double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
                snprintf(http_header, sizeof(http_header), "Server up for %f, served %d pages, %d bytes\n", time_spent, total_pages, total_bytes);
                send(client_socket_fd, http_header, sizeof(http_header), 0);

                memset(http_header, '\0', sizeof(http_header));
                memset(path, '\0', sizeof(path));
                memset(method, '\0', sizeof(method));
                continue;
            }
            else if(!strcmp(method, "SHUTDOWN"))
            {
                printf("Terminating server...\n");

                for(i = 0; i < 2; i++)
                {
                    close(socket_fds[i]);
                }

                return 0;
            }
            else
            {
                snprintf(http_header, sizeof(http_header), "Usage: STATS/SHUTDOWN\n");
                send(client_socket_fd, http_header, sizeof(http_header), 0);

                memset(http_header, '\0', sizeof(http_header));
                memset(path, '\0', sizeof(path));
                memset(method, '\0', sizeof(method));
                continue;

            }
        }
        if (strcmp(method, "GET"))
        {
            snprintf(http_header, sizeof(http_header), "Usage: GET </siteA/pageA_B.html>\n");
            send(client_socket_fd, http_header, sizeof(http_header), 0);

            memset(http_header, '\0', sizeof(http_header));
            memset(path, '\0', sizeof(path));
            memset(method, '\0', sizeof(method));
            continue;
        }

        strcat(dir, path);

        if ((fd = open(dir, O_RDONLY)) == -1)
        {
            if (access(dir, R_OK) != 0)
            {
                if (errno == EACCES)
                {
                    snprintf(http_header, sizeof(http_header), "\nHTTP/1.1 403 Forbidden\r\n"
                             "Server: myhttpd/1.0.0 (Ubuntu64)\r\n"
                             "Date: %s\r\n"
                             "Content-Type: text/html\r\n"
                             "Connection: Closed\r\n\r\n",
                             get_current_date(date, sizeof(date)));
                    send(client_socket_fd, http_header, sizeof(http_header), 0);

                    printf("File \"%s\" is not accessible\n", dir);
                    memset(http_header, '\0', sizeof(http_header));
                    memset(dir, '\0', sizeof(dir));
                    memset(path, '\0', sizeof(path));
                }
                else
                {
                    snprintf(http_header, sizeof(http_header), "\nHTTP/1.1 404 Not Found\r\n"
                             "Server: myhttpd/1.0.0 (Ubuntu64)\r\n"
                             "Date: %s\r\n"
                             "Content-Type: text/html\r\n"
                             "Connection: Closed\r\n\r\n",
                             get_current_date(date, sizeof(date)));
                    send(client_socket_fd, http_header, sizeof(http_header), 0);

                    printf("File \"%s\" does not exist\n", dir);
                    memset(http_header, '\0', sizeof(http_header));
                    memset(dir, '\0', sizeof(dir));
                    memset(path, '\0', sizeof(path));
                }
            }

            close(client_socket_fd);
            continue;
        }
        p = malloc(sizeof(struct node));
        p -> fd = fd;

        pthread_mutex_lock(&mtx);

        p -> next = head;
        head = p;

        pthread_cond_signal(&cond);

        while(fhead == NULL)
        {
            pthread_cond_wait(&cond2, &mtx);
        }

        file = fhead;
        fhead = fhead -> next;

        snprintf(http_header, sizeof(http_header), "\nHTTP/1.1 200 OK\r\n"
                 "Server: myhttpd/1.0.0 (Ubuntu64)\r\n"
                 "Date: %s\r\n"
                 "Content-Type: text/html\r\n"
                 "Content-Length: %ld\r\n"
                 "Connection: Closed\r\n\r\n",
                 get_current_date(date, sizeof(date)),
                 strlen(file -> buffer));

        strcat(http_header, file -> buffer);
        send(client_socket_fd, http_header, sizeof(http_header), 0);
        printf("File \"%s\" has been sent to client\n", dir);

        total_bytes += strlen(file -> buffer);
        total_pages++;

        memset(http_header, '\0', sizeof(http_header));
        memset(dir, '\0', sizeof(dir));
        memset(path, '\0', sizeof(path));

        free(file);

        pthread_mutex_unlock(&mtx);

        close(client_socket_fd);
    }


    return 0;
}


void *thread_f(void *arg)
{
    struct node *p;
    struct file_node *f;
    char buffer[BUFFER_SIZE];

    while(1)
    {
        pthread_mutex_lock(&mtx);

        printf("Thread %lu started...\n", pthread_self());

        while(head == NULL)
        {
            pthread_cond_wait(&cond, &mtx);
        }

        p = head;
        head = head -> next;
        read(p -> fd, buffer, sizeof(buffer));
        free(p);

        f = malloc(sizeof(struct file_node));
        strcpy(f -> buffer, buffer);
        f -> next = fhead;
        fhead = f;
        pthread_cond_signal(&cond2);
        pthread_mutex_unlock(&mtx);
    }

    pthread_exit(NULL);
}


char *get_current_date(char *str, int len)
{
    time_t t = time(NULL);
    struct tm res;

    strftime(str, len, RFC1123FMT, localtime_r(&t, &res));
    return str;
}
