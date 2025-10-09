
import socket
import ssl
context = ssl.create_default_context()
context.check_hostname = False
context.verify_mode = ssl.CERT_NONE
with socket.create_connection(("localhost", 5555)) as sock:
    with context.wrap_socket(sock, server_hostname="localhost") as ssock:
        ssock.send(b"/help\n")
        print(ssock.recv(1024).decode())
