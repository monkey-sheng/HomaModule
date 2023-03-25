from socket import *
serverSocket = socket(AF_INET, SOCK_DGRAM)
serverSocket.bind(('', 2333))
while True:
    message, address = serverSocket.recvfrom(1)
    print(address)