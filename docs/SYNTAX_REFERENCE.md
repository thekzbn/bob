# Bob Language Syntax Reference

Bob is an esoteric programming language for web development that compiles directly into standard HTML, CSS, and JavaScript.

---

## 1. Program Structure
* **Line-based:** Each instruction occupies its own line.
* **No Semicolons:** Statement boundaries are determined by newlines.
* **No Indentation Scoping:** Scope is governed by explicit arrow markers (`make-->` and `<--`), not indentation.
* **Comments:** Begin with `//` and extend to the end of the line.

```bob
// This is a comment
car = 1 * 1
whisper car
```

---

## 2. Variables & The 0 vs 00 Distinction
Bob has an intentional distinction between an uninitialized variable and numeric zero:

* `variable = 0`: Declares a variable in an uninitialized state (compiles to JS `undefined`).
* `variable = 00`: Assigns numeric zero `0` (compiles to JS `0`).
* `variable = 5`: Assigns numeric `5`.
* `variable = "text"`: Assigns a string.
* `variable = 1 * 1`: Evaluates arithmetic expression.

### Single `=` Semantics
A single `=` handles all assignment and comparison operations:
1. **In statements:** `car = 5` (assignment)
2. **In conditional headers:** `if car = 5 make-->` (equality check `car === 5`)
3. **In command expressions:** `whisper car = 1 * 1` (evaluates, assigns, and passes value)

---

## 3. Strings & Multi-Line Literals

### Single-Line Strings
Enclosed in double quotes `"..."`.
```bob
name = "bobo"
tell: "Hello world"
```

### Multi-Line Strings
Enclosed in triple double quotes `""" ... """`.
* Preserves raw newlines and literal indentation.
* Automatic HTML character escaping (`<`, `>`, `&`).
* Bob syntax inside triple-quoted strings is treated as pure text, never evaluated.
* Rendered in elements with `white-space: pre-wrap; word-break: break-word;`.

```bob
tell: """
This is a long paragraph.

It spans multiple lines cleanly.
Code like car = 5 inside here is literal text!
"""
```

---

## 4. Output Commands
* `whisper: expr` or `whisper expr`: Emits to browser console (`console.log`).
* `yell: expr` or `yell expr`: Emits a browser modal alert (`alert`).
* `tell: expr`: At the top level, creates a `<p>` paragraph element containing the text.

---

## 5. Element Creation, Attributes & Modifiers

### Complete Anatomical Form
```bob
[element_modifiers] tag(custom_id)[attributes]: [content] [content_modifiers]
```

* **Element Modifiers (Left of tag):**
  * `center`: Centers element as block with auto margins.
  * `left`: Aligns left.
  * `right`: Aligns right.
* **Custom IDs:**
  * Enclosed in parentheses immediately following tag: `button(my_btn): "Click"`
* **HTML Attributes:**
  * Square brackets immediately follow tag/ID: `tag[key="value", boolean_key, data-id="123"]`
  * Supports standard attributes (`href`, `target`, `type`, `src`, `rel`), boolean attributes (`disabled`, `checked`, `required`), data attributes (`data-theme="dark"`), and ARIA attributes (`aria-label="Action"`).
* **Content Modifiers (Right of colon):**
  * Colors: Standard color names or hexadecimal `#1e90ff`.
  * Backgrounds: `bg #1e90ff` or `bg color`.
  * Fonts: Quoted font name `'Inter'`, `'JetBrains Mono'`.
  * Typography: `bold`, `italic`.
  * Font sizes: Numbers like `1.25` (emitted as `1.25rem`).
  * Visibility: `hide` (`display: none;`).

Examples:
```bob
a(nav_link)[href="https://example.com", target="_blank"]: "Visit Example" #1e90ff bold
input(email_input)[type="email", required]: "user@domain.com"
button(cta)[disabled]: "Processing..."
```

---

## 6. DOM Nesting

### Inline Sibling Nesting (Rule C)
To create nested children without closing tags:
```bob
div: h1: "Hello", a: "Visit my website", button: "Click me"
button: "Outside the div"
```
* The first tag (`div`) is the parent container.
* Each comma `,` introduces another direct sibling child belonging to that parent.
* The end of the line terminates the parent container.
* The next line resumes at the outer sibling level.

