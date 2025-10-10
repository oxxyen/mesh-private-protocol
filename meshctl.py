#!/usr/bin/env python3
"""
MESH Messenger Control Utility v3.3

Управление сервером MESH: сборка, запуск, тестирование, мониторинг, Git-интеграция.
Поддержка C, C++ и Qt GUI-клиента. Полный контроль над жизненным циклом.
"""

import os
import sys
import argparse
import subprocess
import signal
import time
import shutil
import json
from pathlib import Path
from typing import Optional, List

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

def get_git_version() -> str:
    """Возвращает версию в формате v3.3-abc123(-dirty)"""
    try:
        desc = subprocess.check_output(
            ["git", "describe", "--tags", "--always", "--dirty=-dirty"],
            stderr=subprocess.DEVNULL, text=True
        ).strip()
        return desc
    except:
        return "unknown"

def is_git_repo() -> bool:
    return (PROJECT_ROOT / ".git").exists()

def git_has_changes() -> bool:
    if not is_git_repo():
        return False
    result = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True)
    return bool(result.stdout.strip())

def git_current_branch() -> str:
    try:
        return subprocess.check_output(["git", "branch", "--show-current"], text=True).strip()
    except:
        return "unknown"

def git_last_commit() -> str:
    try:
        return subprocess.check_output(["git", "log", "-1", "--pretty=%h %s"], text=True).strip()
    except:
        return "No commits"

# ASCII ART
def show_mesh_ascii():
    version = get_git_version()
    mesh_ascii = f"""
{Colors.MAGENTA}{Colors.BOLD}
███╗   ███╗███████╗███████╗██╗  ██╗      █████╗ ███╗   ██╗███████╗ ██████╗██╗
████╗ ████║██╔════╝██╔════╝██║  ██║     ██╔══██╗████╗  ██║██╔════╝██╔════╝██║
██╔████╔██║█████╗  ███████╗███████║     ███████║██╔██╗ ██║███████╗██║     ██║
██║╚██╔╝██║██╔══╝  ╚════██║██╔══██║     ██╔══██║██║╚██╗██║╚════██║██║     ██║
██║ ╚═╝ ██║███████╗███████║██║  ██║     ██║  ██║██║ ╚████║███████║╚██████╗██║
╚═╝     ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝ ╚═════╝╚═╝
{Colors.RESET}
{Colors.CYAN}          MESSENGER WITH PRIVATE SIGNAL PROTOCOL - CONTROL UTILITY {version}{Colors.RESET}
    """
    print(mesh_ascii)

# Конфигурация
PROJECT_ROOT = Path(__file__).parent.resolve()
BUILD_DIR = PROJECT_ROOT / "build"
SERVER_BIN = BUILD_DIR / "mesh_server"
SERVER_CPP_BIN = BUILD_DIR / "mesh_server_cpp"
GUI_BIN = BUILD_DIR / "MeshChatClient"
CMAKEFILE = PROJECT_ROOT / "CMakeLists.txt"
GUI_PRO_FILE = PROJECT_ROOT / "src" / "assets" / "MeshChatClient.pro"
DB_PATH = PROJECT_ROOT / "database" / "mesh_db.sqlite"
PID_FILE = PROJECT_ROOT / "mesh_server.pid"
PID_CPP_FILE = PROJECT_ROOT / "mesh_server_cpp.pid"
PID_GUI_FILE = PROJECT_ROOT / "mesh_gui.pid"
LOG_FILE = PROJECT_ROOT / "mesh_server.log"
LOG_CPP_FILE = PROJECT_ROOT / "mesh_server_cpp.log"
CERT_FILE = PROJECT_ROOT / "cert.pem"
KEY_FILE = PROJECT_ROOT / "key.pem"
BACKUP_DIR = PROJECT_ROOT / "backups"
CONFIG_FILE = PROJECT_ROOT / "mesh.conf"

