from socket import *

client_socket = socket(AF_INET, SOCK_DGRAM)

msg = input('Enter message: ')
client_socket.sendto(msg.encode(), ('127.0.0.1', 20000))

new_msg , address = client_socket.recvfrom(1024)
print(new_msg.decode())
print(address)
client_socket.close()