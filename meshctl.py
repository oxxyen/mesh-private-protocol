#!/usr/bin/env python3
"""
MESH Messenger Control Utility

Управление сервером MESH: сборка, запуск, тестирование, мониторинг.
"""

import os
import sys
import argparse
import subprocess
import signal
import time
import shutil
from pathlib import Path
from typing import Optional

# Цвета для терминала
class Colors:
    RESET = "\033[0m"
    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    MAGENTA = "\033[35m"
    CYAN = "\033[36m"
    BOLD = "\033[1m"

def cprint(text: str, color: str = Colors.RESET, bold: bool = False):
    style = Colors.BOLD if bold else ""
    print(f"{style}{color}{text}{Colors.RESET}")

def spinner(text: str, duration: float = 1.0):
    """Простая анимация загрузки."""
    chars = "|/-\\"
    end_time = time.time() + duration
    i = 0
    while time.time() < end_time:
        sys.stdout.write(f"\r{Colors.CYAN}{text} {chars[i % len(chars)]}{Colors.RESET}")
        sys.stdout.flush()
        time.sleep(0.1)
        i += 1
    sys.stdout.write("\r" + " " * (len(text) + 4) + "\r")

# ASCII ART
def show_mesh_ascii():
    mesh_ascii = f"""
{Colors.MAGENTA}{Colors.BOLD}
███╗   ███╗███████╗███████╗██╗  ██╗      █████╗ ███╗   ██╗███████╗ ██████╗██╗
████╗ ████║██╔════╝██╔════╝██║  ██║     ██╔══██╗████╗  ██║██╔════╝██╔════╝██║
██╔████╔██║█████╗  ███████╗███████║     ███████║██╔██╗ ██║███████╗██║     ██║
██║╚██╔╝██║██╔══╝  ╚════██║██╔══██║     ██╔══██║██║╚██╗██║╚════██║██║     ██║
██║ ╚═╝ ██║███████╗███████║██║  ██║     ██║  ██║██║ ╚████║███████║╚██████╗██║
╚═╝     ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝ ╚═════╝╚═╝
{Colors.RESET}
{Colors.CYAN}          MESSENGER WITH PRIVATE SIGNAL PROTOCOL - CONTROL UTILITY v3.0{Colors.RESET}
    """
    print(mesh_ascii)

def detect_distro():
    """Определяет дистрибутив Linux"""
    try:
        with open('/etc/os-release', 'r') as f:
            content = f.read().lower()
            if 'arch' in content:
                return 'arch'
            elif 'ubuntu' in content or 'debian' in content:
                return 'ubuntu'
            else:
                return 'unknown'
    except:
        return 'unknown'

def install_signal_protocol(distro: str = None):
    """Установка libsignal-protocol-c для указанного дистрибутива"""
    if distro is None:
        distro = detect_distro()
    
    cprint(f"📦 Установка libsignal-protocol-c для {distro}...", Colors.CYAN)
    
    if distro == "arch":
        if shutil.which("yay"):
            cmd = ["yay", "-S", "--noconfirm", "libsignal-protocol-c-asamk"]
        elif shutil.which("paru"):
            cmd = ["paru", "-S", "--noconfirm", "libsignal-protocol-c-asamk"]
        else:
            cprint("⚠️  Менеджер AUR (yay/paru) не найден. Установите вручную:", Colors.YELLOW)
            cprint("    yay -S libsignal-protocol-c-asamk", Colors.YELLOW)
            return False
    elif distro in ("ubuntu", "debian"):
        if not shutil.which("git") or not shutil.which("cmake"):
            cprint("🔧 Установка зависимостей для сборки...", Colors.CYAN)
            subprocess.run(["sudo", "apt", "update"], check=True)
            subprocess.run(["sudo", "apt", "install", "-y", "git", "cmake", "build-essential", "libssl-dev", "libsqlite3-dev"], check=True)
        
        build_dir = Path.home() / "libsignal-build"
        build_dir.mkdir(exist_ok=True)
        os.chdir(build_dir)
        
        if not (build_dir / "libsignal-protocol-c").exists():
            subprocess.run(["git", "clone", "https://github.com/AsamK/libsignal-protocol-c.git"], check=True)
        
        os.chdir("libsignal-protocol-c")
        subprocess.run(["git", "checkout", "v2.3.1"], check=True)
        
        build_sub = build_dir / "build"
        build_sub.mkdir(exist_ok=True)
        os.chdir(build_sub)
        subprocess.run(["cmake", "..", "-DCMAKE_BUILD_TYPE=Release"], check=True)
        subprocess.run(["make", "-j4"], check=True)
        subprocess.run(["sudo", "make", "install"], check=True)
        subprocess.run(["sudo", "ldconfig"], check=True)
        os.chdir("/")
        return True
    else:
        cprint(f"❌ Не поддерживаемая ОС: {distro}", Colors.RED)
        cprint("   Используйте --install-signal -u для Ubuntu/Debian", Colors.YELLOW)
        cprint("   Используйте --install-signal -arch для Arch Linux", Colors.YELLOW)
        return False

    try:
        subprocess.run(cmd, check=True)
        cprint("✅ libsignal-protocol-c установлен.", Colors.GREEN)
        return True
    except Exception as e:
        cprint(f"❌ Ошибка установки: {e}", Colors.RED)
        return False

