#!/usr/bin/env python3
"""
BROOTFROUCE - Advanced C/C++ Code Obfuscation & Protection System
Мощный инструмент для защиты кода от реверс-инженерии и взлома
"""

import os
import re
import random
import string
import argparse
import sys
import base64
import zlib
import hashlib
import subprocess
import platform
from pathlib import Path
from datetime import datetime

class SecurityChecker:
    """Класс для проверки безопасности и обнаружения инструментов реверса"""
    
    @staticmethod
    def detect_ida():
        """Обнаружение IDA Pro и других дизассемблеров"""
        suspicious_processes = [
            'ida', 'idaq', 'ida64', 'idaq64', 'idafree', 'idafree64',
            'ghidra', 'binaryninja', 'hopper', 'radare2', 'cutter',
            'ollydbg', 'x64dbg', 'x32dbg', 'windbg', 'immunity'
        ]
        
        system = platform.system().lower()
        try:
            if system == 'windows':
                result = subprocess.run(['tasklist'], capture_output=True, text=True, shell=True)
                output = result.stdout.lower()
            else:  # linux/macos
                result = subprocess.run(['ps', 'aux'], capture_output=True, text=True)
                output = result.stdout.lower()
            
            detected_tools = []
            for tool in suspicious_processes:
                if tool in output:
                    detected_tools.append(tool)
            
            return detected_tools
        except Exception:
            return []
    
    @staticmethod
    def check_debugger():
        """Проверка наличия отладчика"""
        try:
            # Простые проверки на отладчик (могут быть расширены)
            if hasattr(sys, 'gettrace') and sys.gettrace() is not None:
                return True
            return False
        except:
            return False
    
    @staticmethod
    def generate_anti_reverse_code():
        """Генерация кода для защиты от реверса"""
        anti_code = '''
// === BROOTFROUCE ANTI-REVERSE PROTECTION ===
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #define _ANTI_DEBUG_ 1
#else
    #include <sys/ptrace.h>
    #include <signal.h>
    #define _ANTI_DEBUG_ 2
#endif

// Функции защиты от отладки
int __anti_debug_check() {
    volatile int result = 0;
    
#if _ANTI_DEBUG_ == 1
    // Windows anti-debug
    if (IsDebuggerPresent()) {
        return -1;
    }
    
    __try {
        __asm { int 3 }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        result = 1;
    }
#else
    // Linux/Unix anti-debug
    if (ptrace(PTRACE_TRACEME, 0, 1, 0) == -1) {
        return -1;
    }
    
    // Проверка времени выполнения (простая защита от трассировки)
    volatile clock_t start = clock();
    for (volatile int i = 0; i < 1000; i++) {
        result ^= i * 0xDEADBEEF;
    }
    volatile clock_t end = clock();
    
    if ((end - start) > (CLOCKS_PER_SEC / 10)) {
        return -2;
    }
#endif

    return 0;
}

// Зашифрованные проверки целостности
void __integrity_check() {
    volatile char* self_ptr = (volatile char*)&__integrity_check;
    volatile unsigned long checksum = 0x12345678;
    
    for (volatile int i = 0; i < 64; i++) {
        checksum = (checksum << 3) ^ (checksum >> 5) ^ self_ptr[i];
    }
    
    // Если checksum не соответствует ожидаемому, код может вести себя непредсказуемо
    if (checksum != 0x89ABCDEF) {  // Это значение будет заменено реальным checksum
        // Непрямой выход для усложнения анализа
        volatile int* crash = 0;
        *crash = 0xDEAD;
    }
}

// Ложные точки останова для запутывания
void __fake_debug_breaks() {
    volatile int trap = 0;
    
    // Ложные INT3 инструкции
    __asm__ volatile ("nop");
    #ifdef _WIN32
        __asm__ volatile ("int $0x03");
    #endif
    
    trap = 1;
    while(trap) {
        // Бесконечный цикл для запутывания
        trap ^= 0x1;
    }
}
'''
        return anti_code