def ensure_directories():
    (PROJECT_ROOT / "database").mkdir(exist_ok=True)
    BACKUP_DIR.mkdir(exist_ok=True)

def run_command(cmd: list, cwd: Optional[Path] = None, silent: bool = False) -> bool:
    try:
        result = subprocess.run(cmd, cwd=cwd or PROJECT_ROOT, capture_output=not silent, text=True)
        if not silent and result.returncode != 0:
            cprint(result.stderr, Colors.RED)
        return result.returncode == 0
    except Exception as e:
        if not silent:
            cprint(f"❌ Ошибка выполнения: {e}", Colors.RED)
        return False

def install_signal_protocol_ubuntu():
    cprint("📦 Установка libsignal-protocol-c для Ubuntu...", Colors.CYAN)
    cmds = [
        ["sudo", "apt", "update"],
        ["sudo", "apt", "install", "-y", "build-essential", "cmake", "git", "libssl-dev", "libsqlite3-dev"]
    ]
    for cmd in cmds:
        if not run_command(cmd, silent=False):
            cprint("❌ Ошибка при установке зависимостей", Colors.RED)
            return False

    # Клонирование и сборка libsignal-protocol-c
    libsignal_dir = PROJECT_ROOT / "libsignal-protocol-c"
    if not libsignal_dir.exists():
        cprint("📥 Клонирование libsignal-protocol-c...", Colors.CYAN)
        if not run_command(["git", "clone", "https://github.com/signalapp/libsignal-protocol-c.git"], silent=False):
            return False

    build_dir = libsignal_dir / "build"
    build_dir.mkdir(exist_ok=True)
    if not run_command(["cmake", ".."], cwd=build_dir, silent=False):
        return False
    if not run_command(["make", "-j4"], cwd=build_dir, silent=False):
        return False
    if not run_command(["sudo", "make", "install"], cwd=build_dir, silent=False):
        return False

    cprint("✅ libsignal-protocol-c установлен для Ubuntu!", Colors.GREEN, bold=True)
    return True

def install_signal_protocol_arch():
    cprint("📦 Установка libsignal-protocol-c для Arch Linux...", Colors.CYAN)
    # Arch: используем AUR или собираем вручную
    if shutil.which("yay"):
        cprint("🔍 Используем yay для установки из AUR...", Colors.CYAN)
        if run_command(["yay", "-S", "--noconfirm", "libsignal-protocol-c-git"]):
            cprint("✅ Установлено через AUR!", Colors.GREEN)
            return True
    elif shutil.which("paru"):
        cprint("🔍 Используем paru для установки из AUR...", Colors.CYAN)
        if run_command(["paru", "-S", "--noconfirm", "libsignal-protocol-c-git"]):
            cprint("✅ Установлено через AUR!", Colors.GREEN)
            return True

    # Если нет AUR-хелпера — собираем вручную
    cprint("🛠 Сборка libsignal-protocol-c вручную...", Colors.YELLOW)
    if not run_command(["sudo", "pacman", "-Sy", "--noconfirm", "base-devel", "cmake", "git", "openssl", "sqlite"]):
        return False
    return install_signal_protocol_ubuntu()  # повторно используем ту же логику сборки

def generate_self_signed_cert():
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

def is_server_running(pid_file: Path) -> bool:
    if not pid_file.exists():
        return False
    try:
        with open(pid_file, 'r') as f:
            pid = int(f.read().strip())
        os.kill(pid, 0)
        return True
    except (OSError, ValueError):
        pid_file.unlink(missing_ok=True)
        return False

def get_server_pid(pid_file: Path) -> Optional[int]:
    if not pid_file.exists():
        return None
    try:
        with open(pid_file, 'r') as f:
            return int(f.read().strip())
    except (OSError, ValueError):
        return None

