# test_client.py
import socket
import ssl
import sys

def main():
    context = ssl.create_default_context()
    context.check_hostname = False
    context.verify_mode = ssl.CERT_NONE  # самоподписанный сертификат

    with socket.create_connection(("localhost", 5555)) as sock:
        with context.wrap_socket(sock, server_hostname="localhost") as ssock:
            print("Подключено к серверу. Введите команды (/register, /login и т.д.):")
            try:
                while True:
                    # Читаем ответ сервера (асинхронно)
                    data = ssock.recv(4096)
                    if data:
                        print(data.decode('utf-8', errors='replace'), end='')

                    # Ввод от пользователя
                    try:
                        msg = input()
                        if msg.lower() in ('exit', 'quit'):
                            break
                        ssock.sendall((msg + '\n').encode('utf-8'))
                    except (EOFError, KeyboardInterrupt):
                        break
            except Exception as e:
                print("Ошибка:", e)

if __name__ == "__main__":
    main()