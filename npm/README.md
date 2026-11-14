# bob-lang

The official npm distribution for **Bob**, an experimental, line-based esoteric programming language designed for web development.

Bob unifies HTML structure, CSS styling, and reactive JavaScript logic into one unconventional, semicolon-free syntax that compiles directly into browser-ready web applications.

---

## Quick Start

### Global Installation

```bash
npm install -g bob-lang
```

After installation, the `bobc` (and alias `bob`) command will be available globally:

```bash
bobc app.bob -o dist/index.html
```

### Run with `npx` (No Global Install)

```bash
npx bob-lang app.bob -o dist/index.html
```

---

## Supported Platforms

Pre-compiled, zero-dependency native binaries are included for:
* **macOS:** Apple Silicon (`darwin-arm64`) and Intel (`darwin-x64`)
* **Linux:** `linux-x64` and `linux-arm64`
* **Windows:** `win32-x64`

No C compiler (GCC/Clang) or build tools are required on the client machine when installing via npm.

---

## Basic Syntax Preview

```bob
page: bg #0b2e1e #f5f5dc 'Inter'

center div(card): h1: "Hello Bob", button(btn): "Click Me"

when click(btn) make-->
    btn: "Clicked!"
    whisper: "Hello from Bob runtime!"
<--
```

Compile into a standalone HTML application:

```bash
bobc input.bob -o index.html
```

---

## Documentation & Repository

* Complete syntax guide and language reference: [GitHub Repository](https://github.com/ayomide/bob)
* License: MIT
