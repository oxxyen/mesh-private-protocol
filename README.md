<h1 align="center">
  🔒 <b>MESH SECURITY LABS</b> 🔒
</h1>

<h3 align="center">
  <i>Anonymity. Safety. Control.</i>
</h3>

<p align="center">
  <img src="https://img.shields.io/badge/Status-Internal_Development-red?style=for-the-badge&logo=github"/>
  <img src="https://img.shields.io/badge/Encryption-E2EE-green?style=for-the-badge&logo=lock"/>
  <img src="https://img.shields.io/badge/Network-P2P-blue?style=for-the-badge&logo=peercoin"/>
</p>

 Mesh — Anonymous P2P messenger development (internal repository)
<p align="center">
  🔒───🔐───🛡️───🕵️‍♂️───🧩<br>
  │&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│<br>
  🧩───🕵️‍♂️───🛡️───🔐───🔒<br>
  │&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;│<br>
  🔐───🛡️───🕵️‍♂️───🧩───🔒
</p>

> ⚠️ **This repository is intended exclusively for the team's developers. Not for public use.**

**Mesh** is an internal code base for the development of an anonymous P2P messenger with an emphasis on security, local storage and minimal vulnerability.  
The project uses C/C++ for the main logic, SQLite for data storage, Python for auxiliary tools: assembly, testing, obfuscation and debugging.

---
```
## 📁 Project structure
├── src/ → Main server code (C)
├── assets/ → Client code (C++, Qt / GUI)
├── p2p_messenger/ → Test P2P server + client + alarm protocol
├── tests_scripts/ → Autotests and verification utilities
├── trash/ → Temporary files, experiments, drafts
,── meshctl.py → Main control script: build, run, debug
,── brootfrouce.py → Source code obfuscation script (for IP protection)
├── mesh_tester.py → Automated tests (integration, unit tests)
├── compile.sh → Quick Build (wrapper over CMake)
├── Makefile → Build under Linux/Unix
,── CMakeLists.txt → The main CMake file of the project
├── README.md → This document
└── .env → Environment variables for development
```
---

### ⚙️ Scripts
At the root of the repository are key Python scripts for automating the team's daily tasks.:

- **`meshctl.py *** is a universal project controller: build, server startup, debugging, cleaning, and environment management.  
- **`brootfrouce.py `** — Obfuscation of C/C++ code to protect intellectual property during external transfer.  
- **`mesh_tester.py `** — Automated tests: checking P2P connections, encryption integrity, SQLite compatibility, and error tolerance.

## 🛠️ Technologies

| Component | Is used for |
|---------------------|------------------|
| **C++17 / C99** | Server core, Networking, Cryptography |
| **SQLite3**         | Local storage of messages, metadata, keys |
| **Python 3.9+** | Project Management (`meshctl.py `), obfuscation (`brootfrouce.py `), testing (`mesh_tester.py `) |
| **Qt (optional)**| GUI client in `assets/`     |
| **SignalProtocol**  | Signal protocol(libsignal-protocol-c) |
| **libsodium / OpenSSL** | Message encryption, key generation, authentication |

 All components must be assembled locally. No external cloud services or dependencies should be included in the final build.

---

## 🚀 Project launch and management

### 🔧 Main tool: `meshctl.py `

A script for managing the project lifecycle (everything is described and it is clear how to use it):

```bash
python3 meshctl.py --help
``` 

## Code obfuscation: `brootfrouce.py `

Script **`brootfrouce.py '** is designed to protect the source code from reverse engineering, static analysis, and unauthorized copying.

- Used **before release builds** or when transferring code to external parties (for example, the QA team or partners).
- Performs renaming variables, deleting comments, and distorting the code structure without changing the logic.
- Works with the input directories and generates an obfuscated copy in the specified output folder.

```bash
python3 brootfrouce.py --help
```
### 🖥️ Architecture

#### 📌 `src/` — Server part  
The main server logic is written in C/C++.

- **`server.c`** — main server(MAIN FILE): listens to ports, manages sessions and message routing, signal protocol, DDOS attack protection, encryption, token generation, local data storage, security.   
- **`server.h`** — header files: declarations of functions, data structures, and interfaces.  
- **`MeshServer.hpp`** — C++ classes and interfaces  
- **`server_wrapper.h`** — wrappers for FFI (Foreign Function Interface), allowing to call server functions from Python or other languages.

---

#### 📌 `assets/` — Client part  
GUI client based on Qt (C++).

- **`chatclient.cpp `**, **`mainwindow.cpp `** — implementation of the chat logic and graphical interface.  
- **`main.cpp '** is the entry point of the client application.  
- **`ChatClient.pro `** is a project file for Qt Creator (contains dependencies and build settings).

---

#### 📌 `p2p_messenger/` — Test module  
A prototype of P2P interaction for debugging the signaling protocol and network logic.

- **`p2p_messenger.c.c`**, **`main.c`** — simplified implementations of the P2P client and signaling server.  
- **`network.c`** — work with sockets, connection processing, implementation of the signaling protocol.  
- **`store.c`** — temporary storage of messages in memory or on disk.  
- **`p2p_messenger.db`** is a test SQLite database for emulating local storage.

> 🔁 This module is used **only for development and testing** — not included in the final build.

---

### 📜 License

This project is a **proprietary intellectual property of MESH SECURITY LABS**.

- 🔒 **All code, architecture, and algorithms are copyrighted.**  
- *** It is prohibited** to copy, modify, distribute, decompile or use **in any form** without the written permission of the copyright holder.  
- 🌐 The public repository is intended **exclusively for the internal development of the team**.  
 Any unauthorized use will be prosecuted in accordance with the law.

> ✉️ For licensing issues, please contact: **[@test002227](https://t.me/test002227 )**

---

## Communication with the team

> **Developed by**  
> **<span style="color:#00ff00; font-weight:bold">MESH SECURITY LABS</span>n>**  
> *Privacy is not an option, but a foundation.*

---

- 🧠 **Lead Developer**:
[@test002227](https://t.me/test002227 ) — *architect of anonymity*

- 📡 **Official Channel**:  
  [@mesh_im](https://t.me/mesh_im ) — *Updates, announcements, safe discussions*

- 🌐 **Web portal**:
[**justccode.github.io/mesh-web**](https://justccode.github.io/mesh-web /) — *documentation, philosophy, download*

---

> 🔒 **Important**:
> We do not use email, forms or trackers.  
> The only safe way is **Telegram**.  
> Your words remain between you and us. No logs. There are no traces.
