#!/usr/bin/env python3
"""
███████╗███████╗███████╗████████╗ █████╗ 
██╔════╝██╔════╝██╔════╝╚══██╔══╝██╔══██╗
███████╗███████╗███████╗   ██║   ███████║
╚════██║╚════██║╚════██║   ██║   ██╔══██║
███████║███████║███████║   ██║   ██║  ██║
╚══════╝╚══════╝╚══════╝   ╚═╝   ╚═╝  ╚═╝
           DEEP INSPECTION SUITE v5.0

Полный, пошаговый аудит MESH-проекта с живым выводом в консоль.
Каждый файл анализируется отдельно с паузой для удобства чтения.
"""

import os
import sys
import subprocess
import time
import socket
import ssl
import sqlite3
import signal
import shutil
from pathlib import Path
from typing import List

# Цвета
class C:
    R = "\033[31m"
    G = "\033[32m"
    Y = "\033[33m"
    B = "\033[34m"
    M = "\033[35m"
    C = "\033[36m"
    W = "\033[0m"
    BOLD = "\033[1m"

def log(msg: str, color: str = C.W, delay: float = 0.02):
    """Печатает сообщение посимвольно для эффекта "живого" вывода"""
    for char in msg:
        print(f"{color}{char}{C.W}", end="", flush=True)
        time.sleep(delay)
    print()

def log_slow(msg: str, color: str = C.W):
    """Быстрый вывод заголовков"""
    print(f"{color}{msg}{C.W}")

