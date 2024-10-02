import socket

def test_echo_server():
    # Define server address and port
    server_address = 'localhost'
    server_port = 1234

    # Create a TCP/IP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        # Connect to the server
        sock.connect((server_address, server_port))
        print(f"Connected to {server_address} on port {server_port}")

        # # Send a message
        message = "Hello, Echo Server!\n"
        print(f"Sending: {message.strip()}")
        sock.sendall(message.encode())

        # Receive the echoed message
        received = sock.recv(1024)
        print(f"Received: {received.decode().strip()}")

    finally:
        # Close the socket
        sock.close()
        print("Connection closed")

if __name__ == "__main__":
    test_echo_server()