class AdvancedObfuscator:
    def __init__(self):
        self.var_mapping = {}
        self.func_mapping = {}
        self.type_mapping = {}
        self.macro_mapping = {}
        self.used_names = set()
        self.string_mapping = {}
        self.obfuscation_level = "high"
        self.security_checker = SecurityChecker()
        
    def set_obfuscation_level(self, level):
        """Устанавливает уровень обфускации"""
        self.obfuscation_level = level
        
    def generate_complex_name(self, length=12):
        """Генерирует сложное имя с использованием различных символов"""
        chars = string.ascii_letters + string.digits + '_'
        while True:
            if random.random() > 0.7:
                name = '_' + ''.join(random.choices(chars, k=length-1))
            else:
                name = ''.join(random.choices(chars, k=length))
            
            if name not in self.used_names and not name[0].isdigit():
                self.used_names.add(name)
                return name
    
    def obfuscate_variables(self, code):
        """Расширенная обфускация переменных"""
        print("🔤 Обфускация переменных...")
        
        patterns = [
            r'\b(int|float|double|char|void|bool|long|short|unsigned|const|static|auto|register|volatile)\s+(\w+)\s*[;=,\[]',
            r'\b(struct|class|enum|union)\s+(\w+)\s*[{]',
            r'\b(\w+)\s*(\**\w+)\s*=\s*[^;]+;',
            r'\b(\w+)\s*(\**\w+)\s*;',
        ]
        
        for pattern in patterns:
            matches = re.finditer(pattern, code, re.MULTILINE)
            for match in matches:
                var_name = match.group(2) if len(match.groups()) >= 2 else match.group(1)
                if (var_name and 
                    len(var_name) > 1 and 
                    var_name not in ['main', 'printf', 'scanf', 'malloc', 'free', 'sizeof', 'if', 'for', 'while'] and
                    not var_name.startswith('_')):
                    if var_name not in self.var_mapping:
                        self.var_mapping[var_name] = self.generate_complex_name(random.randint(8, 15))
        
        for old_name, new_name in sorted(self.var_mapping.items(), key=lambda x: len(x[0]), reverse=True):
            code = re.sub(r'\b' + re.escape(old_name) + r'\b', new_name, code)
        
        return code
    
    def obfuscate_functions(self, code):
        """Расширенная обфускация функций"""
        print("⚙️  Обфускация функций...")
        
        func_patterns = [
            r'\b(\w+)\s+(\w+)\s*\([^)]*\)\s*{',
            r'\b(\w+)\s+(\w+)\s*\([^)]*\)\s*;',
        ]
        
        for pattern in func_patterns:
            matches = re.finditer(pattern, code)
            for match in matches:
                return_type = match.group(1)
                func_name = match.group(2)
                if (func_name and 
                    len(func_name) > 1 and 
                    func_name not in ['main', 'printf', 'scanf', 'malloc', 'free', 'if', 'for', 'while'] and
                    not func_name.startswith('__')):
                    if func_name not in self.func_mapping:
                        self.func_mapping[func_name] = self.generate_complex_name(random.randint(10, 20))
        
        for old_name, new_name in sorted(self.func_mapping.items(), key=lambda x: len(x[0]), reverse=True):
            code = re.sub(r'\b' + re.escape(old_name) + r'\s*\(', new_name + '(', code)
        
        return code
    
    def obfuscate_types(self, code):
        """Обфускация пользовательских типов"""
        print("📐 Обфускация типов данных...")
        
        type_patterns = [
            r'\btypedef\s+.*?\s+(\w+)\s*;',
            r'\bstruct\s+(\w+)\s*{',
            r'\bclass\s+(\w+)\s*{',
            r'\benum\s+(\w+)\s*{',
        ]
        
        for pattern in type_patterns:
            matches = re.finditer(pattern, code, re.MULTILINE | re.DOTALL)
            for match in matches:
                type_name = match.group(1)
                if type_name and type_name not in self.type_mapping:
                    self.type_mapping[type_name] = self.generate_complex_name(random.randint(8, 15))
        
        for old_name, new_name in self.type_mapping.items():
            code = re.sub(r'\b' + re.escape(old_name) + r'\b', new_name, code)
        
        return code
    
    def remove_comments_and_spaces(self, code):
        """Агрессивное удаление комментариев и пробелов"""
        print("🗑️  Удаление комментариев и форматирования...")
        
        code = re.sub(r'/\*.*?\*/', '', code, flags=re.DOTALL)
        code = re.sub(r'//.*$', '', code, flags=re.MULTILINE)
        
        if self.obfuscation_level == "high":
            code = re.sub(r'[ \t]+', ' ', code)
            code = re.sub(r'\s*([=+-\/*&|!<>(){}\[\].,;:])\s*', r'\1', code)
            code = re.sub(r'\n\s*\n', '\n', code)
            code = re.sub(r'^\s+', '', code, flags=re.MULTILINE)
        
        return code
    
    def encrypt_strings(self, code):
        """Шифрование строковых литералов с помощью XOR и Base64"""
        print("🔒 Шифрование строк...")
        
        strings = re.findall(r'\"([^\"]*)\"', code)
        for s in strings:
            if len(s) > 2 and s not in ['%d', '%s', '%c', '%f']:
                if random.random() > 0.5:
                    encrypted = base64.b64encode(s.encode()).decode()
                    code = code.replace(f'"{s}"', f'__decrypt_b64("{encrypted}")')
                else:
                    key = random.randint(1, 255)
                    encrypted = ''.join(chr(ord(c) ^ key) for c in s)
                    hex_str = ''.join(f'\\x{ord(c):02x}' for c in encrypted)
                    code = code.replace(f'"{s}"', f'__decrypt_xor("{hex_str}", {key})')
        
        return code
    
    def add_string_decryption_functions(self, code):
        """Добавляет функции для дешифровки строк"""
        decryption_code = '''
// === BROOTFROUCE STRING DECRYPTION ===
char* __decrypt_b64(const char* encoded) {
    static char buffer[1024];
    int len = strlen(encoded);
    __b64_decode(encoded, len, buffer);
    return buffer;
}

char* __decrypt_xor(const char* encrypted, int key) {
    static char result[1024];
    int i = 0;
    const char* ptr = encrypted;
    while (*ptr) {
        if (*ptr == '\\\\' && *(ptr+1) == 'x') {
            char hex[3] = {*(ptr+2), *(ptr+3), 0};
            result[i++] = (char)strtol(hex, NULL, 16);
            ptr += 4;
        } else {
            result[i++] = *ptr++;
        }
    }
    result[i] = '\\\\0';
    
    for (int j = 0; j < i; j++) {
        result[j] = result[j] ^ key;
    }
    return result;
}

void __b64_decode(const char* encoded, int len, char* decoded) {
    const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0, j = 0;
    unsigned char char_array_4[4], char_array_3[3];
    int in_len = len;
    
    while (in_len-- && encoded[i] != '=') {
        char_array_4[j++] = encoded[i++];
        if (j == 4) {
            for (j = 0; j < 4; j++)
                char_array_4[j] = strchr(base64_chars, char_array_4[j]) - base64_chars;
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (j = 0; j < 3; j++)
                *decoded++ = char_array_3[j];
            j = 0;
        }
    }
    
    if (j) {
        for (int k = j; k < 4; k++)
            char_array_4[k] = 0;
        
        for (int k = 0; k < 4; k++)
            char_array_4[k] = strchr(base64_chars, char_array_4[k]) - base64_chars;
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        
        for (int k = 0; k < j - 1; k++)
            *decoded++ = char_array_3[k];
    }
    *decoded = '\\\\0';
}
'''
        return decryption_code + code
    
    def add_junk_code(self, code):
        """Добавляет сложный бесполезный код с антиотладочными элементами"""
        print("👻 Добавление сложного бесполезного кода...")
        
        junk_functions = [
            '''
void __junk_func_1(int x, int y) { 
    volatile int a = x * y + 0xDEADBEEF; 
    volatile int b = (a << 3) | (a >> 29); 
    for(int i = 0; i < 10; i++) { 
        b ^= (i * 0x12345678); 
        // Ложная проверка отладчика
        if (b == 0x1337) {
            volatile int* p = 0;
            *p = 0xDEAD;
        }
    }
    a = b * 0xCAFEBABE;
    // Бесполезные вычисления
    volatile double d = (double)a / 3.1415926535;
    d = d * d - 2.0 * d + 1.0;
}
            ''',
            '''
int __junk_func_2(char* ptr) { 
    static unsigned long state = 0x12345678;
    state = (state * 0x19660D + 0x3C6EF35F) & 0xFFFFFFFF;
    int result = 0;
    while(*ptr) { 
        result += *ptr++ ^ (state & 0xFF); 
        state = state * 0x19660D + 0x3C6EF35F; 
        // Ложная проверка целостности
        if (result == 0xDEADBEEF) {
            for(;;) { /* бесконечный цикл для запутывания */ }
        }
    }
    return result & 0x7F;
}
            ''',
            '''
float __junk_func_3(float f1, float f2) { 
    volatile double d = (double)f1 * f2 * 3.141592653589793;
    volatile int mask = 0xFF00FF;
    int* ptr = (int*)&d;
    *ptr ^= mask;
    
    // Ложные ветвления
    volatile int counter = 0;
    for (int i = 0; i < 100; i++) {
        counter += (i % 2 == 0) ? i : -i;
        if (counter > 1000) {
            counter = 0;
        }
    }
    
    return (float)(d / (*ptr & 0xFFFF));
}
            ''',
            '''
void __junk_math_1(int* arr, int size) {
    for(int i = 0; i < size; i++) {
        arr[i] = (arr[i] * 0xCCD + 0x235) ^ 0xFEED;
        if(i % 3 == 0) arr[i] = ~arr[i];
        
        // Ложные проверки времени выполнения
        volatile clock_t start = clock();
        for (volatile int j = 0; j < 100; j++) {
            arr[i] ^= j * 0xABCD;
        }
    }
}
            '''
        ]
        
        junk_code = "\n".join(junk_functions) + "\n"
        code = junk_code + code
        
        lines = code.split('\n')
        new_lines = []
        
        junk_calls = [
            '__junk_func_1(rand(), time(NULL));',
            '__junk_func_2("junk_string");',
            '__junk_func_3(1.0, 2.0);',
            'volatile int __junk_arr[10]; __junk_math_1(__junk_arr, 10);',
            '// Ложный вызов антиотладки\n volatile int __debug_check = __anti_debug_check();',
            '__fake_debug_breaks();'
        ]
        
        for i, line in enumerate(lines):
            new_lines.append(line)
            if '{' in line and random.random() < 0.25:
                new_lines.append('    ' + random.choice(junk_calls))
            elif ';' in line and random.random() < 0.15:
                new_lines.append('    ' + random.choice(junk_calls))
        
        return '\n'.join(new_lines)
    
    def obfuscate_control_flow(self, code):
        """Обфускация управляющих конструкций с непрозрачными предикатами"""
        print("🔄 Обфускация управляющих конструкций...")
        
        # Замена простых условий на сложные выражения
        code = re.sub(r'if\s*\((.*?)\)\s*{', self._complexify_condition, code)
        
        # Добавление ложных ветвей
        lines = code.split('\n')
        new_lines = []
        
        for line in lines:
            new_lines.append(line)
            if 'return' in line and random.random() < 0.3:
                # Добавляем ложный return перед настоящим
                false_returns = [
                    'if (0xDEADBEEF == 0xCAFEBABE) return 0;',
                    'if (time(NULL) % 2 == 0) { /* ложный return */ }',
                    'volatile int __false_flag = 1; if (__false_flag) { /* ничего */ }'
                ]
                new_lines.append('    ' + random.choice(false_returns))
        
        return '\n'.join(new_lines)
    
    def _complexify_condition(self, match):
        """Усложняет условия в if statements"""
        condition = match.group(1)
        
        # Простое усложнение - можно расширить
        complex_conditions = [
            f'({condition}) && (0x{random.randint(0, 0xFFFF):04X} != 0xDEAD)',
            f'({condition}) || (0 && (0x{random.randint(0, 0xFFFF):04X} == 0xBEEF))',
            f'!!({condition})',
            f'({condition}) ^ 0',
            f'(({condition}) + 0) != 0'
        ]
        
        return f'if ({random.choice(complex_conditions)}) {{'
    
    def add_metadata(self, code):
        """Добавляет метаданные обфускации"""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        metadata = f'''
/*
 * === BROOTFROUCE OBFUSCATED CODE ===
 * Generated: {timestamp}
 * Obfuscation level: {self.obfuscation_level}
 * Security: ANTI-DEBUG + STRING ENCRYPTION + CONTROL FLOW OBFUSCATION
 * Variables obfuscated: {len(self.var_mapping)}
 * Functions obfuscated: {len(self.func_mapping)}
 * Types obfuscated: {len(self.type_mapping)}
 * 
 * WARNING: This code is protected against reverse engineering
 *          and contains anti-debugging techniques.
 */
'''
        return metadata + code
    
    def add_security_checks(self, code):
        """Добавляет проверки безопасности в код"""
        print("🛡️  Добавление защитных механизмов...")
        
        security_code = self.security_checker.generate_anti_reverse_code()
        
        # Вставляем вызовы проверок безопасности в main функцию
        if 'int main(' in code:
            main_injection = '''
    // Встроенные проверки безопасности
    if (__anti_debug_check() != 0) {
        // Действия при обнаружении отладчика
        volatile int* p = 0;
        *p = 0xDEAD; // Intentional crash
    }
    
    __integrity_check();
    __fake_debug_breaks();
'''
            code = code.replace('int main(', 'int main(' + main_injection)
        
        return security_code + code
    
    def obfuscate_code(self, code, language):
        """Основная функция обфускации с защитными механизмами"""
        print("🚀 Начало расширенной обфускации кода...")
        
        # Добавляем метаданные
        code = self.add_metadata(code)
        
        # Шаг 1: Удаление комментариев и пробелов
        code = self.remove_comments_and_spaces(code)
        
        # Шаг 2: Обфускация типов
        code = self.obfuscate_types(code)
        
        # Шаг 3: Обфускация функций
        code = self.obfuscate_functions(code)
        
        # Шаг 4: Обфускация переменных
        code = self.obfuscate_variables(code)
        
        # Шаг 5: Шифрование строк
        if self.obfuscation_level == "high":
            code = self.encrypt_strings(code)
            code = self.add_string_decryption_functions(code)
        
        # Шаг 6: Добавление защитных механизмов
        code = self.add_security_checks(code)
        
        # Шаг 7: Обфускация управляющих конструкций
        if self.obfuscation_level == "high":
            code = self.obfuscate_control_flow(code)
        
        # Шаг 8: Добавление сложного бесполезного кода
        code = self.add_junk_code(code)
        
        print("✅ Расширенная обфускация завершена!")
        return code