def git_update():
    """Обновление репозитория на GitHub"""
    cprint("🔄 Обновление репозитория на GitHub...", Colors.CYAN)
    
    try:
        # Проверяем, есть ли изменения
        result = subprocess.run(["git", "status", "--porcelain"], 
                              capture_output=True, text=True, check=True)
        
        if not result.stdout.strip():
            cprint("✅ Нет изменений для коммита.", Colors.GREEN)
            return True
            
        # Добавляем все файлы
        cprint("📁 Добавление файлов...", Colors.CYAN)
        if not run_command(["git", "add", "."], silent=True):
            cprint("❌ Ошибка при добавлении файлов", Colors.RED)
            return False
            
        # Создаем коммит
        cprint("💾 Создание коммита...", Colors.CYAN)
        commit_msg = f"MESH Update {time.strftime('%Y-%m-%d %H:%M:%S')}"
        if not run_command(["git", "commit", "-m", commit_msg], silent=True):
            cprint("❌ Ошибка при создании коммита", Colors.RED)
            return False
            
        # Пушим изменения
        cprint("🚀 Отправка изменений на GitHub...", Colors.CYAN)
        if not run_command(["git", "push", "origin", "main"], silent=False):
            cprint("❌ Ошибка при отправке на GitHub", Colors.RED)
            return False
            
        cprint("✅ Репозиторий успешно обновлен на GitHub!", Colors.GREEN, bold=True)
        return True
        
    except Exception as e:
        cprint(f"❌ Ошибка при работе с Git: {e}", Colors.RED)
        return False

# Конфигурация
PROJECT_ROOT = Path(__file__).parent.resolve()
BUILD_DIR = PROJECT_ROOT / "build"
SERVER_BIN = BUILD_DIR / "mesh_server"
CMAKEFILE = PROJECT_ROOT / "CMakeLists.txt"
DB_PATH = PROJECT_ROOT / "database" / "mesh_db.sqlite"
PID_FILE = PROJECT_ROOT / "mesh_server.pid"
LOG_FILE = PROJECT_ROOT / "mesh_server.log"
CERT_FILE = PROJECT_ROOT / "cert.pem"
KEY_FILE = PROJECT_ROOT / "key.pem"
BACKUP_DIR = PROJECT_ROOT / "backups"

def ensure_directories():
    (PROJECT_ROOT / "database").mkdir(exist_ok=True)
    BACKUP_DIR.mkdir(exist_ok=True)

def run_command(cmd: list, cwd: Optional[Path] = None, silent: bool = False) -> bool:
    """Выполняет команду и возвращает успех."""
    try:
        result = subprocess.run(cmd, cwd=cwd or PROJECT_ROOT, 
                              capture_output=not silent, text=True)
        if not silent and result.returncode != 0:
            cprint(result.stderr, Colors.RED)
        return result.returncode == 0
    except Exception as e:
        if not silent:
            cprint(f"❌ Ошибка выполнения: {e}", Colors.RED)
        return False

def generate_self_signed_cert():
    """Генерирует TLS-сертификат."""
    if CERT_FILE.exists() and KEY_FILE.exists():
        return True

    cprint("🔐 Генерация самоподписанного TLS-сертификата...", Colors.CYAN)
    cmd = [
        "openssl", "req", "-x509", "-nodes", "-days", "365",
        "-newkey", "rsa:2048",
        "-keyout", str(KEY_FILE),
        "-out", str(CERT_FILE),
        "-subj", "/C=RU/ST=State/L=City/O=MESH/CN=localhost"
    ]
    success = run_command(cmd, silent=True)
    if success:
        CERT_FILE.chmod(0o644)
        KEY_FILE.chmod(0o600)
    return success

def is_server_running() -> bool:
    if not PID_FILE.exists():
        return False
    try:
        with open(PID_FILE, 'r') as f:
            pid = int(f.read().strip())
        os.kill(pid, 0)
        return True
    except (OSError, ValueError):
        PID_FILE.unlink(missing_ok=True)
        return False

