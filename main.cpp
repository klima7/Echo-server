#include <iostream>
#include <winsock2.h>
#include <cstdlib>
#include <unistd.h>
#include <windows.h>

#define DEFAULT_PORT 7

using namespace std;

int main(int argc, char **argv) {

    int port = DEFAULT_PORT;

    // Get port
    if(argc > 1) {
        port = atoi(argv[1]);
        if(port < 0 || port > 65535) {
            cerr << "Invalid port number supplied" << endl;
            return 1;
        }
    }
    cout << "Server is listening on port " << port << endl;

    // WSAStartup
    WSADATA wsadata;
    int result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result != 0) {
        cerr << "Unable to initialize winsock. Error: " << WSAGetLastError() << endl;
        return EXIT_FAILURE;
    }

    // Socket
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        cerr << "Unable to create socket. Error: " << WSAGetLastError() << endl;
        WSACleanup();
        return EXIT_FAILURE;
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
        return EXIT_FAILURE;
    }

    // Listen
    result = listen(server_sock, 10);
    if(result == SOCKET_ERROR) {
        cerr << "Unable to listen. Error: " << WSAGetLastError << endl;
        WSACleanup();
        return EXIT_FAILURE;
    }

    while(true) {

        // Accept
        cout << "Waiting to accept client" << endl;
        struct sockaddr_in client_addr;
        int addrlen;
        SOCKET client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &addrlen);
        if (result == INVALID_SOCKET) {
            cerr << "Unable to accept. Error: " << WSAGetLastError << endl;
            WSACleanup();
            return EXIT_FAILURE;
        }

        // Display client info
        char *client_ip = inet_ntoa(client_addr.sin_addr);
        int client_port = client_addr.sin_port;
        cout << "Client " << client_ip << " connected on port " << client_port << endl;

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
            result = send(client_sock, buffer, received_bytes, 0);
            if (result == SOCKET_ERROR) break;
        }

        // Display info
        cout << "Client " << client_ip << " on port " << client_port << " disconnected" << endl;

        close(client_sock);
        free(client_ip);
    }

    // Close
    result = closesocket(server_sock);
    if(result == SOCKET_ERROR) {
        cerr << "Unable to close socket. Error: " << WSAGetLastError << endl;
        WSACleanup();
        return EXIT_FAILURE;
    }

    // WSACleanup
    WSACleanup();
    return EXIT_SUCCESS;
}
