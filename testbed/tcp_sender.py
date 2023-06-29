from socket import *
data = "a" * 5000
clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect(('', 23333))
while True:
    clientSocket.sendall(data.encode())
    # print(len(message))