def get_server_pid() -> Optional[int]:
    if not PID_FILE.exists():
        return None
    try:
        with open(PID_FILE, 'r') as f:
            return int(f.read().strip())
    except (OSError, ValueError):
        return None

def start_server():
    if is_server_running():
        cprint("⚠️  Сервер уже запущен.", Colors.YELLOW)
        return

    ensure_directories()
    if not generate_self_signed_cert():
        cprint("❌ Не удалось создать сертификат. Установите OpenSSL.", Colors.RED)
        return

    if not SERVER_BIN.exists():
        cprint("❌ Бинарник не найден. Соберите проект: ./meshctl.py --make", Colors.RED)
        return

    cprint("🚀 Запуск MESH Server...", Colors.CYAN, bold=True)
    log_fd = open(LOG_FILE, "a")
    proc = subprocess.Popen(
        [str(SERVER_BIN)],
        stdout=log_fd,
        stderr=subprocess.STDOUT,
        cwd=PROJECT_ROOT,
        preexec_fn=os.setsid
    )
    with open(PID_FILE, "w") as f:
        f.write(str(proc.pid))
    cprint(f"✅ Сервер запущен (PID: {proc.pid})", Colors.GREEN, bold=True)

def stop_server():
    if not is_server_running():
        cprint("⚠️  Сервер не запущен.", Colors.YELLOW)
        return

    pid = get_server_pid()
    if pid:
        cprint(f"🛑 Остановка сервера (PID: {pid})...", Colors.YELLOW)
        try:
            os.kill(pid, signal.SIGINT)
            for _ in range(10):
                if not is_server_running():
                    break
                time.sleep(1)
            else:
                cprint("⚠️  Принудительное завершение...", Colors.YELLOW)
                os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
    PID_FILE.unlink(missing_ok=True)
    cprint("✅ Сервер остановлен.", Colors.GREEN)

def make_server():
    cprint("🔨 Сборка MESH Server через CMake...", Colors.CYAN, bold=True)
    if not CMAKEFILE.exists():
        cprint("❌ CMakeLists.txt не найден.", Colors.RED)
        return

    # Создаём build директорию
    BUILD_DIR.mkdir(exist_ok=True)

    # Выполняем cmake
    if run_command(["cmake", ".."], cwd=BUILD_DIR, silent=True):
        cprint("✅ CMake конфигурация успешна.", Colors.GREEN)
    else:
        cprint("❌ Ошибка конфигурации CMake.", Colors.RED)
        return

    # Получаем количество ядер
    try:
        import multiprocessing
        jobs = multiprocessing.cpu_count()
    except:
        jobs = 4

    # Собираем с правильным -j
    if run_command(["make", f"-j{jobs}"], cwd=BUILD_DIR, silent=False):
        cprint("✅ Сборка успешна!", Colors.GREEN, bold=True)
    else:
        cprint("❌ Ошибка сборки. Убедитесь, что установлены зависимости:", Colors.RED, bold=True)
        cprint("   sudo pacman -S gcc cmake make sqlite openssl", Colors.YELLOW)
        cprint("   yay -S libsignal-protocol-c-asamk", Colors.YELLOW)

def clean_project():
    cprint("🧹 Полная очистка проекта...", Colors.YELLOW)
    stop_server()
    time.sleep(1)

    to_remove = [DB_PATH, LOG_FILE, PID_FILE, CERT_FILE, KEY_FILE]
    for path in to_remove:
        if path.exists():
            path.unlink()
            cprint(f"  Удалён: {path.name}", Colors.GREEN)

    # Удаляем build
    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
        cprint("  Удалена папка build", Colors.GREEN)

    cprint("✨ Очистка завершена.", Colors.GREEN)

def show_status():
    if is_server_running():
        pid = get_server_pid()
        cprint(f"🟢 Сервер запущен (PID: {pid})", Colors.GREEN, bold=True)
    else:
        cprint("🔴 Сервер остановлен", Colors.RED, bold=True)

def show_logs(n: int = 20):
    if not LOG_FILE.exists():
        cprint("📝 Лог-файл отсутствует.", Colors.YELLOW)
        return

    with open(LOG_FILE, "r") as f:
        lines = f.readlines()
        cprint(f"📄 Последние {min(n, len(lines))} строк лога:", Colors.CYAN, bold=True)
        for line in lines[-n:]:
            print(line, end="")

def rebuild():
    clean_project()
    time.sleep(1)
    make_server()