def start_server(bin_path: Path, pid_file: Path, log_file: Path, name: str):
    if is_server_running(pid_file):
        cprint(f"⚠️  {name} уже запущен.", Colors.YELLOW)
        return
    ensure_directories()
    if not generate_self_signed_cert():
        cprint("❌ Не удалось создать сертификат. Установите OpenSSL.", Colors.RED)
        return
    if not bin_path.exists():
        cprint(f"❌ Бинарник {name} не найден. Соберите: --make{'-cpp' if 'cpp' in name.lower() else ''}", Colors.RED)
        return
    cprint(f"🚀 Запуск {name}...", Colors.CYAN, bold=True)
    log_fd = open(log_file, "a")
    proc = subprocess.Popen([str(bin_path)], stdout=log_fd, stderr=subprocess.STDOUT, cwd=PROJECT_ROOT, preexec_fn=os.setsid)
    with open(pid_file, "w") as f:
        f.write(str(proc.pid))
    cprint(f"✅ {name} запущен (PID: {proc.pid})", Colors.GREEN, bold=True)

def start_gui():
    if is_server_running(PID_GUI_FILE):
        cprint("⚠️  GUI-клиент уже запущен.", Colors.YELLOW)
        return
    if not GUI_BIN.exists():
        cprint("❌ GUI-клиент не собран. Выполните: --make-gui", Colors.RED)
        return
    cprint("🖥️  Запуск Qt GUI-клиента...", Colors.CYAN, bold=True)
    proc = subprocess.Popen([str(GUI_BIN)], cwd=PROJECT_ROOT, preexec_fn=os.setsid)
    with open(PID_GUI_FILE, "w") as f:
        f.write(str(proc.pid))
    cprint(f"✅ GUI-клиент запущен (PID: {proc.pid})", Colors.GREEN, bold=True)

def stop_server(pid_file: Path, name: str):
    if not is_server_running(pid_file):
        cprint(f"⚠️  {name} не запущен.", Colors.YELLOW)
        return
    pid = get_server_pid(pid_file)
    if pid:
        cprint(f"🛑 Остановка {name} (PID: {pid})...", Colors.YELLOW)
        try:
            os.kill(pid, signal.SIGINT)
            for _ in range(10):
                if not is_server_running(pid_file):
                    break
                time.sleep(1)
            else:
                cprint("⚠️  Принудительное завершение...", Colors.YELLOW)
                os.kill(pid, signal.SIGKILL)
        except OSError:
            pass
    pid_file.unlink(missing_ok=True)
    cprint(f"✅ {name} остановлен.", Colors.GREEN)

def make_server(cpp: bool = False):
    target = "mesh_server_cpp" if cpp else "mesh_server"
    cprint(f"🔨 Сборка {target} через CMake...", Colors.CYAN, bold=True)
    if not CMAKEFILE.exists():
        cprint("❌ CMakeLists.txt не найден.", Colors.RED)
        return
    BUILD_DIR.mkdir(exist_ok=True)
    if run_command(["cmake", ".."], cwd=BUILD_DIR, silent=True):
        cprint("✅ CMake конфигурация успешна.", Colors.GREEN)
    else:
        cprint("❌ Ошибка конфигурации CMake.", Colors.RED)
        return
    try:
        import multiprocessing
        jobs = multiprocessing.cpu_count()
    except:
        jobs = 4
    if run_command(["cmake", "--build", ".", "--parallel", str(jobs), "--target", target], cwd=BUILD_DIR, silent=False):
        cprint(f"✅ Сборка {target} успешна!", Colors.GREEN, bold=True)
    else:
        cprint(f"❌ Ошибка сборки {target}.", Colors.RED, bold=True)

def make_gui():
    cprint("🎨 Сборка Qt GUI-клиента...", Colors.CYAN, bold=True)
    if not GUI_PRO_FILE.exists():
        cprint(f"❌ Файл проекта не найден: {GUI_PRO_FILE}", Colors.RED)
        return
    BUILD_DIR.mkdir(exist_ok=True)
    if run_command(["qmake", str(GUI_PRO_FILE)], cwd=BUILD_DIR, silent=True) and \
       run_command(["make", "-j4"], cwd=BUILD_DIR, silent=False):
        cprint("✅ GUI-клиент собран!", Colors.GREEN, bold=True)
    else:
        cprint("❌ Ошибка сборки GUI-клиента.", Colors.RED, bold=True)