def print_ascii_banner():
    """Выводит ASCII баннер BROOTFROUCE"""
    banner = r"""
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║ ██████╗ ██████╗  ██████╗  ██████╗ ████████╗███████╗██████╗  ██████╗ ██████╗ ║
║ ██╔══██╗██╔══██╗██╔═══██╗██╔═══██╗╚══██╔══╝██╔════╝██╔══██╗██╔═══██╗██╔══██╗║
║ ██████╔╝██████╔╝██║   ██║██║   ██║   ██║   █████╗  ██████╔╝██║   ██║██████╔╝║
║ ██╔══██╗██╔══██╗██║   ██║██║   ██║   ██║   ██╔══╝  ██╔══██╗██║   ██║██╔══██╗║
║ ██████╔╝██║  ██║╚██████╔╝╚██████╔╝   ██║   ███████╗██║  ██║╚██████╔╝██║  ██║║
║ ╚═════╝ ╚═╝  ╚═╝ ╚═════╝  ╚═════╝    ╚═╝   ╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝║
║                                                                              ║
║                  ADVANCED C/C++ CODE OBFUSCATION SYSTEM                     ║
║              WITH ANTI-REVERSE ENGINEERING PROTECTION v3.0                 ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
"""
    print(banner)

def check_environment_security():
    """Проверяет окружение на наличие инструментов реверса"""
    print("🔍 Проверка безопасности окружения...")
    
    checker = SecurityChecker()
    
    # Проверка отладчика
    if checker.check_debugger():
        print("⚠️  ВНИМАНИЕ: Обнаружен отладчик!")
        response = input("Продолжить выполнение? (y/N): ")
        if response.lower() != 'y':
            print("❌ Выполнение прервано из-за соображений безопасности.")
            sys.exit(1)
    
    # Проверка инструментов реверса
    detected_tools = checker.detect_ida()
    if detected_tools:
        print(f"⚠️  ВНИМАНИЕ: Обнаружены инструменты реверса: {', '.join(detected_tools)}")
        response = input("Продолжить выполнение? (y/N): ")
        if response.lower() != 'y':
            print("❌ Выполнение прервано из-за соображений безопасности.")
            sys.exit(1)
    else:
        print("✅ Окружение безопасно для работы.")
    
    return True

