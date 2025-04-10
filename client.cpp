// client.cpp
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Force 1-byte alignment
#pragma pack(push, 1)
struct CustomHeader {
    uint16_t srcPort;     // Source Port (2 bytes)
    uint16_t destPort;    // Destination Port (2 bytes)
    uint32_t seqNum;      // Sequence Number (4 bytes)
    uint8_t  ackFlag;     // ACK Flag (1 byte)
    uint8_t  synFlag;     // SYN Flag (1 byte)
    uint8_t  finFlag;     // FIN Flag (1 byte)
    uint16_t payloadSize; // Payload Size (2 bytes)
};
#pragma pack(pop)

const int HEADER_SIZE = sizeof(CustomHeader);  // Should be 13 bytes

int main() {
    const char* serverIP = "127.0.0.1";
    const int serverPort = 12345;
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    // Create socket.
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported" << std::endl;
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Connect to the server.
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        close(sock);
        exit(EXIT_FAILURE);
    }
    std::cout << "Connected to server " << serverIP << ":" << serverPort << std::endl;
    
    // Prepare header values.
    CustomHeader header;
    header.srcPort = htons(5000);         // Arbitrary client port
    header.destPort = htons(serverPort);    // Server's listening port
    header.seqNum = htonl(1);               // Example sequence number
    
    // Set flag values for testing.
    // For example, set SYN flag to 1 (and ACK, FIN to 0)
    header.ackFlag = 0;
    header.synFlag = 1;
    header.finFlag = 0;
    
    // Prepare payload.
    std::string payload = "Hello from client";
    header.payloadSize = htons(payload.size());
    
    // Send header and payload as a single message.
    // Note: Since our header is already in network order where applicable, we can send it directly.
    if (send(sock, &header, HEADER_SIZE, 0) != HEADER_SIZE) {
        std::cerr << "Failed to send header" << std::endl;
        close(sock);
        exit(EXIT_FAILURE);
    }
    if (!payload.empty()) {
        if (send(sock, payload.c_str(), payload.size(), 0) != (ssize_t)payload.size()) {
            std::cerr << "Failed to send payload" << std::endl;
            close(sock);
            exit(EXIT_FAILURE);
        }
    }
    std::cout << "Sent message: header (" << HEADER_SIZE << " bytes) + payload (" 
              << payload.size() << " bytes)" << std::endl;
    
    // Receive and display the server's response.
    char buffer[1024] = {0};
    ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        std::string response(buffer, n);
        std::cout << "Server response: " << response << std::endl;
    } else {
        std::cout << "No response received or error occurred." << std::endl;
    }
    
    close(sock);
    return 0;
}