### Arbitrary-Depth Block Nesting
Use `make-->` and `<--` to open and close element children blocks to any depth:
```bob
article make-->
    section make-->
        h1: "System Architecture"
        div make-->
            p: "Nested paragraph with deep hierarchy"
            a[href="/docs", target="_blank"]: "Learn more" #1e90ff
        <--
    <--
<--
```
* Visual indentation is optional; `<--` explicitly closes the matching parent element.
* Inline comma children can be freely combined inside block elements.

---

## 7. SVG Elements
Bob has native support for SVG tags.
* Supported tags: `svg`, `circle`, `rect`, `path`, `line`, `polygon`, `polyline`, `ellipse`, `g`, `text`, `defs`, `use`.
* Root `svg` elements automatically receive the SVG namespace (`xmlns="http://www.w3.org/2000/svg"`).
* Empty SVG shapes self-close (`<circle ... />`).

```bob
svg[viewBox="0 0 100 100", width="100", height="100"] make-->
    circle[cx="50", cy="50", r="40", fill="#1e90ff"]
    path[d="M 30 50 L 45 65 L 70 35", stroke="#ffffff", stroke-width="4", fill="none"]
<--
```

---

## 8. Tables
Tables use the standard element and nesting syntax without requiring custom keywords:

```bob
table make-->
    thead make-->
        tr: th: "Feature", th: "Status", th: "Engine"
    <--
    tbody make-->
        tr: td: "Multi-line", td: "Supported", td: "Lexer"
        tr: td: "SVG & Tables", td: "Supported", td: "Codegen"
        tr: td: "JS Interop", td: "Supported", td: "Runtime"
    <--
<--
```
Bob automatically emits clean table defaults (`border-collapse: collapse; width: 100%;`).

---

## 9. CSS Classes & Dynamic Class Mutations

### Class Styling Selectors
Define reusable class styling at top-level with `.classname:` syntax:
```bob
.panel: bg #111115 #f5f5dc 'Inter'
.badge: bg #1e90ff #ffffff bold
```

### Runtime Class Mutations
Add, remove, or toggle classes dynamically at runtime:
```bob
add_class target "class-name"
remove_class target "class-name"
toggle_class target "class-name"
```

* `target` can be an element ID, `this` inside an event listener, or `root` / `document` for `<html>`:
```bob
when click(theme_btn) make-->
    toggle_class root "dark"
<--
```

---

## 10. Global Page & All Selectors
* `page: bg #0b2e1e #f5f5dc 'Inter'`: Applies styling to `<body>`.
* `all: bg #0b2e1e #f5f5dc 'Inter'`: Applies styling to all elements (`*`).

---

## 11. Conditionals & Logic Blocks
```bob
if car = 1 make-->
    whisper: "First car"
or else if car = 2 make-->
    whisper: "Second car"
or else-->
    whisper: "No car"
<--
```

---

## 12. Events & Interactive Updates
```bob
when click(hello_btn) make-->
    // Directly modifies the clicked element
    hello_btn: "Clicked!"
    whisper: "Button was pressed"
<--
```

### Reading and Updating Input Fields
```bob
center input(search_box): "Type to search..."
when click(submit_btn) make-->
    query = read search_box
    whisper: query
    search_box: "" // Clears input value
<--
```

---

## 13. JavaScript Interoperability & Browser Storage

### JS Expressions
Pass JavaScript expressions directly into Bob variables using `js`:
```bob
saved_theme = js localStorage.getItem("theme")
client_w = js window.innerWidth
current_time = js Date.now()
```

### JS Single-Line Statements
Execute JavaScript statements directly:
```bob
js: localStorage.setItem("theme", "dark")
js: console.time("render")
```

### JS Multi-Line Code Blocks
Execute arbitrary multi-line JavaScript blocks using `js: """ ... """`:
```bob
js: """
console.log("Bob interop initialized");
window.addEventListener("resize", () => {
    console.log("Window resized to:", window.innerWidth);
});
"""
```