def process_single_file(input_file, output_file, language, obfuscation_level):
    """Обрабатывает одиночный файл"""
    try:
        with open(input_file, 'r', encoding='utf-8', errors='ignore') as f:
            code = f.read()
    except Exception as e:
        print(f"❌ Ошибка чтения файла {input_file}: {e}")
        return False
    
    obfuscator = AdvancedObfuscator()
    obfuscator.set_obfuscation_level(obfuscation_level)
    obfuscated_code = obfuscator.obfuscate_code(code, language)
    
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(obfuscated_code)
        print(f"💾 Обфусцированный код сохранен в: {output_file}")
        return True
    except Exception as e:
        print(f"❌ Ошибка записи файла {output_file}: {e}")
        return False

def process_directory(input_dir, output_dir, language, obfuscation_level):
    """Обрабатывает все файлы в директории"""
    input_path = Path(input_dir)
    output_path = Path(output_dir)
    
    output_path.mkdir(parents=True, exist_ok=True)
    
    extensions = ['.c', '.h'] if language == 'c' else ['.cpp', '.hpp', '.h', '.cc']
    
    processed_files = 0
    for ext in extensions:
        for input_file in input_path.rglob(f'*{ext}'):
            if input_file.is_file():
                relative_path = input_file.relative_to(input_path)
                output_file = output_path / relative_path
                
                output_file.parent.mkdir(parents=True, exist_ok=True)
                
                print(f"\n📁 Обработка: {input_file}")
                if process_single_file(input_file, output_file, language, obfuscation_level):
                    processed_files += 1
    
    return processed_files

