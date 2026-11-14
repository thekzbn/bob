# Bob

**A tiny esoteric language that turns source files into web pages.**

I started Bob because I was bored and wanted to try making an esolang for the web. It grew into a small compiler written in C: Bob source goes in, and standalone HTML or split HTML/CSS/JavaScript files come out.

It’s an experiment in language design, browser output, and finding out how much of a web page can be described with syntax that looks a little like a conversation.

## A tiny example

```bob
page: bg #0b2e1e #f5f5dc 'Inter'

center h1(greeting): "Hello from Bob."
center button(hello_btn): "Click me"

when click(hello_btn) make-->
    greeting: "It worked."
<--
```

## Installation & Quick Start

### Via npm (Recommended for macOS, Linux, and Windows)

Zero build setup or C compiler required:

```sh
# Install globally
npm install -g bob-lang

# Compile a Bob file
bobc app.bob -o dist/index.html
```

Or run directly without installing:

```sh
npx bob-lang app.bob -o dist/index.html
```

### Build from source

You need a C compiler such as GCC or Clang, and `make` for the commands below.

```sh
git clone https://github.com/thekzbn/bob.git
cd bob
make
make test
```

Compile an example to a standalone HTML page:

```sh
mkdir -p dist
./bobc.exe examples/interactive_greeting.bob -o dist/index.html
```

Or emit separate files:

```sh
mkdir -p dist
./bobc.exe examples/interactive_greeting.bob --split dist
```

Open the generated HTML in a browser. Create the output directory first; the compiler currently expects it to exist.

## What Bob can do

- Describe elements, IDs, attributes, nested content, and page styling
- Use variables, arithmetic, conditions, and event handlers
- Read inputs and update text or visibility
- Add reusable classes and interoperate with JavaScript
- Generate one standalone HTML file or split HTML, CSS, and JavaScript output

Bob is a web-focused language experiment, not a replacement for JavaScript or a general-purpose language.

## Generated IDs

Elements without a custom ID can receive a generated ID. The current format combines metadata-derived digits with a changing generated portion. That means the full ID can change between compilations, even when some metadata stays the same. Treat this feature as experimental: don't rely on generated IDs as permanent public identifiers or security tokens. See [the ID generator notes](docs/ID_GENERATOR.md) for the design and current implementation details.

## Docs

- [Getting started](docs/getting_started_guide.html)
- [Syntax reference](docs/SYNTAX_REFERENCE.md)
- [Syntax ambiguities and design decisions](docs/AMBIGUITIES_AND_DECISIONS.md)
- [Automatic ID generation](docs/ID_GENERATOR.md)
- [Compiler architecture](docs/architecture_and_design_system.html)

The syntax reference is the place to look up current language behavior; the other docs go deeper into implementation and design decisions.

## Status

Bob is early-stage and experimental. The compiler builds and the checked-in tests pass, but syntax and behavior may change. The current test suite is a useful start, not a guarantee that every program works in every browser.

## License

Bob is licensed under the [GNU Affero General Public License v3.0](LICENSE).
