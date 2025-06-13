# LightEdit 🌙 – Lightweight C++ Game Code Editor

![LightEdit](https://raw.githubusercontent.com/lul-v3/LightEdit/refs/heads/main/github/img/LightEditPreview.png?token=GHSAT0AAAAAADERC7IMCUBPZCHCPFGSKMWG2CLQXSQ)

**LightEdit** is a modern, lightweight C++ code editor built for SDL3/OpenGL/GLEW projects — ideal for game development, low-level rendering, or small indie engines.

> ✨ Built with **C++17**, **SDL3**, **OpenGL/GLEW**, **ImGui (Docking)** and **ImGuiColorTextEdit**

---

## 🔧 Features

- 🌲 **Project Tree View** with folder/file structure
- 📄 **Tabbed Interface** with multiple editor instances
- 💾 **Hotkeys** like `F5` (Build & Run) and `Ctrl+S` (Save)
- ⚙️ **Integrated Build System** (customizable per platform)
- 🎨 **Modern Docking UI** using ImGui with full viewport support
- 🧠 **Dirty-State Indicator** (`*` on unsaved files)
- 📦 **No runtime dependencies** (can be statically linked)

---

## 📸 Screenshots


### 🧱 Project Explorer + Tabbed Editor  
![Project Explorer](https://raw.githubusercontent.com/lul-v3/LightEdit/refs/heads/main/github/img/ProjectExplorer.png?token=GHSAT0AAAAAADERC7INB5LML2IONWZKL3FG2CLRBHA)

### 🧑‍💻 Editor View with Syntax Highlighting  
![Code Editor](https://raw.githubusercontent.com/lul-v3/LightEdit/refs/heads/main/github/img/CodeEditor.png?token=GHSAT0AAAAAADERC7INMWE446TJUSZ5KB4A2CLQ7IQ)

### ⚙️ Build Output / Console Panel  
![Build Output](https://raw.githubusercontent.com/lul-v3/LightEdit/refs/heads/main/github/img/BuildOutput.png?token=GHSAT0AAAAAADERC7IMBNAFJO46YQWZBP5I2CLQ7JA)

---

## 🛠️ Build Instructions

### Requirements

- C++17 compatible compiler (g++, clang++, MSVC)
- SDL3
- OpenGL (e.g., GLEW)
- ImGui with Docking support
- ImGuiColorTextEdit (included or added manually)

### Linux (g++)

```bash
sudo apt install libsdl3-dev libglew-dev
git clone https://github.com/yourname/LightEdit.git
cd LightEdit
mkdir build && cd build
cmake ..
make
./LightEdit
```
## Windows (MSVC)

- Open the project in Visual Studio 2022+

- Make sure all dependencies (``SDL3``, ``GLEW``, ``ImGui``, ``ColorTextEdit``) are included

- Press ``F5`` to build and run

# 🚧 TODO / Roadmap

- [x] Close tabs via middle-click or 'X' button
- [ ] Right-click context menu in project tree
- [ ] Syntax highlighting for different file types
- [ ] Command Palette (Ctrl+P, fuzzy search)
- [ ] Integrated terminal
- [ ] CMake or Makefile integration
- [ ] File auto-reload (watch for external changes)
- [ ] Light/Dark theme selector
...

# 🙏 Credits

- [Dear ImGui](https://github.com/ocornut/imgui) – UI framework for fast editor tools

- [Custom ImGuiColorTextEdit Fork](https://github.com/lul-v3/ImGuiColorTextEdit) – Modified version of the original C++ text editor (original version is ~6 years old)

- [SDL3](https://github.com/libsdl-org/SDL) – Cross-platform input/windowing backend

- [GLEW](https://glew.sourceforge.net/) + OpenGL – Fast, hardware-accelerated rendering

# 💡 Motivation

LightEdit was born out of the need for a lightweight, customizable C++ code editor tailored for OpenGL/SDL3 game and engine development — without the complexity of full-blown IDEs like Visual Studio or Qt Creator.

It is modular by design and aims to eventually become a tooling base for game developers and engine creators alike.

# 📜 License

MIT License – Free for personal and commercial use.