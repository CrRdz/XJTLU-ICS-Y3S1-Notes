from socket import *

client = socket(AF_INET, SOCK_STREAM)

client.connect(('127.0.0.1', 20000))

msg = input("Enter a message: ")
client.send(msg.encode())

msg = client.recv(1024)

print("Received from server:", msg.decode())

client.close()