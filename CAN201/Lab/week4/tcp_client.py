# client side
from socket import *
server_hostname = '127.0.0.1'
server_port = 12000
client_socket = socket(AF_INET, SOCK_STREAM)
client_socket.connect((server_hostname, server_port))
sentence = input('Input a sentence:')
client_socket.send(sentence.encode())
modified_message = client_socket.recv(20480)
print(modified_message)
client_socket.close()