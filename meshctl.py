#!/usr/bin/env python3
"""
Mesh Messenger Control Script

Управление сервером Mesh: запуск, остановка, очистка, логи.
"""

import os
import sys
import argparse
import subprocess
import signal
import time
import shutil
from pathlib import Path

# Конфигурация
PROJECT_ROOT = Path(__file__).parent.resolve()
SERVER_BIN = PROJECT_ROOT / "mesh_server"
DB_PATH = PROJECT_ROOT / "database" / "mesh_db.sqlite"
PID_FILE = PROJECT_ROOT / "mesh_server.pid"
LOG_FILE = PROJECT_ROOT / "mesh_server.log"
CERT_FILE = PROJECT_ROOT / "cert.pem"
KEY_FILE = PROJECT_ROOT / "key.pem"

def ensure_directories():
    """Создаёт необходимые директории."""
    (PROJECT_ROOT / "database").mkdir(exist_ok=True)

def generate_self_signed_cert():
    """Генерирует самоподписанный TLS-сертификат, если отсутствует."""
    if not CERT_FILE.exists() or not KEY_FILE.exists():
        print("🔐 Генерация самоподписанного TLS-сертификата...")
        try:
            subprocess.run([
                "openssl", "req", "-x509", "-nodes", "-days", "365",
                "-newkey", "rsa:2048",
                "-keyout", str(KEY_FILE),
                "-out", str(CERT_FILE),
                "-subj", "/C=RU/ST=State/L=City/O=Mesh/CN=localhost"
            ], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            CERT_FILE.chmod(0o644)
            KEY_FILE.chmod(0o600)
            print("✅ Сертификат создан.")
        except Exception as e:
            print(f"❌ Ошибка генерации сертификата: {e}")
            sys.exit(1)

def is_server_running():
    """Проверяет, запущен ли сервер по PID-файлу."""
    if not PID_FILE.exists():
        return False
    try:
        with open(PID_FILE, 'r') as f:
            pid = int(f.read().strip())
        os.kill(pid, 0)  # Не отправляет сигнал, только проверяет существование
        return True
    except (OSError, ValueError):
        PID_FILE.unlink(missing_ok=True)
        return False

def get_server_pid():
    """Возвращает PID сервера или None."""
    if not PID_FILE.exists():
        return None
    try:
        with open(PID_FILE, 'r') as f:
            return int(f.read().strip())
    except (OSError, ValueError):
        return None

def start_server():
    """Запускает сервер в фоне."""
    if is_server_running():
        print("⚠️  Сервер уже запущен.")
        return

    ensure_directories()
    generate_self_signed_cert()

    if not SERVER_BIN.exists():
        print("❌ Бинарник сервера не найден. Соберите проект сначала:")
        print("   make")
        sys.exit(1)

    print("🚀 Запуск Mesh Server...")
    log_fd = open(LOG_FILE, "a")
    proc = subprocess.Popen(
        [str(SERVER_BIN)],
        stdout=log_fd,
        stderr=subprocess.STDOUT,
        cwd=PROJECT_ROOT,
        preexec_fn=os.setsid  # Создаём новую сессию
    )
    with open(PID_FILE, "w") as f:
        f.write(str(proc.pid))
    print(f"✅ Сервер запущен (PID: {proc.pid})")

def stop_server():
    """Останавливает сервер."""
    if not is_server_running():
        print("⚠️  Сервер не запущен.")
        return

    pid = get_server_pid()
    if pid:
        print(f"🛑 Остановка сервера (PID: {pid})...")
        try:
            os.kill(pid, signal.SIGINT)
            for _ in range(10):  # Ждём до 10 секунд
                if not is_server_running():
                    break
                time.sleep(1)
            else:
                print("⚠️  Принудительное завершение...")
                os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
    PID_FILE.unlink(missing_ok=True)
    print("✅ Сервер остановлен.")

def restart_server():
    """Перезапускает сервер."""
    stop_server()
    time.sleep(1)
    start_server()

def clean_project():
    """Очищает базу данных и логи."""
    print("🧹 Очистка проекта...")
    stop_server()
    time.sleep(1)

    # Удаляем БД
    if DB_PATH.exists():
        DB_PATH.unlink()
        print("✅ База данных удалена.")

    # Удаляем лог
    if LOG_FILE.exists():
        LOG_FILE.unlink()
        print("✅ Лог удалён.")

    # Удаляем PID
    PID_FILE.unlink(missing_ok=True)

    print("✨ Очистка завершена.")

def show_status():
    """Показывает статус сервера."""
    if is_server_running():
        pid = get_server_pid()
        print(f"🟢 Сервер запущен (PID: {pid})")
    else:
        print("🔴 Сервер остановлен")

def show_logs(n=20):
    """Показывает последние n строк лога."""
    if not LOG_FILE.exists():
        print("📝 Лог-файл отсутствует.")
        return

    with open(LOG_FILE, "r") as f:
        lines = f.readlines()
        for line in lines[-n:]:
            print(line, end="")

def main():
    parser = argparse.ArgumentParser(
        description="Управление Mesh Messenger Server",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog="""
Примеры:
  python3 meshctl.py --start
  python3 meshctl.py --status
  python3 meshctl.py --log 50
  python3 meshctl.py --clean
        """
    )
    parser.add_argument("--start", action="store_true", help="Запустить сервер")
    parser.add_argument("--stop", action="store_true", help="Остановить сервер")
    parser.add_argument("--restart", action="store_true", help="Перезапустить сервер")
    parser.add_argument("--status", action="store_true", help="Показать статус")
    parser.add_argument("--clean", action="store_true", help="Очистить БД и логи")
    parser.add_argument("--log", nargs="?", const=20, type=int, metavar="N",
                        help="Показать последние N строк лога (по умолчанию: 20)")
    parser.add_argument("--version", action="version", version="Mesh Messenger Control v1.0")

    args = parser.parse_args()

    if not any([args.start, args.stop, args.restart, args.status, args.clean, args.log is not None]):
        parser.print_help()
        sys.exit(0)

    if args.start:
        start_server()
    elif args.stop:
        stop_server()
    elif args.restart:
        restart_server()
    elif args.status:
        show_status()
    elif args.clean:
        clean_project()
    elif args.log is not None:
        show_logs(args.log)

if __name__ == "__main__":
    main()