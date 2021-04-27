#include <iostream>
#include <winsock2.h>
#include <cstdlib>
#include <unistd.h>
#include <windows.h>

#define DEFAULT_PORT 7
#define MAX_CONNECTED_CLIENTS 3

using namespace std;

volatile bool running = true;
int connected_clients = 0;
CRITICAL_SECTION cs;

// Prototypes
DWORD WINAPI client_thread(LPVOID args);
void handle_client(sockaddr_in client_addr, SOCKET client_sock);
void not_handle_client(sockaddr_in client_addr, SOCKET client_sock);
void accept_client(SOCKET server_sock, sockaddr_in &client_addr, SOCKET &client_sock);
SOCKET prepare_server(int port);
void close_server(SOCKET server_sock);
int get_port(int argc, char *const *argv);

struct client_data_t {
    SOCKET client_sock;
    sockaddr_in client_addr;
};

int main(int argc, char **argv) {

    InitializeCriticalSection(&cs);
    int port = get_port(argc, argv);
    SOCKET server_sock = prepare_server(port);

    while(running) {
        struct sockaddr_in client_addr;
        struct client_data_t client_data;
        SOCKET client_sock;

        accept_client(server_sock, client_addr, client_sock);
        client_data.client_sock = client_sock;
        client_data.client_addr = client_addr;
        CreateThread(NULL, 0, client_thread, &client_data, 0, NULL);
    }

    close_server(server_sock);
    DeleteCriticalSection(&cs);
}

int get_port(int argc, char *const *argv) {
    int port = DEFAULT_PORT;
    if(argc > 1) {
        port = atoi(argv[1]);
        if(port < 0 || port > 65535) {
            cerr << "Invalid port number supplied" << endl;
            exit(EXIT_FAILURE);
        }
    }
    return port;
}

SOCKET prepare_server(int port) {

    // WSAStartup
    WSADATA wsadata;
    int result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result != 0) {
        cerr << "Unable to initialize winsock. Error: " << WSAGetLastError() << endl;
        exit(EXIT_FAILURE);
    }

    // Socket
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        cerr << "Unable to create socket. Error: " << WSAGetLastError() << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Bind
    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    result = bind(server_sock, (struct sockaddr *) &sockaddr, sizeof(sockaddr_in));
    if (result == SOCKET_ERROR) {
        cerr << "Unable to bind. Error: " << WSAGetLastError << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Listen
    result = listen(server_sock, 10);
    if(result == SOCKET_ERROR) {
        cerr << "Unable to listen. Error: " << WSAGetLastError << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    cout << "Server is listening on port " << port << endl;

    return server_sock;
}

void close_server(SOCKET server_sock) {// close socket

    // Close server socket
    int result = closesocket(server_sock);
    if(result == SOCKET_ERROR) {
        cerr << "Unable to close socket. Error: " << WSAGetLastError << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // WSACleanup
    WSACleanup();

    // Display info
    cout << "Server closed" << endl;
    sleep(5);
    exit(EXIT_SUCCESS);
}

void accept_client(SOCKET server_sock, sockaddr_in &client_addr, SOCKET &client_sock) {

    // Accept
    client_sock= accept(server_sock, (struct sockaddr *) &client_addr, NULL);
    if (client_sock == INVALID_SOCKET) {
        cerr << "Unable to accept. Error: " << WSAGetLastError << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }
}

DWORD WINAPI client_thread(LPVOID args) {
    struct client_data_t *data = (struct client_data_t *)args;

    EnterCriticalSection(&cs);

    if(connected_clients >= MAX_CONNECTED_CLIENTS) {
        LeaveCriticalSection(&cs);
        not_handle_client(data->client_addr, data->client_sock);
    }

    else {
        connected_clients++;
        LeaveCriticalSection(&cs);
        handle_client(data->client_addr, data->client_sock);

        EnterCriticalSection(&cs);
        connected_clients--;
        LeaveCriticalSection(&cs);
    }
}

void handle_client(sockaddr_in client_addr, SOCKET client_sock) {

    // Display info
    char *client_ip = inet_ntoa(client_addr.sin_addr);
    int client_port = client_addr.sin_port;
    cout << "Client " << client_ip << " connected on port " << client_port << " (" << connected_clients << " connected clients)" << endl;

    while (true) {

        // Receive
        char buffer[100];
        int received_bytes = recv(client_sock, buffer, 100, 0);
        if (received_bytes <= 0) break;

        // Display info
        string msg(buffer, received_bytes);
        cout << "Client " << client_ip << ":" << client_port << " SAYS: " << msg;
        cout << " (" << received_bytes << " bytes)" << endl;

        // Send
        int result = send(client_sock, buffer, received_bytes, 0);
        if (result == SOCKET_ERROR) break;

        if(!running) break;
    }

    // Close client socket
    close(client_sock);
    free(client_ip);

    // Display info
    cout << "Client " << client_ip << " on port " << client_port << " disconnected" << " (" << connected_clients-1 << " connected clients)" << endl;
}

void not_handle_client(sockaddr_in client_addr, SOCKET client_sock) {

    // Display info
    char *client_ip = inet_ntoa(client_addr.sin_addr);
    int client_port = client_addr.sin_port;
    cout << "Refused to handle client " << client_ip << " on port " << client_port << endl;

    // Send error message
    char message[] = "Unable to respond. Too many connections";
    send(client_sock, message, strlen(message), 0);

    // Close client socket
    close(client_sock);
    free(client_ip);
}