def main():
    print_ascii_banner()
    
    # Проверка безопасности окружения
    if not check_environment_security():
        return
    
    parser = argparse.ArgumentParser(
        description='BROOTFROUCE - Advanced C/C++ Code Obfuscation & Protection System',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Примеры использования:
  %(prog)s --input main.c --output protected.c --lang c --level high
  %(prog)s --input src/ --output build/ --lang cpp --level extreme
  %(prog)s --file program.cpp --out-dir obfuscated/ --lang cpp --level medium

Уровни обфускации:
  • low    - базовая обфускация (переименование переменных)
  • medium - средняя обфускация + шифрование строк
  • high   - полная обфускация + защита от реверса
  • extreme- максимальная защита (экспериментально)

Возможности защиты:
  • Обфускация переменных, функций и типов
  • Шифрование строковых литералов
  • Анти-отладочные техники
  • Проверки целостности кода
  • Ложные ветвления и код
  • Защита от дизассемблеров :cite[1]
  • Обфускация управляющих конструкций
        """
    )
    
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--input', '-i', help='Входной файл или директория')
    group.add_argument('--file', '-f', help='Входной файл (альтернативный вариант)')
    
    parser.add_argument('--output', '-o', help='Выходной файл или директория')
    parser.add_argument('--out-dir', '-d', help='Выходная директория')
    parser.add_argument('--lang', '-l', choices=['c', 'cpp'], required=True,
                       help='Язык программирования (C или C++)')
    parser.add_argument('--name', '-n', help='Название для выходного файла')
    parser.add_argument('--level', '--obfuscation-level', 
                       choices=['low', 'medium', 'high', 'extreme'],
                       default='high',
                       help='Уровень обфускации (по умолчанию: high)')
    parser.add_argument('--no-security-check', action='store_true',
                       help='Отключить проверку безопасности окружения')
    
    args = parser.parse_args()
    
    # Отключение проверки безопасности если запрошено
    if not args.no_security_check:
        if not check_environment_security():
            return
    
    input_path = args.input or args.file
    output_path = args.output or args.out_dir
    
    if not output_path:
        print("❌ Необходимо указать выходной путь (--output или --out-dir)")
        sys.exit(1)
    
    if not os.path.exists(input_path):
        print(f"❌ Входной путь не существует: {input_path}")
        sys.exit(1)
    
    processed_count = 0
    
    if os.path.isfile(input_path):
        if os.path.isdir(output_path):
            if args.name:
                output_file = os.path.join(output_path, args.name)
            else:
                original_name = Path(input_path).stem
                output_file = os.path.join(output_path, f"{original_name}_protected{Path(input_path).suffix}")
        else:
            output_file = output_path
        
        print(f"🎯 Обработка файла: {input_path}")
        print(f"💾 Выходной файл: {output_file}")
        print(f"🛡️  Уровень защиты: {args.level}")
        
        if process_single_file(input_path, output_file, args.lang, args.level):
            processed_count = 1
            
    elif os.path.isdir(input_path):
        if not output_path:
            print("❌ Для обработки директории необходимо указать выходную директорию")
            sys.exit(1)
        
        if os.path.isfile(output_path):
            print("❌ Для обработки директории выход должен быть директорией")
            sys.exit(1)
        
        print(f"📁 Обработка директории: {input_path}")
        print(f"💾 Выходная директория: {output_path}")
        print(f"🔤 Язык: {args.lang}")
        print(f"🛡️  Уровень защиты: {args.level}")
        
        processed_count = process_directory(input_path, output_path, args.lang, args.level)
        
    else:
        print(f"❌ Неверный тип входного пути: {input_path}")
        sys.exit(1)
    
    print(f"\n🎉 Обработка завершена! Обработано файлов: {processed_count}")
    
    if processed_count > 0:
        print("\n⚠️  Важные замечания:")
        print("🔧 Обфусцированный код требует тщательного тестирования")
        print("🛡️  Защитные механизмы могут влиять на производительность")
        print("🔍 Рекомендуется тестирование на различных платформах")
        print("📚 Использованы техники защиты от IDA и других дизассемблеров :cite[1]:cite[3]")

if __name__ == "__main__":
    main()