#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>  // For _beginthread

#pragma comment(lib, "Ws2_32.lib") // Link against Winsock library

#define LISTENADDR "0.0.0.0"

// Error handler
char *error;

/**
 * Initialize server socket
 * @param portNo - The port number to bind the server to
 * @return 0 on error, socket otherwise
 */
int srv_init(int portNo) {
    SOCKET initializeSocket;
    struct sockaddr_in srv;

    // Create socket
    initializeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (initializeSocket == INVALID_SOCKET) {
        error = "socket() error";
        return 0;
    }

    // Server address setup
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = inet_addr(LISTENADDR);
    srv.sin_port = htons(portNo);

    // Bind the socket
    if (bind(initializeSocket, (struct sockaddr*)&srv, sizeof(srv)) == SOCKET_ERROR) {
        closesocket(initializeSocket);
        error = "bind() error";
        return 0;
    }

    // Listen for incoming connections
    if (listen(initializeSocket, 5) == SOCKET_ERROR) {
        closesocket(initializeSocket);
        error = "listen() error";
        return 0;
    }

    return initializeSocket;
}

/**
 * Accept a client connection
 * @param s - The server socket
 * @return client socket or 0 on error
 */
int cli_accept(SOCKET s) {
    SOCKET c;
    int addressLen = sizeof(struct sockaddr_in);
    struct sockaddr_in cli;

    memset(&cli, 0, sizeof(cli));
    c = accept(s, (struct sockaddr*)&cli, &addressLen);
    if (c == INVALID_SOCKET) {
        error = "accept() error";
        return 0;
    }

    return c;
}

#define PORT 8080 // Default port for the server
#define BASE_DIR "D:/projects/C-C++/web server/www" // Directory for the website files

/**
 * Placeholder function for handling client connection
 */
void cli_conn(void* client_socket) {
    SOCKET c = *(SOCKET*)client_socket;  // Dereference to get the socket
    free(client_socket);  // Free allocated memory for socket
    printf("Connected to client\n");

    // Buffer for incoming data
    char buffer[1024];
    recv(c, buffer, sizeof(buffer) - 1, 0); // Receive the request
    buffer[sizeof(buffer) - 1] = '\0'; // Null-terminate

    // Extract the requested URL from the request
    char *method = strtok(buffer, " "); // Get the HTTP method (e.g., GET)
    char *url = strtok(NULL, " "); // Get the URL
    char *script_name = url + 1; // Remove leading '/' from the URL

    //use default file if script name is empty 
    if(strlen(script_name) <= 0 ){
        script_name = "index.html";
    }
    printf("%s \n",script_name);

    // Construct the full path to the PHP file
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s", BASE_DIR, script_name);

    // Prepare the command to execute the PHP script
    char command[512];
    snprintf(command, sizeof(command), "php \"%s\" > response.txt", full_path);

    // Execute the PHP script
    int result = system(command); // Use system to call PHP interpreter

    // Check if the execution was successful
    if (result != 0) {
        const char *error_header = "HTTP/1.1 500 Internal Server Error\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Connection: close\r\n"
                                   "\r\n"
                                   "<html><body><h1>500 Internal Server Error</h1></body></html>\n";
        send(c, error_header, strlen(error_header), 0);
        closesocket(c);
        printf("Connection closed with error.\n");
        return;
    }

    // Read the output from the file
    FILE *response_file = fopen("response.txt", "r");
    if (!response_file) {
        perror("Could not open response file");
        closesocket(c);
        return;
    }

    // Prepare HTTP response headers
    const char *header = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/html\r\n"
                         "Connection: close\r\n"
                         "\r\n";  // Important: Double CRLF to signal end of headers

    // Send the response headers
    send(c, header, strlen(header), 0);

    // Send the body (output from the PHP script)
    char response_line[256];
    while (fgets(response_line, sizeof(response_line), response_file)) {
        send(c, response_line, strlen(response_line), 0);
    }

    // Close the response file
    fclose(response_file);

    // Close client socket after handling
    closesocket(c);
    printf("Connection closed.\n");
}

int main(int argc, char const* argv[]) {
    WSADATA wsaData;
    int s;
    SOCKET* client_socket;
    const char* port;  // Make port const to match argv type

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <listening port>\n", argv[0]);
        return -1;
    }

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }

    port = argv[1];

    // Initialize server
    s = srv_init(atoi(port));
    if (!s) {
        fprintf(stderr, "%s\n", error);
        WSACleanup();
        return -1;
    }

    printf("Listening on %s:%s\n", LISTENADDR, port);
    while (1) {
        SOCKET c = cli_accept(s);
        if (!c) {
            fprintf(stderr, "%s\n", error);
            continue;
        }

        printf("Incoming connection\n");

        // Allocate memory for the client socket and pass it to the thread
        client_socket = (SOCKET*)malloc(sizeof(SOCKET));
        if (client_socket == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            closesocket(c);
            continue;
        }
        *client_socket = c;

        // Create a new thread for each client
        _beginthread(cli_conn, 0, client_socket);
    }

    closesocket(s);
    WSACleanup();
    return 0;
}