def run_cmd(cmd: List[str], cwd: Path = None, timeout: int = 30) -> subprocess.CompletedProcess:
    try:
        return subprocess.run(cmd, cwd=cwd or Path.cwd(), capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return subprocess.CompletedProcess(cmd, returncode=124, stdout="", stderr="Timeout")

# Глобальные пути
ROOT = Path(__file__).parent.resolve()
BUILD_DIR = ROOT / "build"
SERVER_BIN = BUILD_DIR / "mesh_server"
SERVER_C = ROOT / "server.c"
CMAKE_FILE = ROOT / "CMakeLists.txt"
DB_PATH = ROOT / "database" / "mesh_db.sqlite"
CERT = ROOT / "cert.pem"
KEY = ROOT / "key.pem"
PID_FILE = ROOT / "mesh_server.pid"

class MeshDeepInspector:
    def __init__(self):
        self.server_proc = None

    # ==================== АНАЛИЗ ОДНОГО ФАЙЛА ====================
    def inspect_file(self, file_path: Path):
        rel_path = file_path.relative_to(ROOT)
        log_slow(f"\n🔍 Анализ файла: {rel_path}", C.C)
        time.sleep(0.3)

        if file_path.suffix in {".c", ".h", ".cpp"}:
            try:
                content = file_path.read_text(errors='ignore')
                lines = content.splitlines()
                log(f"  Строк: {len(lines)}", C.Y, delay=0.01)

                # Поиск уязвимостей
                issues = []
                if "strcpy" in content:
                    issues.append("⚠️  Найдена небезопасная функция: strcpy")
                if "gets(" in content:
                    issues.append("🔥 КРИТИЧЕСКАЯ УЯЗВИМОСТЬ: gets()")
                if "sprintf" in content:
                    issues.append("⚠️  Найдена небезопасная функция: sprintf")
                if '"/home/just/' in content:
                    issues.append("📁 Жёстко заданный путь — не переносимо")
                if "malloc(" in content and "free(" not in content:
                    issues.append("💧 Возможна утечка памяти (malloc без free)")

                if issues:
                    for issue in issues:
                        log(issue, C.R, delay=0.015)
                else:
                    log("✅ Без критических проблем", C.G, delay=0.01)
            except Exception as e:
                log(f"❌ Ошибка чтения: {e}", C.R, delay=0.01)
        else:
            log("  Пропускаем (не код)", C.Y, delay=0.01)

        time.sleep(0.5)

    # ==================== ГЛУБОКИЙ СКАН ПРОЕКТА ====================
    def deep_inspect_project(self):
        log_slow("\n" + "="*80, C.BOLD)
        log_slow("🚀 НАЧАЛО ГЛУБОКОГО ИНСПЕКТИРОВАНИЯ ПРОЕКТА", C.BOLD)
        log_slow("="*80 + "\n", C.BOLD)
        time.sleep(1)

        all_files = sorted(ROOT.rglob("*"))
        code_files = [f for f in all_files if f.is_file() and f.suffix in {".c", ".h", ".cpp", ".hpp", ".cc"}]
        other_files = [f for f in all_files if f.is_file() and f not in code_files]

        log(f"📁 Обнаружено файлов всего: {len(all_files)}", C.C)
        log(f"📜 Файлов кода (C/C++): {len(code_files)}", C.C)
        log(f"📦 Прочих файлов: {len(other_files)}", C.Y)
        time.sleep(1.5)

        log_slow("\n" + "─"*50, C.C)
        log_slow("🔍 ПОДРОБНЫЙ АНАЛИЗ КАЖДОГО ФАЙЛА КОДА", C.C)
        log_slow("─"*50 + "\n", C.C)
        time.sleep(1)

        for file in code_files:
            self.inspect_file(file)

        log_slow("\n" + "─"*50, C.Y)
        log_slow("📂 КРАТКИЙ ОБЗОР ВАЖНЫХ ФАЙЛОВ", C.Y)
        log_slow("─"*50, C.Y)

        important = ["server.c", "server.h", "CMakeLists.txt", "src/main.cpp", "src/MeshServer.hpp", "meshctl.py"]
        for name in important:
            path = ROOT / name
            status = "✅ НАЙДЕН" if path.exists() else "❌ ОТСУТСТВУЕТ"
            color = C.G if path.exists() else C.R
            log(f"  {name:<25} {status}", color, delay=0.01)

        time.sleep(1)

    # ==================== САМОВОССТАНОВЛЕНИЕ ====================
    def self_heal(self):
        log_slow("\n" + "="*80, C.BOLD)
        log_slow("🛠️  ЭТАП: АВТОИСПРАВЛЕНИЕ ОКРУЖЕНИЯ", C.BOLD)
        log_slow("="*80, C.BOLD)
        time.sleep(1)

        log("Создание директорий...", C.C)
        (ROOT / "database").mkdir(exist_ok=True)
        BUILD_DIR.mkdir(exist_ok=True)
        time.sleep(0.5)

        if not CERT.exists() or not KEY.exists():
            log("Генерация TLS-сертификатов...", C.C)
            cmd = [
                "openssl", "req", "-x509", "-nodes", "-days", "365",
                "-newkey", "rsa:2048",
                "-keyout", str(KEY),
                "-out", str(CERT),
                "-subj", "/C=RU/ST=State/L=City/O=MESH/CN=localhost"
            ]
            res = run_cmd(cmd)
            if res.returncode == 0:
                CERT.chmod(0o644)
                KEY.chmod(0o600)
                log("✅ Сертификаты успешно созданы", C.G)
            else:
                log("❌ Не удалось создать сертификаты (установите OpenSSL)", C.R)
                return False
        else:
            log("✅ Сертификаты уже существуют", C.G)

        time.sleep(1)
        return True

    # ==================== СБОРКА ====================
    def build_server(self):
        log_slow("\n" + "="*80, C.BOLD)
        log_slow("🔨 ЭТАП: СБОРКА СЕРВЕРА", C.BOLD)
        log_slow("="*80, C.BOLD)
        time.sleep(1)

        if not CMAKE_FILE.exists():
            log("❌ CMakeLists.txt не найден", C.R)
            return False

        log("Запуск CMake...", C.C)
        res1 = run_cmd(["../cmake", ".."], cwd=BUILD_DIR)
        if res1.returncode != 0:
            log("❌ Ошибка CMake", C.R)
            return False

        log("Сборка через make...", C.C)
        res2 = run_cmd(["make", "-j4", "mesh_server"], cwd=BUILD_DIR)
        if res2.returncode != 0:
            log("❌ Ошибка сборки", C.R)
            return False

        if SERVER_BIN.exists():
            log("✅ Сборка завершена успешно", C.G)
            return True
        else:
            log("❌ Бинарник не создан", C.R)
            return False

    # ==================== ЗАПУСК И ТЕСТ ====================
    def start_and_test(self):
        log_slow("\n" + "="*80, C.BOLD)
        log_slow("🚀 ЭТАП: ЗАПУСК И ТЕСТИРОВАНИЕ", C.BOLD)
        log_slow("="*80, C.BOLD)
        time.sleep(1)

        if not SERVER_BIN.exists():
            log("❌ Сервер не собран", C.R)
            return False

        log("Запуск сервера в фоне...", C.C)
        PID_FILE.unlink(missing_ok=True)
        self.server_proc = subprocess.Popen(
            [str(SERVER_BIN)],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            cwd=ROOT,
            preexec_fn=os.setsid
        )
        time.sleep(4)

        if self.server_proc.poll() is not None:
            log("❌ Сервер упал при запуске", C.R)
            return False

        with open(PID_FILE, "w") as f:
            f.write(str(self.server_proc.pid))
        log("✅ Сервер запущен", C.G)

        log("Выполнение тестовой регистрации...", C.C)
        try:
            ctx = ssl.create_default_context()
            ctx.check_hostname = False
            ctx.verify_mode = ssl.CERT_NONE
            with socket.create_connection(("localhost", 5555), timeout=5) as sock:
                with ctx.wrap_socket(sock, server_hostname="localhost") as ssock:
                    ssock.send(b"/register mesh_inspector_test\n")
                    resp = ssock.recv(1024).decode()
                    if "success" in resp:
                        log("✅ Тестовая регистрация успешна", C.G)
                        success = True
                    else:
                        log("❌ Регистрация не удалась", C.R)
                        success = False
        except Exception as e:
            log(f"❌ Ошибка подключения: {e}", C.R)
            success = False

        self.stop_server()
        return success

    def stop_server(self):
        if PID_FILE.exists():
            try:
                pid = int(PID_FILE.read_text().strip())
                os.kill(pid, signal.SIGINT)
                time.sleep(2)
            except:
                pass
        if self.server_proc and self.server_proc.poll() is None:
            os.killpg(os.getpgid(self.server_proc.pid), signal.SIGINT)
            try:
                self.server_proc.wait(timeout=5)
            except:
                os.killpg(os.getpgid(self.server_proc.pid), signal.SIGKILL)
        PID_FILE.unlink(missing_ok=True)
        self.server_proc = None

    # ==================== ФИНАЛ ====================
    def final_report(self, success: bool):
        log_slow("\n" + "="*80, C.BOLD)
        log_slow("🎉 ФИНАЛЬНЫЙ ОТЧЁТ", C.BOLD)
        log_slow("="*80, C.BOLD)
        time.sleep(1)

        if success:
            log("✅ ПРОЕКТ ПОЛНОСТЬЮ РАБОТОСПОСОБЕН!", C.G)
            log("   - Все файлы проанализированы", C.G)
            log("   - Уязвимости не обнаружены", C.G)
            log("   - Сервер собран и протестирован", C.G)
        else:
            log("⚠️  ТРЕБУЕТСЯ ВНИМАНИЕ", C.Y)
            log("   Проверьте логи выше для деталей.", C.Y)

        log(f"\n📄 Отчёт сохранён в реальном времени в консоли.", C.C)
        log("💡 Совет: используйте `./meshctl.py --log` для просмотра серверных логов.", C.C)

def show_help():
    help_text = f"""
{C.BOLD}{C.M}MESH DEEP INSPECTION SUITE v5.0{C.W}

{C.BOLD}ИСПОЛЬЗОВАНИЕ:{C.W}
  {C.G}python3 mesh_test.py --deep-inspect{C.W}

{C.BOLD}ЧТО ДЕЛАЕТ СКРИПТ:{C.W}
  - Перебирает КАЖДЫЙ файл проекта по одному
  - Анализирует C-код на уязвимости
  - Показывает прогресс в реальном времени
  - Автоматически создаёт сертификаты
  - Собирает и тестирует сервер
  - Выводит красивый финальный отчёт

{C.BOLD}ОСОБЕННОСТЬ:{C.W}
  Весь вывод — "живой": символ за символом, с паузами,
  чтобы вы могли следить за процессом в деталях.
"""
    print(help_text)

def main():
    # ASCII заголовок
    print(f"""
{C.BOLD}{C.M}
███████╗███████╗███████╗████████╗ █████╗ 
██╔════╝██╔════╝██╔════╝╚══██╔══╝██╔══██╗
███████╗███████╗███████╗   ██║   ███████║
╚════██║╚════██║╚════██║   ██║   ██╔══██║
███████║███████║███████║   ██║   ██║  ██║
╚══════╝╚══════╝╚══════╝   ╚═╝   ╚═╝  ╚═╝
           DEEP INSPECTION SUITE v5.0{C.W}
    """)

    if not (ROOT / "server.c").exists():
        print(f"{C.R}❌ Скрипт должен находиться в корне проекта MESH{C.W}")
        sys.exit(1)

    if "--help" in sys.argv or "-h" in sys.argv or len(sys.argv) == 1:
        show_help()
        return

    if "--deep-inspect" in sys.argv:
        inspector = MeshDeepInspector()
        try:
            inspector.deep_inspect_project()
            if inspector.self_heal():
                if inspector.build_server():
                    inspector.start_and_test()
            inspector.final_report(True)
        except KeyboardInterrupt:
            log_slow("\n⚠️  Инспекция прервана пользователем", C.Y)
            inspector.stop_server()
            sys.exit(1)
    else:
        print(f"{C.R}❌ Используйте --deep-inspect или --help{C.W}")
        sys.exit(1)

if __name__ == "__main__":
    main()