def check_dependencies():
    deps = {
        "gcc": ["gcc", "--version"],
        "cmake": ["cmake", "--version"],
        "make": ["make", "--version"],
        "sqlite3": ["sqlite3", "--version"],
        "openssl": ["openssl", "version"],
        "libsignal-protocol-c": ["pkg-config", "--exists", "libsignal-protocol-c"]
    }
    cprint("🔍 Проверка зависимостей...", Colors.CYAN)
    missing = []
    for name, cmd in deps.items():
        if run_command(cmd, silent=True):
            cprint(f"  ✅ {name}", Colors.GREEN)
        else:
            cprint(f"  ❌ {name}", Colors.RED)
            missing.append(name)
    
    if missing:
        cprint("\n⚠️  Отсутствуют зависимости. Установите:", Colors.YELLOW)
        cprint("   sudo pacman -S gcc cmake make sqlite openssl", Colors.YELLOW)
        cprint("   yay -S libsignal-protocol-c-asamk", Colors.YELLOW)
    else:
        cprint("✨ Все зависимости установлены.", Colors.GREEN)

def backup_db():
    if not DB_PATH.exists():
        cprint("❌ База данных не найдена.", Colors.RED)
        return

    timestamp = time.strftime("%Y%m%d_%H%M%S")
    backup_file = BACKUP_DIR / f"mesh_db_{timestamp}.sqlite"
    shutil.copy2(DB_PATH, backup_file)
    cprint(f"✅ Резервная копия создана: {backup_file.name}", Colors.GREEN)

def restore_db(backup_name: str = None):
    backups = list(BACKUP_DIR.glob("*.sqlite"))
    if not backups:
        cprint("❌ Нет резервных копий.", Colors.RED)
        return

    if backup_name:
        backup_file = BACKUP_DIR / backup_name
        if not backup_file.exists():
            cprint(f"❌ Резервная копия не найдена: {backup_name}", Colors.RED)
            return
    else:
        backup_file = max(backups, key=os.path.getctime)

    stop_server()
    time.sleep(1)
    shutil.copy2(backup_file, DB_PATH)
    cprint(f"✅ База восстановлена из: {backup_file.name}", Colors.GREEN)

def show_stats():
    if not DB_PATH.exists():
        cprint("❌ База данных не найдена.", Colors.RED)
        return

    import sqlite3
    try:
        conn = sqlite3.connect(DB_PATH)
        cur = conn.cursor()
        cur.execute("SELECT COUNT(*) FROM users")
        users = cur.fetchone()[0]
        cur.execute("SELECT COUNT(*) FROM messages")
        messages = cur.fetchone()[0]
        cur.execute("SELECT MAX(created_at) FROM messages")
        last_msg = cur.fetchone()[0] or "Never"
        conn.close()
        
        cprint("📊 Статистика сервера MESH:", Colors.CYAN, bold=True)
        cprint(f"  Зарегистрированных пользователей: {users}", Colors.GREEN)
        cprint(f"  Всего сообщений: {messages}", Colors.GREEN)
        cprint(f"  Последнее сообщение: {last_msg}", Colors.GREEN)
    except Exception as e:
        cprint(f"❌ Ошибка чтения БД: {e}", Colors.RED)

def run_test_client():
    cprint("🧪 Запуск тестового клиента...", Colors.CYAN)
    test_script = PROJECT_ROOT / "test_client.py"
    if not test_script.exists():
        cprint("❌ test_client.py не найден.", Colors.RED)
        return

    if run_command([sys.executable, str(test_script)]):
        cprint("✅ Тест завершён успешно.", Colors.GREEN)
    else:
        cprint("❌ Тест не удался.", Colors.RED)