def clean_project():
    cprint("🧹 Полная очистка проекта...", Colors.YELLOW)
    stop_server(PID_FILE, "C-сервер")
    stop_server(PID_CPP_FILE, "C++ обёртка")
    stop_server(PID_GUI_FILE, "GUI-клиент")
    time.sleep(1)
    to_remove = [DB_PATH, LOG_FILE, LOG_CPP_FILE, PID_FILE, PID_CPP_FILE, PID_GUI_FILE, CERT_FILE, KEY_FILE, CONFIG_FILE]
    for path in to_remove:
        if path.exists():
            path.unlink()
            cprint(f"  Удалён: {path.name}", Colors.GREEN)
    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
        cprint("  Удалена папка build", Colors.GREEN)
    cprint("✨ Очистка завершена.", Colors.GREEN)

def show_status(pid_file: Path, name: str):
    if is_server_running(pid_file):
        pid = get_server_pid(pid_file)
        cprint(f"🟢 {name} запущен (PID: {pid})", Colors.GREEN, bold=True)
    else:
        cprint(f"🔴 {name} остановлен", Colors.RED, bold=True)

def show_logs(log_file: Path, n: int = 20):
    if not log_file.exists():
        cprint(f"📝 Лог {log_file.name} отсутствует.", Colors.YELLOW)
        return
    with open(log_file, "r") as f:
        lines = f.readlines()
        cprint(f"📄 Последние {min(n, len(lines))} строк ({log_file.name}):", Colors.CYAN, bold=True)
        for line in lines[-n:]:
            print(line, end="")

def rotate_logs():
    for log in [LOG_FILE, LOG_CPP_FILE]:
        if log.exists():
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            backup = log.with_name(f"{log.stem}_{timestamp}{log.suffix}")
            shutil.move(log, backup)
            cprint(f"✅ Лог архивирован: {backup.name}", Colors.GREEN)
    cprint("🔄 Логи очищены. Новые записи будут в новых файлах.", Colors.CYAN)

def backup_db():
    if not DB_PATH.exists():
        cprint("❌ База данных не найдена.", Colors.RED)
        return
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    backup_file = BACKUP_DIR / f"mesh_db_{timestamp}.sqlite"
    shutil.copy2(DB_PATH, backup_file)
    cprint(f"✅ Резервная копия: {backup_file.name}", Colors.GREEN)

def restore_db(backup_name: str = None):
    backups = list(BACKUP_DIR.glob("*.sqlite"))
    if not backups:
        cprint("❌ Нет резервных копий.", Colors.RED)
        return
    if backup_name:
        backup_file = BACKUP_DIR / backup_name
        if not backup_file.exists():
            cprint(f"❌ Копия не найдена: {backup_name}", Colors.RED)
            return
    else:
        backup_file = max(backups, key=os.path.getctime)
    stop_server(PID_FILE, "C-сервер")
    stop_server(PID_CPP_FILE, "C++ обёртка")
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
        cprint(f"  Пользователей: {users}", Colors.GREEN)
        cprint(f"  Сообщений: {messages}", Colors.GREEN)
        cprint(f"  Последнее сообщение: {last_msg}", Colors.GREEN)
    except Exception as e:
        cprint(f"❌ Ошибка БД: {e}", Colors.RED)

