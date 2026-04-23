#!/bin/bash

# Проверка наличия утилиты wg
if [ ! -x "./wg" ]; then
    echo "Ошибка: Исполняемый файл ./wg не найден в текущей директории."
    exit 1
fi

echo "Генерация ключей..."
SERVER_PRIV=$(./wg genkey)
SERVER_PUB=$(echo "$SERVER_PRIV" | ./wg pubkey)

CLIENT_PRIV=$(./wg genkey)
CLIENT_PUB=$(echo "$CLIENT_PRIV" | ./wg pubkey)

echo "Создание server.conf..."
cat > server.conf <<EOF
[Interface]
Address = 10.0.0.1/24
ListenPort = 51820
PrivateKey = $SERVER_PRIV
MTU = 1280

[Peer]
PublicKey = $CLIENT_PUB
AllowedIPs = 10.0.0.2/32
EOF

echo "Создание client.conf..."
cat > client.conf <<EOF
[Interface]
Address = 10.0.0.2/24
PrivateKey = $CLIENT_PRIV
MTU = 1280

[Peer]
PublicKey = $SERVER_PUB
Endpoint = 127.0.0.1:51820
AllowedIPs = 10.0.0.0/24
PersistentKeepalive = 25
EOF

echo "Готово! Файлы server.conf и client.conf успешно созданы."