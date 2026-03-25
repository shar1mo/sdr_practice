#!/usr/bin/env bash

PLAYBOOK_PATH="ansible_install.yml"
INVENTORY_FILE="inventory.ini"
INSTALL_PATH="$(pwd)"
INSTALL_USER=$(whoami)

# Создание директории 
echo "Создание директории ..."
echo "$INSTALL_PATH"
echo "$INSTALL_USER"

# Установка Ansible
echo "Установка Ansible..."
if ! command -v ansible &> /dev/null; then
    sudo apt update
    sudo apt install -y ansible
else
    echo "Ansible уже установлен."
fi

# Запуск playbook
echo "Запуск playbook..."
ansible-playbook -i "$INVENTORY_FILE" "$PLAYBOOK_PATH" --ask-become-pass --extra-vars "install_path=$INSTALL_PATH install_user=$INSTALL_USER debug_mode=true"

echo "Завершен!"