def health_check():
    ok = True
    cprint("🩺 Проверка здоровья системы...", Colors.CYAN, bold=True)
    if DB_PATH.exists():
        cprint("✅ База данных: доступна", Colors.GREEN)
    else:
        cprint("⚠️  База данных: отсутствует", Colors.YELLOW)
        ok = False
    if SERVER_BIN.exists():
        cprint("✅ C-бинарник: собран", Colors.GREEN)
    else:
        cprint("⚠️  C-бинарник: не собран", Colors.YELLOW)
    if SERVER_CPP_BIN.exists():
        cprint("✅ C++ бинарник: собран", Colors.GREEN)
    else:
        cprint("⚠️  C++ бинарник: не собран", Colors.YELLOW)
    if GUI_BIN.exists():
        cprint("✅ GUI-клиент: собран", Colors.GREEN)
    else:
        cprint("⚠️  GUI-клиент: не собран", Colors.YELLOW)
    if is_server_running(PID_FILE):
        cprint("🟢 C-сервер: запущен", Colors.GREEN)
    if is_server_running(PID_CPP_FILE):
        cprint("🟢 C++ обёртка: запущена", Colors.GREEN)
    if is_server_running(PID_GUI_FILE):
        cprint("🟢 GUI-клиент: запущен", Colors.GREEN)
    if ok:
        cprint("✨ Система в порядке!", Colors.GREEN, bold=True)
    else:
        cprint("🔧 Рекомендуется выполнить: --make --make-cpp --make-gui", Colors.YELLOW)

def generate_config():
    config = {
        "port": 5555,
        "db_path": str(DB_PATH),
        "max_connections_per_ip": 5,
        "enable_ratelimit": True,
        "tls_cert": str(CERT_FILE),
        "tls_key": str(KEY_FILE),
        "log_level": "info"
    }
    with open(CONFIG_FILE, "w") as f:
        json.dump(config, f, indent=4)
    cprint(f"⚙️  Конфигурация сохранена: {CONFIG_FILE.name}", Colors.GREEN)

def git_status():
    if not is_git_repo():
        cprint("❌ Не Git-репозиторий", Colors.RED)
        return
    cprint(f"🌿 Ветка: {git_current_branch()}", Colors.CYAN)
    cprint(f"🔖 Последний коммит: {git_last_commit()}", Colors.CYAN)
    if git_has_changes():
        cprint("⚠️  Есть несохранённые изменения!", Colors.YELLOW)
        subprocess.run(["git", "status", "--short"])
    else:
        cprint("✅ Рабочая директория чиста", Colors.GREEN)

def git_commit(message: Optional[str] = None):
    if not is_git_repo():
        cprint("❌ Не Git-репозиторий", Colors.RED)
        return
    if not git_has_changes():
        cprint("✅ Нет изменений для коммита", Colors.GREEN)
        return
    if not message:
        types = ["feat", "fix", "docs", "style", "refactor", "test", "chore", "security"]
        cprint("Выберите тип коммита:", Colors.CYAN)
        for i, t in enumerate(types, 1):
            print(f"  {i}. {t}")
        try:
            choice = int(input("Ваш выбор (1-8): ")) - 1
            if 0 <= choice < len(types):
                scope = input("Область (опционально, например 'server', 'cli'): ").strip()
                desc = input("Описание: ").strip()
                if scope:
                    message = f"{types[choice]}({scope}): {desc}"
                else:
                    message = f"{types[choice]}: {desc}"
            else:
                cprint("Неверный выбор", Colors.RED)
                return
        except (ValueError, KeyboardInterrupt):
            cprint("Отмена", Colors.YELLOW)
            return
    if run_command(["git", "add", "."], silent=True) and run_command(["git", "commit", "-m", message], silent=False):
        cprint("✅ Коммит создан!", Colors.GREEN)
    else:
        cprint("❌ Ошибка при коммите", Colors.RED)

def git_push():
    if not is_git_repo():
        cprint("❌ Не Git-репозиторий", Colors.RED)
        return
    if run_command(["git", "push", "origin", "main"]):
        cprint("🚀 Изменения отправлены на GitHub", Colors.GREEN)
    else:
        cprint("❌ Ошибка при отправке", Colors.RED)

