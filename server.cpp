// server.cpp
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
using namespace std;

// Force 1-byte alignment 
#pragma pack(push, 1)
struct CustomHeader {
    uint16_t srcPort;     // Source Port (2 bytes)
    uint16_t destPort;    // Dest Port (2 bytes)
    uint32_t seqNum;      // Sequence Number (4 bytes)
    uint8_t  ackFlag;     // ACK Flag (1 byte)
    uint8_t  synFlag;     // SYN Flag (1 byte)
    uint8_t  finFlag;     // FIN Flag (1 byte)
    uint16_t payloadSize; // Payload Size (2 bytes)
};
#pragma pack(pop)

const int HEADER_SIZE = sizeof(CustomHeader); 

ssize_t recv_all(int sock, char *buffer, size_t length) {
    size_t total = 0;
    while(total < length) {
        ssize_t bytes = recv(sock, buffer + total, length - total, 0);
        if(bytes <= 0) {
            return bytes; // Error or connection closed
        }
        total += bytes;
    }
    return total;
}

string get_response(const CustomHeader &header) {
    // Determine response according to assignment logic
    if (header.synFlag == 1) {
        return "SYN received - connection initiated";
    } else if (header.ackFlag == 1) {
        return "ACK received - message acknowledged";
    } else if (header.finFlag == 1) {
        return "FIN received - connection closing";
    } else {
        return "Data received - payload length: " + to_string(ntohs(header.payloadSize));
    }
}

int main() {
    const int serverPort = 8000;
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Allow reuse of address/port
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Bind to all interfaces at serverPort
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(serverPort);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Listen for incoming connection
    if (listen(server_fd, 1) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    cout << "Server listening on port " << serverPort << endl;
    
    // Accept a single connection.
    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    cout << "Client connected at " << inet_ntoa(address.sin_addr) << endl;
    
    while (true) {
        CustomHeader header;
        // Receive the header 
        ssize_t n = recv_all(client_fd, reinterpret_cast<char*>(&header), HEADER_SIZE);
        if(n <= 0) {
            cout << "Client disconnected or error in receiving header." << endl;
            break;
        }
        
        header.srcPort = ntohs(header.srcPort);
        header.destPort = ntohs(header.destPort);
        header.seqNum  = ntohl(header.seqNum);
        header.payloadSize = ntohs(header.payloadSize);

        cout << "Received Header:" << endl;
        cout << "  Source Port: "  << header.srcPort << endl;
        cout << "  Dest Port: "    << header.destPort << endl;
        cout << "  Sequence No: "  << header.seqNum << endl;
        cout << "  ACK Flag: "     << static_cast<int>(header.ackFlag) << endl;
        cout << "  SYN Flag: "     << static_cast<int>(header.synFlag) << endl;
        cout << "  FIN Flag: "     << static_cast<int>(header.finFlag) << endl;
        cout << "  Payload Size: " << header.payloadSize << endl;
        
        // Receive the payload
        string payload;
        if (header.payloadSize > 0) {
            char *payloadBuffer = new char[header.payloadSize];
            n = recv_all(client_fd, payloadBuffer, header.payloadSize);
            if(n <= 0) {
                cout << "Error receiving payload or client disconnected." << endl;
                delete[] payloadBuffer;
                break;
            }
            payload.assign(payloadBuffer, header.payloadSize);
            delete[] payloadBuffer;
            cout << "Client Says"< endl;
        }
        
        // Response based on header flags.
        string response = get_response(header);
        
        // Send the response
        if (send(client_fd, response.c_str(), response.size(), 0) == -1) {
            perror("send");
            break;
        }
        cout << "Sent response: " << response << endl;
        
        // If FIN flag is set, close the connection.
        if (header.finFlag == 1) {
            cout << "FIN flag received. Closing connection." << endl;
            break;
        }
    }
    
    close(client_fd);
    close(server_fd);
    return 0;
}
