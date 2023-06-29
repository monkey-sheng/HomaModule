from socket import *
serverSocket = socket(AF_INET, SOCK_DGRAM)
serverSocket.bind(('', 23333))
while True:
    message, address = serverSocket.recvfrom(10000)
    # print(len(message))