def main():
    show_mesh_ascii()
    
    parser = argparse.ArgumentParser(
        prog="meshctl",
        description="Управление сервером MESH",
        formatter_class=argparse.RawTextHelpFormatter,
        add_help=False
    )
    parser.add_argument("--start", action="store_true", help="Запустить сервер MESH")
    parser.add_argument("--stop", action="store_true", help="Остановить сервер")
    parser.add_argument("--restart", action="store_true", help="Перезапустить сервер")
    parser.add_argument("--status", action="store_true", help="Показать статус сервера")
    parser.add_argument("--clean", action="store_true", help="Полная очистка (БД, логи, build)")
    parser.add_argument("--log", nargs="?", const=20, type=int, metavar="N", help="Показать последние N строк лога")
    parser.add_argument("--make", action="store_true", help="Собрать сервер через CMake")
    parser.add_argument("--rebuild", action="store_true", help="Очистить и пересобрать")
    parser.add_argument("--deps", action="store_true", help="Проверить зависимости")
    parser.add_argument("--backup", action="store_true", help="Создать резервную копию БД")
    parser.add_argument("--restore", nargs="?", const=None, metavar="BACKUP", help="Восстановить БД из резервной копии")
    parser.add_argument("--stats", action="store_true", help="Показать статистику сервера")
    parser.add_argument("--cert", action="store_true", help="Перегенерировать TLS-сертификаты")
    parser.add_argument("--test", action="store_true", help="Запустить тестовый клиент")
    parser.add_argument("--git-update", action="store_true", help="Обновить репозиторий на GitHub")
    
    # Аргументы для установки signal protocol
    signal_group = parser.add_argument_group("Signal Protocol Installation")
    signal_group.add_argument("--install-signal", action="store_true", help="Установить signal protocol (автоопределение ОС)")
    signal_group.add_argument("-u", "--ubuntu", action="store_true", help="Установить signal protocol для Ubuntu/Debian")
    signal_group.add_argument("-a", "--arch", action="store_true", help="Установить signal protocol для Arch Linux")
    
    parser.add_argument("--help", "-h", action="store_true", help="Показать эту справку")
    parser.add_argument("--version", action="version", version="MESH Control v3.0")

    args = parser.parse_args()

    if args.help:
        cprint("✨ MESH Messenger Control Utility", Colors.MAGENTA, bold=True)
        cprint("Управление безопасным мессенджером с E2EE по протоколу Signal", Colors.CYAN)
        print()
        parser.print_help()
        print(f"""
{Colors.BOLD}Описание:{Colors.RESET}
  MESH — это защищённый мессенджер с:
  • Сквозным шифрованием (E2EE) через Signal Protocol
  • Аутентификацией по нику и токену
  • Поддержкой приватных и публичных чатов
  • Защитой от флуда и DDoS
  • Хранением данных в SQLite

{Colors.BOLD}Примеры:{Colors.RESET}
  {Colors.CYAN}./meshctl.py --make{Colors.RESET}          # Собрать сервер
  {Colors.CYAN}./meshctl.py --start{Colors.RESET}        # Запустить
  {Colors.CYAN}./meshctl.py --status{Colors.RESET}       # Проверить статус
  {Colors.CYAN}./meshctl.py --log 50{Colors.RESET}       # Последние 50 строк лога
  {Colors.CYAN}./meshctl.py --backup{Colors.RESET}       # Резервная копия БД
  {Colors.CYAN}./meshctl.py --test{Colors.RESET}         # Запустить тестовый клиент
  {Colors.CYAN}./meshctl.py --install-signal -u{Colors.RESET}  # Установить Signal для Ubuntu
  {Colors.CYAN}./meshctl.py --install-signal -a{Colors.RESET}  # Установить Signal для Arch
  {Colors.CYAN}./meshctl.py --git-update{Colors.RESET}   # Обновить репозиторий на GitHub
        """)
        return

    if not any(vars(args).values()):
        cprint("✨ MESH Messenger Control Utility", Colors.MAGENTA, bold=True)
        cprint("Управление безопасным мессенджером с E2EE по протоколу Signal", Colors.CYAN)
        print("\nИспользуйте --help для получения справки.\n")
        return

    # Выполнение команд
    if args.make:
        make_server()
    elif args.rebuild:
        rebuild()
    elif args.start:
        start_server()
    elif args.stop:
        stop_server()
    elif args.restart:
        stop_server()
        time.sleep(1)
        start_server()
    elif args.status:
        show_status()
    elif args.clean:
        clean_project()
    elif args.log is not None:
        show_logs(args.log)
    elif args.deps:
        check_dependencies()
    elif args.backup:
        backup_db()
    elif args.restore is not None:
        restore_db(args.restore)
    elif args.stats:
        show_stats()
    elif args.cert:
        for f in [CERT_FILE, KEY_FILE]:
            if f.exists():
                f.unlink()
        generate_self_signed_cert()
    elif args.test:
        run_test_client()
    elif args.git_update:
        git_update()
    elif args.install_signal:
        if args.ubuntu:
            install_signal_protocol("ubuntu")
        elif args.arch:
            install_signal_protocol("arch")
        else:
            # Автоопределение
            distro = detect_distro()
            if distro in ("ubuntu", "debian", "arch"):
                install_signal_protocol(distro)
            else:
                cprint("❌ Не удалось определить дистрибутив.", Colors.RED)
                cprint("   Используйте --install-signal -u для Ubuntu/Debian", Colors.YELLOW)
                cprint("   Используйте --install-signal -a для Arch Linux", Colors.YELLOW)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        cprint("\n⚠️  Операция прервана пользователем.", Colors.YELLOW)
        sys.exit(1)