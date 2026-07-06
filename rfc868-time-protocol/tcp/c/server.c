#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>

#define PORT 8000
#define BACKLOG 3
#define MAX_BUFFER_SIZE 1024
#define SERVER_RESPONSE "Hello From The Server\n"
#define EXIT_COMMAND "EXIT\r\n"
#define MAX_CONN 2
#define ADDR_LEN 22

static int CLOSE_COMMAND = EOF;

void print_fd_set(fd_set *fdset, int max_fd)
{
    printf("FDSET: ");
    for (int i = 3; i <= max_fd; i++)
    {
        if (FD_ISSET(i, fdset))
            printf("%d", i);
    }
    printf("\n");
}

typedef struct _conn
{
    int fd;
    char read_buffer[MAX_BUFFER_SIZE];
    size_t read_buffer_len;
    char write_buffer[MAX_BUFFER_SIZE];
    size_t write_buffer_len;
    char addr[ADDR_LEN];
} Connection;

typedef struct _srv
{
    int fd;
    Connection *conntab[MAX_CONN];
    int max_fd;
} Server;

Connection *create_connection()
{
    Connection *connection = calloc(1, sizeof(*connection));
    return connection;
}

Server *create_server()
{
    Server *server = calloc(1, sizeof(*server));
    // printf("== Server space allocated ==\n");

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    for (int i = 0; i < MAX_CONN; i++)
        server->conntab[i] = create_connection();

    return server;
}

void set_socket_options(int fd)
{
    int status_flags;
    status_flags = fcntl(fd, F_GETFL);
    status_flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, status_flags);

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

void bind_server(Server *server, char *port)
{
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    set_socket_options(server->fd);

    int bind_result = bind(server->fd, (struct sockaddr *)&address, sizeof(address));
    if (bind_result == -1)
    {
        perror("Unable to bind");
        close(server->fd);
        exit(EXIT_FAILURE);
    }

    int listen_result = listen(server->fd, BACKLOG);
    if (listen_result == -1)
    {
        perror("Unable to listen");
        close(server->fd);
        exit(EXIT_FAILURE);
    }
}

int add_connection(Server *server, int fd)
{
    int i;
    for (i = 0; i < MAX_CONN && server->conntab[i]->fd > 0; i++)
    {
    }
    if (i == MAX_CONN)
        return -1;
    server->conntab[i]->fd = fd;
    return i;
}

int search_connection(Server *server, fd_set *readfds)
{
    // printf("Start the search...\n");
    for (int i = 0; i < MAX_CONN - 1; i++)
    {
        Connection *c = server->conntab[i];
        // printf("Searching for ready connection with fd: %d\n", c->fd);
        if (FD_ISSET(c->fd, readfds))
        {
            // printf("Found ready connection with fd: %d\n", c->fd);
            FD_CLR(c->fd, readfds);
            // printf("Cleared connection with fd: %d\n", c->fd);
            // print_fd_set(readfds, server->max_fd);
            return i;
        }
    }
    return -1;
}

void remove_connection(Server *server, int fd)
{
    int i;
    for (i = 0; i < MAX_CONN - 1 && server->conntab[i]->fd != fd; i++)
    {
    }
    server->conntab[i]->fd = 0;
    memset(server->conntab[i]->read_buffer, 0, sizeof(server->conntab[i]->read_buffer));
    memset(server->conntab[i]->write_buffer, 0, sizeof(server->conntab[i]->write_buffer));
    memset(server->conntab[i]->addr, 0, sizeof(server->conntab[i]->addr));
    server->conntab[i]->read_buffer_len = 0;
    server->conntab[i]->write_buffer_len = 0;
    close(fd);
}

int accept_new_connection(Server *server, int *error)
{
    struct sockaddr_in address = {0};
    socklen_t addrlen = sizeof(address);

    int new_connection_socket = accept(server->fd, (struct sockaddr *)&address, &addrlen);
    if (new_connection_socket == -1)
    {
        perror("Unable to accept new connection");
        exit(EXIT_FAILURE);
    }
    set_socket_options(new_connection_socket);
    int add_connection_result = add_connection(server, new_connection_socket);
    if (add_connection_result >= 0)
    {
        char ip[INET_ADDRSTRLEN];
        sprintf(server->conntab[add_connection_result]->addr, "%s:%d", inet_ntop(AF_INET, &(address.sin_addr), ip, INET_ADDRSTRLEN), ntohs(address.sin_port));
        printf("Accepted connection from %s\n", server->conntab[add_connection_result]->addr);
    }
    *error = add_connection_result;
    return new_connection_socket;
    // printf("Server max fd: %d and new socket: %d\n", server->max_fd, new_connection_socket);
    // printf("New connection pointer: %p\n", connection);
    // if (server->max_fd < new_connection_socket + 1)
    //     server->max_fd = new_connection_socket + 1;
    // printf("added connection with fd %d - connections_len: %d server max_fd: %d\n", new_connection_socket, server->connections_len,server->max_fd);
}

