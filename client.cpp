/// client.cpp
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
using namespace std;

// Force 1-byte alignment for the custom header.

//a pragma is a preprocessor directive that provides additional information to the compiler.
//uint8_t is an unsigned integer type that is guaranteed to be 8 bits (1 byte) in size.
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

const int HEADER_SIZE = sizeof(CustomHeader);  // Should be 13 bytes because of 1-byte alignment.

// Function that will be used so that the client can set a custom header everytime.
int getValidatedFlag(const string &flagName) {
    int value;
    while (true) {
        cout << "Enter " << flagName << " (0 or 1): ";
        cin >> value;
        if (value == 0 || value == 1)
            break;
        cout << "Invalid input. " << flagName << " must be 0 or 1." << endl;
    }
    return value;
}

int main() {
    const char* serverIP = "127.0.0.1"; //home IP address
    const int serverPort = 8000;
    int sock = 0; 
    struct sockaddr_in serv_addr;
    
    // Create a socket.
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Socket creation error" << endl;
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP, &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid address / Address not supported" << endl;
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Connect to the server.
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection Failed" << endl;
        close(sock);
        exit(EXIT_FAILURE);
    }
    cout << "Connected to server " << serverIP << ":" << serverPort << endl;

    // Prompt the user for header values.
    uint16_t srcPort = 8000;
    
    uint16_t destPort = serverPort; // Destination port is the same as server port.
    
    uint32_t seqNum = 1; 
    
    
    // For the flag fields, use the helper function to ensure value is either 0 or 1.
    int ackFlag = getValidatedFlag("ACK flag");
    int synFlag = getValidatedFlag("SYN flag");
    int finFlag = getValidatedFlag("FIN flag");

    // Prepare payload.
    string payload;
    cout << "Enter The message for the server: ";
    cin.ignore(); // Clear newline leftover in input stream.
    getline(cin, payload);
    
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
        cerr << "There was an error sending the message" << endl;
        close(sock);
        exit(EXIT_FAILURE);
    }
    cout << "Sent message: header (" << HEADER_SIZE << " bytes) + payload (" 
              << payloadSize << " bytes)" << endl;
    
    // Receive and display the server's response.
    char responseBuffer[1024] = {0};
    ssize_t n = recv(sock, responseBuffer, sizeof(responseBuffer) - 1, 0);
    if (n > 0) {
        string response(responseBuffer, n);
        cout << "Server says: " << response << endl;
    } else {
        cout << "Nothing was returned or a fatal error occured." << endl;
    }
    
    close(sock);
    return 0;
}
