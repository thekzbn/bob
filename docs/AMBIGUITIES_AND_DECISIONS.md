# Syntax Ambiguities & Implementation Decisions

During the design and prototyping of the Bob language compiler, several grammatical and semantic ambiguities were identified. Below is the full documentation of each ambiguity and the conservative prototype decision implemented.

---

### 1. Single `=` in Variable Assignment vs Equality Comparison vs Output Expressions
* **Ambiguity:** Bob uses a single `=` for both variable assignment (`car = 5`) and conditionals (`if car = 1 make-->`), and supports expressions inside commands (`whisper car = 1 * 1`).
* **Decision:**
  * In statement contexts (`<ident> = <expr>`), `=` compiles to variable assignment (`car = 5;`).
  * In conditional expression contexts (`if <expr> make-->` or `or else if <expr> make-->`), `=` compiles to JavaScript strict equality (`===`).
  * In expression/command contexts (`whisper car = 1 * 1`), `=` compiles to an assignment expression (`car = 1 * 1`) which updates the variable and passes the resulting value to the output command.

---

### 2. Output Command Colons
* **Ambiguity:** Some examples show colons on output commands (`whisper: "My name is bobo"`, `tell: "..."`), while other examples omit the colon (`whisper car = 1 * 1`).
* **Decision:** The parser treats the colon as **optional** after action keywords (`whisper`, `yell`, `read`, `show`, `hide`), while requiring it on element declarations to strictly divide element modifiers from content modifiers.

---

### 3. DOM Nesting & Child Sibling Boundaries (Rule C)
* **Ambiguity:** How to represent child hierarchies without indentation or ending tags.
* **Decision:** Implemented inline comma-separated declarations:
  ```bob
  div: h1: "Hello", a: "Visit my website", button: "Click me"
  ```
  * The first declaration is the parent.
  * Commas `,` separate direct sibling children belonging to that parent.
  * The newline `\n` ends parent scope; the following line begins a sibling outside the parent.
  * Commas within quoted strings (`"Hello, world"`) are tokenized as string literals and never split children.

---

### 4. Input Placeholder vs Live Value
* **Ambiguity:** An `<input>` followed by a string content modifier in initial declaration vs inside logic blocks.
* **Decision:**
  * At creation time, `input: "Enter your name..."` generates `<input placeholder="Enter your name..." />`.
  * In update blocks (`input: ""`), the compiler generates dynamic property updates (`element.value = ""`).

---

### 5. Event Target Implicit `this` Resolution
* **Ambiguity:** In `when click(button) make-->`, updating `button: "Confirm"` should target only the clicked button, but updating other tags should update all matching elements.
* **Decision:** The compiled JavaScript runtime passes `_event` to element update and read helpers. If the update target matches the event's `currentTarget` (by tag name or ID), the runtime modifies only that specific element.

---

### 6. Element Identity Persistence Across Edits
* **Ambiguity:** The spec mandates that inserting an element at the beginning of a file must not alter the identity of existing elements, and that element line numbers cannot be the sole identity criterion.
* **Decision:** Implemented a persistent `.meta` registry storing canonical element fingerprints (`tag::custom_id:content_prefix:sibling_index`) mapped to permanent birth timestamps and stable octals.
