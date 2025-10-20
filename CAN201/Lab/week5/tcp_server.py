from socket import *

server_socket = socket(AF_INET, SOCK_STREAM)
server_socket.bind(('', 20000))

server_socket.listen(5)

while True:
    connection_socket, client_address = server_socket.accept()
    print(client_address)
    msg = connection_socket.recv(1024)
    msg = msg.decode()
    msg = msg.upper()

    connection_socket.send(msg.encode())

    connection_socket.close()