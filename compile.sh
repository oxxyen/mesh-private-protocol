#!/bin/bash

echo "🔧 Compiling Veil Secure Messenger for Arch Linux..."

# Проверка и установка зависимостей
echo "📦 Installing dependencies..."
sudo pacman -Sy --noconfirm gcc make json-c openssl mariadb-libs

# Компиляция сервера с максимальной безопасностью
echo "🔒 Building server with security flags..."
gcc -o veil_server server.c \
    -lssl -lcrypto -ljson-c -lmysqlclient -lpthread \
    -O2 -Wall -Wextra -Wpedantic \
    -D_FORTIFY_SOURCE=2 -fstack-protector-strong \
    -Wformat -Wformat-security -Werror=format-security \
    -fPIE -pie -Wl,-z,relro,-z,now

if [ $? -eq 0 ]; then
    echo "✅ Server compiled successfully!"
else
    echo "❌ Server compilation failed!"
    exit 1
fi

# Компиляция клиента
echo "💻 Building client..."
gcc -o veil_client client.c \
    -lssl -lcrypto -ljson-c -lpthread \
    -O2 -Wall -Wextra

if [ $? -eq 0 ]; then
    echo "✅ Client compiled successfully!"
else
    echo "❌ Client compilation failed!"
    exit 1
fi

# Проверка безопасности бинарников
echo "🔍 Security check..."
checksec --file=veil_server 2>/dev/null || echo "Install 'checksec' for detailed security report"
file veil_server veil_client

echo ""
echo "🎉 Build complete!"
echo ""
echo "🚀 Usage:"
echo "  Server: ./veil_server"
echo "  Client: ./veil_client 127.0.0.1 5555"
echo ""
echo "📋 Before first run:"
echo "  1. Set environment variables:"
echo "     export VEIL_DB_HOST=localhost"
echo "     export VEIL_DB_USER=veil_user" 
echo "     export VEIL_DB_PASS=your_password"
echo "     export VEIL_DB_NAME=veil_chat"
echo "  2. Or create .env file with these variables"