char *find_char(char *text, char item)
{
    char *p;
    for (p = text; *p != item && *p != '\0'; p++)
    {
    }
    if (*p == '\0')
        return NULL;
    return p;
}

void handle_read(Server *server, Connection *connection)
{
    size_t remaining_space = sizeof(connection->read_buffer) - connection->read_buffer_len - 1;

    if (remaining_space == 0)
    {
        printf("Buffer is full. Closing the connection\n");
        remove_connection(server, connection->fd);
        return;
    }

    printf("Reading connection %d\n", connection->fd);
    int read_result = read(connection->fd, connection->read_buffer, remaining_space);
    printf("Read result: %d\n", read_result);
    if (read_result > 0)
    {
        connection->read_buffer_len += read_result;
        connection->read_buffer[connection->read_buffer_len] = '\0';

        char *newline_position = find_char(connection->read_buffer, '\n');
        if (newline_position != NULL)
        {
            printf("Received a complete message from connection: %s", connection->read_buffer);
            size_t write_buffer_capacity = sizeof(connection->write_buffer) - connection->write_buffer_len;
            if (read_result > write_buffer_capacity)
            {
                remove_connection(server, connection->fd);
                printf("Connections dedicated buffer is out of capacity address: %s\n", connection->addr);
                return;
            }
            strcpy(connection->write_buffer, connection->read_buffer);
            connection->write_buffer_len += read_result;
            memset(connection->read_buffer, 0, sizeof(connection->read_buffer));
            connection->read_buffer_len = 0;
        }
    }
    else if (read_result == 0)
    {
        printf("Client closed the connection wit fd: %d\n", connection->fd);
        remove_connection(server, connection->fd);
    }
    else
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // try again later
        }
        else
        {
            remove_connection(server, connection->fd);
        }
    }
}

void handle_write(Server *server, Connection *connection)
{
    if (connection->write_buffer_len == 0)
        return;
    int write_result = write(connection->fd, connection->write_buffer, connection->write_buffer_len);
    if (write_result > 0)
    {
        if (write_result < connection->write_buffer_len)
        {
            connection->write_buffer_len -= write_result;
            memmove(connection->write_buffer, connection->write_buffer + write_result, connection->write_buffer_len);
        }
        else
        {
            memset(connection->write_buffer, 0, sizeof(connection->write_buffer));
            connection->write_buffer_len = 0;
        }
    }
    else if (write_result < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        remove_connection(server, connection->fd);
    }
}

fd_set reload_readfds(Server *server)
{
    fd_set readfds;
    int max_fd = server->fd;
    FD_ZERO(&readfds);
    FD_SET(server->fd, &readfds);
    for (int i = 0; i < MAX_CONN; i++)
    {
        Connection *c = server->conntab[i];
        if (c->fd > 0)
        {
            FD_SET(c->fd, &readfds);
            if (max_fd < c->fd)
                max_fd = c->fd;
        }
    }
    server->max_fd = max_fd + 1;
    return readfds;
}

fd_set reload_writefds(Server *server)
{
    fd_set writefds;
    int max_fd = server->fd;
    FD_ZERO(&writefds);
    for (int i = 0; i < MAX_CONN; i++)
    {
        Connection *c = server->conntab[i];
        if (c->fd > 0 && c->write_buffer_len > 0)
        {
            FD_SET(c->fd, &writefds);
            if (max_fd < c->fd)
                max_fd = c->fd;
        }
    }
    if (server->max_fd < max_fd + 1)
        server->max_fd = max_fd + 1;
    return writefds;
}

void handle_server_read(Server *server, fd_set *readfds, fd_set *writefds)
{
    if (FD_ISSET(server->fd, readfds))
    {
        int error = 0;
        int fd = accept_new_connection(server, &error);
        if (error == -1)
        {
            printf("Server is full. Closing the connection...\n");
            close(fd);
            printf("Connection closed.\n");
        }
    }
}

void start_server(Server *server)
{
    printf("== Listening on port %d ==\n", PORT);
    while (1)
    {
        fd_set readfds = reload_readfds(server);
        fd_set writefds = reload_writefds(server);
        int select_result = select(server->max_fd, &readfds, &writefds, NULL, NULL);
        printf("== Select invoked ==\n");
        handle_server_read(server, &readfds, &writefds);
        int i;
        while ((i = search_connection(server, &readfds)) != -1)
        {
            printf("Reading from client connection\n");
            Connection *connection = server->conntab[i];
            // printf("Connection selected - fd: %d\n", connection->fd);
            handle_read(server, connection);
        }
        while ((i = search_connection(server, &writefds)) != -1)
        {
            printf("Writing to client connection\n");
            Connection *connection = server->conntab[i];
            // printf("Connection selected - fd: %d\n", connection->fd);
            handle_write(server, connection);
        }
        // exit(EXIT_SUCCESS);
    }
}

int main()
{
    printf("== Application Startup ==\n");
    Server *server = create_server();
    printf("== Server created ==\n");
    bind_server(server, PORT);
    start_server(server);
    free(server);
    return 0;
}