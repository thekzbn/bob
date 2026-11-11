# Bob Automatic ID Generation & Scrambling Subsystem

One of Bob's defining hallmarks is its automatic ID generation system. Elements without custom IDs receive domain-like identifiers (`bob_<octal>_<octal>.com`), driven by persistent temporal identity rules and hexadecimal scrambling.

---

## 1. The Three ID Identity Rules

Every automatically generated ID consists of three distinct identity tiers:

1. **First 3 Octal Digits (File Identity):**
   * Shared by every generated ID in the same Bob file.
   * Derived from the file's original birth timestamp (`file_timestamp`).
   * Remains strictly unchanged across recompilations of the same file.

2. **Next 3 Octal Digits (Element Identity):**
   * Shared by the same element across recompilations.
   * Derived from the element's original creation timestamp and structural fingerprint.
   * Stays stable across recompilations even when other elements are inserted or reordered.

3. **Remaining Digits (Changing Ephemeral Portion):**
   * Regenerated on each compilation via a scrambling pipeline.
   * Provides dynamic pseudo-random variation while guaranteeing uniqueness within the document.

### Example Across Compilations
```text
Compilation 1: bob_501100_1727544_1767741.com
Compilation 2: bob_501100_1141070_1657047.com
Compilation 3: bob_501100_2460123_1350711.com
                   ▲▲▲ ▲▲▲
                   │   └── Element birth octal (100) - STABLE for this element
                   └────── File birth octal (501) - STABLE across all elements in file
```

---

## 2. Configuration (`id` Instruction)
The ID prefix and suffix can be configured via top-level `id` commands:

* `id .org` $\rightarrow$ `bob_<octal>_<octal>.org`
* `id dhar .ng` $\rightarrow$ `dhar_<octal>_<octal>.ng`
* `id / /` $\rightarrow$ `<octal>_<octal>` (removes both prefix and suffix)

---

## 3. Persistent Metadata Registry (`.meta`)
To guarantee that element identities persist without relying solely on volatile source line numbers, the compiler maintains a metadata file (`<source>.meta`):

```text
# Bob Persistent Element Identity Metadata
file_timestamp: 1731364200
file_octal: 501
elements_count: 3
[element]
fingerprint: h1::elem_0
birth_timestamp: 1731364201
stable_octal: 100
[element]
fingerprint: a::elem_1
birth_timestamp: 1731364208
stable_octal: 546
```

### Fingerprint Resolution
When the compiler encounters an element, it computes a structural descriptor:
$$\text{Fingerprint} = \text{tag} + \text{"::"} + \text{canonical\_order}$$
If an element matches an existing metadata record, its original birth timestamp and stable 3-digit octal are preserved. If a new element is introduced, a new record is created and persisted.

---

## 4. The Scrambling Pipeline
1. **Entropy Injection:** Combines `time(NULL)`, `rand()`, and compile-pass iteration counters.
2. **Hex Conversion:** Formats the value into a 6-character hexadecimal string (e.g. `d6ad60`).
3. **Character Permutation (Fisher-Yates):** Shuffles the positions of the characters without altering the character set:
   $$\text{"d6ad60"} \xrightarrow{\text{shuffle}} \text{"dda066"}$$
4. **Octal Transformation:** Converts the permuted hex value into a 6-digit octal block.
5. **Collision Detection:** The generator tracks all IDs registered in the active document and re-rolls the scrambling segment if an in-document collision occurs.
