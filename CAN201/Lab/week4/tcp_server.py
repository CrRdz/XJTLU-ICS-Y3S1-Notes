# server side
from socket import *
server_port = 12000
server_socket = socket(AF_INET, SOCK_STREAM)
server_socket.bind(('', server_port))
server_socket.listen(10)
print('The TCP server is listening')
while True:
    connection_socket, addr = server_socket.accept()
    sentence = connection_socket.recv(20480).decode()
    print(sentence)
    capitalized_sentence = sentence.upper()
    connection_socket.send(capitalized_sentence.encode())