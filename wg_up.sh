#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Использование: $0 <интерфейс> <файл_конфигурации>"
    echo "Пример: $0 wg0 ./client.conf"
    exit 1
fi

IFACE=$1
CONF_FILE=$2

if [ ! -f "$CONF_FILE" ]; then
    echo "Ошибка: Файл конфигурации $CONF_FILE не найден."
    exit 1
fi

# 1. Извлекаем Address, MTU и Endpoint (wg setconf игнорирует Address/MTU, нам надо настроить их самим)
ADDRESS=$(grep -i '^Address' "$CONF_FILE" | cut -d'=' -f2 | tr -d ' ')
MTU=$(grep -i '^MTU' "$CONF_FILE" | cut -d'=' -f2 | tr -d ' ')
ENDPOINT=$(grep -i '^Endpoint' "$CONF_FILE" | cut -d'=' -f2 | tr -d ' ')
ALLOWED_IPS=$(grep -i '^AllowedIPs' "$CONF_FILE" | cut -d'=' -f2 | tr -d ' ')

MTU=${MTU:-1280}

# Скрываем баннер-предупреждение от wireguard-go
export WG_I_PREFER_BUGGY_USERSPACE_TO_POLISHED_KMOD=1

echo "1. Запуск демона wireguard-go для интерфейса $IFACE..."
./wireguard-go "$IFACE"

echo "2. Настройка IP-адреса и MTU ($MTU)..."
ip address add "$ADDRESS" dev "$IFACE"
ip link set mtu "$MTU" up dev "$IFACE"

echo "3. Применение криптографии и пиров (через setconf)..."
# Создаем временный файл конфига, очищенный от системных параметров (Address и MTU)
TMP_CONF=$(mktemp)
grep -v -i -E '^\s*(Address|MTU)\s*=' "$CONF_FILE" > "$TMP_CONF"

# Применяем "чистый" конфиг и сразу удаляем временный файл
./wg setconf "$IFACE" "$TMP_CONF"
rm -f "$TMP_CONF"

if [ -n "$ENDPOINT" ]; then
    echo "4. Добавление маршрута в туннель..."
    FIRST_ROUTE=$(echo "$ALLOWED_IPS" | cut -d',' -f1)
    
    # Используем replace, чтобы не было ошибки "File exists", 
    # если ядро уже добавило маршрут автоматически.
    ip route replace "$FIRST_ROUTE" dev "$IFACE"
fi

echo "Туннель $IFACE успешно поднят!"