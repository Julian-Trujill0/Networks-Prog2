/// client.cpp
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Force 1-byte alignment for the custom header.
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

// A helper function to get and validate a binary flag (0 or 1)
int getValidatedFlag(const std::string &flagName) {
    int value;
    while (true) {
        std::cout << "Enter " << flagName << " (0 or 1): ";
        std::cin >> value;
        if (value == 0 || value == 1)
            break;
        std::cout << "Invalid input. " << flagName << " must be 0 or 1." << std::endl;
    }
    return value;
}

int main() {
    const char* serverIP = "127.0.0.1";
    const int serverPort = 12345;
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    // Create a socket.
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

    // Prompt the user for header values.
    uint16_t srcPort;
    std::cout << "Enter source port (client-side port number): ";
    std::cin >> srcPort;
    
    // destination port is assumed to be the server's listening port.
    uint16_t destPort = serverPort;
    
    uint32_t seqNum;
    std::cout << "Enter sequence number: ";
    std::cin >> seqNum;
    
    // For the flag fields, use the helper function to ensure value is either 0 or 1.
    int ackFlag = getValidatedFlag("ACK flag");
    int synFlag = getValidatedFlag("SYN flag");
    int finFlag = getValidatedFlag("FIN flag");

    // Prepare payload.
    std::string payload;
    std::cout << "Enter payload (as a string): ";
    std::cin.ignore(); // Clear newline leftover in input stream.
    std::getline(std::cin, payload);
    
    uint16_t payloadSize = payload.size();
    
    // Prepare the header. Convert multi-byte fields to network byte order.
    CustomHeader header;
    header.srcPort = htons(srcPort);
    header.destPort = htons(destPort);
    header.seqNum = htonl(seqNum);
    header.ackFlag = ackFlag;
    header.synFlag = synFlag;
    header.finFlag = finFlag;
    header.payloadSize = htons(payloadSize);
    
    // Combine header and payload.
    char buffer[HEADER_SIZE + payload.size()];
    memcpy(buffer, &header, HEADER_SIZE);
    memcpy(buffer + HEADER_SIZE, payload.c_str(), payloadSize);
    
    // Send header + payload in one message.
    ssize_t totalSize = HEADER_SIZE + payloadSize;
    if (send(sock, buffer, totalSize, 0) != totalSize) {
        std::cerr << "Failed to send complete message" << std::endl;
        close(sock);
        exit(EXIT_FAILURE);
    }
    std::cout << "Sent message: header (" << HEADER_SIZE << " bytes) + payload (" 
              << payloadSize << " bytes)" << std::endl;
    
    // Receive and display the server's response.
    char responseBuffer[1024] = {0};
    ssize_t n = recv(sock, responseBuffer, sizeof(responseBuffer) - 1, 0);
    if (n > 0) {
        std::string response(responseBuffer, n);
        std::cout << "Server response: " << response << std::endl;
    } else {
        std::cout << "No response received or an error occurred." << std::endl;
    }
    
    close(sock);
    return 0;
}