def main():
    show_mesh_ascii()
    if is_git_repo() and git_has_changes():
        cprint("⚠️  Обнаружены несохранённые изменения в репозитории!", Colors.YELLOW, bold=True)
        cprint("    Используйте --git-commit для фиксации изменений.", Colors.YELLOW)

    parser = argparse.ArgumentParser(
        prog="meshctl",
        description="Управление MESH сервером (C, C++, Qt GUI) с Git-интеграцией",
        formatter_class=argparse.RawTextHelpFormatter,
        add_help=False
    )
    parser.add_argument("--start", action="store_true", help="Запустить C-сервер")
    parser.add_argument("--start-cpp", action="store_true", help="Запустить C++ обёртку")
    parser.add_argument("--start-gui", action="store_true", help="Запустить Qt GUI-клиент")
    parser.add_argument("--stop", action="store_true", help="Остановить C-сервер")
    parser.add_argument("--stop-cpp", action="store_true", help="Остановить C++ обёртку")
    parser.add_argument("--stop-gui", action="store_true", help="Остановить GUI-клиент")
    parser.add_argument("--restart", action="store_true", help="Перезапустить C-сервер")
    parser.add_argument("--restart-cpp", action="store_true", help="Перезапустить C++ обёртку")
    parser.add_argument("--status", action="store_true", help="Статус C-сервера")
    parser.add_argument("--status-cpp", action="store_true", help="Статус C++ обёртки")
    parser.add_argument("--status-gui", action="store_true", help="Статус GUI-клиента")
    parser.add_argument("--clean", action="store_true", help="Полная очистка")
    parser.add_argument("--log", nargs="?", const=20, type=int, metavar="N", help="Лог C-сервера")
    parser.add_argument("--log-cpp", nargs="?", const=20, type=int, metavar="N", help="Лог C++ обёртки")
    parser.add_argument("--make", action="store_true", help="Собрать C-сервер")
    parser.add_argument("--make-cpp", action="store_true", help="Собрать C++ обёртку")
    parser.add_argument("--make-gui", action="store_true", help="Собрать Qt GUI-клиент из src/assets/")
    parser.add_argument("--rebuild", action="store_true", help="Очистить и пересобрать C-сервер")
    parser.add_argument("--rebuild-cpp", action="store_true", help="Очистить и пересобрать C++ обёртку")
    parser.add_argument("--backup", action="store_true", help="Резервная копия БД")
    parser.add_argument("--restore", nargs="?", const=None, metavar="BACKUP", help="Восстановить БД")
    parser.add_argument("--stats", action="store_true", help="Статистика сервера")
    parser.add_argument("--cert", action="store_true", help="Перегенерировать TLS-сертификаты")
    parser.add_argument("--rotate-logs", action="store_true", help="Архивировать и очистить логи")
    parser.add_argument("--health-check", action="store_true", help="Проверить здоровье системы")
    parser.add_argument("--gen-config", action="store_true", help="Сгенерировать mesh.conf")
    
    # Git команды
    parser.add_argument("--git-status", action="store_true", help="Показать статус репозитория")
    parser.add_argument("--git-commit", nargs="?", const=None, metavar="MESSAGE", help="Сделать коммит (интерактивно, если без сообщения)")
    parser.add_argument("--git-push", action="store_true", help="Отправить изменения на GitHub")
    
    # Signal Protocol
    parser.add_argument("--install-signal", action="store_true", help="Установить libsignal-protocol-c")
    parser.add_argument("-u", action="store_true", help="Для Ubuntu/Debian")
    parser.add_argument("-a", action="store_true", help="Для Arch Linux")
    
    parser.add_argument("--help", "-h", action="store_true", help="Показать справку")
    parser.add_argument("--version-full", action="store_true", help="Показать полную версию")

    args = parser.parse_args()

    if args.version_full:
        cprint(f"📦 Версия: {get_git_version()}", Colors.CYAN, bold=True)
        if is_git_repo():
            cprint(f"🌿 Ветка: {git_current_branch()}", Colors.CYAN)
            cprint(f"🔖 Коммит: {git_last_commit()}", Colors.CYAN)
        return

    if args.help:
        cprint("✨ MESH Messenger Control Utility", Colors.MAGENTA, bold=True)
        cprint("Управление защищённым мессенджером с E2EE по протоколу Signal", Colors.CYAN)
        print()
        parser.print_help()
        print(f"""
{Colors.BOLD}Примеры:{Colors.RESET}
  {Colors.CYAN}./meshctl.py --make --make-cpp --make-gui{Colors.RESET}
  {Colors.CYAN}./meshctl.py --install-signal -u{Colors.RESET}      # Ubuntu
  {Colors.CYAN}./meshctl.py --install-signal -a{Colors.RESET}      # Arch
  {Colors.CYAN}./meshctl.py --start-gui{Colors.RESET}
  {Colors.CYAN}./meshctl.py --git-commit{Colors.RESET}
        """)
        return

    if args.install_signal:
        if args.u:
            install_signal_protocol_ubuntu()
        elif args.a:
            install_signal_protocol_arch()
        else:
            cprint("❌ Укажите ОС: --install-signal -u (Ubuntu) или -a (Arch)", Colors.RED)
        return

    if not any(vars(args).values()):
        cprint("✨ MESH Messenger Control Utility", Colors.MAGENTA, bold=True)
        cprint("Используйте --help для справки или --git-status для проверки репозитория.", Colors.CYAN)
        return

    # Выполнение команд
    if args.make:
        make_server(cpp=False)
    elif args.make_cpp:
        make_server(cpp=True)
    elif args.make_gui:
        make_gui()
    elif args.rebuild:
        clean_project()
        time.sleep(1)
        make_server(cpp=False)
    elif args.rebuild_cpp:
        clean_project()
        time.sleep(1)
        make_server(cpp=True)
    elif args.start:
        start_server(SERVER_BIN, PID_FILE, LOG_FILE, "C-сервер")
    elif args.start_cpp:
        start_server(SERVER_CPP_BIN, PID_CPP_FILE, LOG_CPP_FILE, "C++ обёртка")
    elif args.start_gui:
        start_gui()
    elif args.stop:
        stop_server(PID_FILE, "C-сервер")
    elif args.stop_cpp:
        stop_server(PID_CPP_FILE, "C++ обёртка")
    elif args.stop_gui:
        stop_server(PID_GUI_FILE, "GUI-клиент")
    elif args.restart:
        stop_server(PID_FILE, "C-сервер")
        time.sleep(1)
        start_server(SERVER_BIN, PID_FILE, LOG_FILE, "C-сервер")
    elif args.restart_cpp:
        stop_server(PID_CPP_FILE, "C++ обёртка")
        time.sleep(1)
        start_server(SERVER_CPP_BIN, PID_CPP_FILE, LOG_CPP_FILE, "C++ обёртка")
    elif args.status:
        show_status(PID_FILE, "C-сервер")
    elif args.status_cpp:
        show_status(PID_CPP_FILE, "C++ обёртка")
    elif args.status_gui:
        show_status(PID_GUI_FILE, "GUI-клиент")
    elif args.clean:
        clean_project()
    elif args.log is not None:
        show_logs(LOG_FILE, args.log)
    elif args.log_cpp is not None:
        show_logs(LOG_CPP_FILE, args.log_cpp)
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
    elif args.rotate_logs:
        rotate_logs()
    elif args.health_check:
        health_check()
    elif args.gen_config:
        generate_config()
    elif args.git_status:
        git_status()
    elif args.git_commit is not None:
        git_commit(args.git_commit)
    elif args.git_push:
        git_push()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        cprint("\n⚠️  Операция прервана пользователем.", Colors.YELLOW)
        sys.exit(1)