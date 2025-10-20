from socket import *

server_socket = socket(AF_INET, SOCK_DGRAM)

server_socket.bind(('', 20000))

while True:
    msg, address = server_socket.recvfrom(1024)
    print(address)
    msg = msg.decode()
    modified_msg = msg.upper()
    server_socket.sendto(modified_msg.encode(), address)