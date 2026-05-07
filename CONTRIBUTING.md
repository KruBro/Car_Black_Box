# Contributing to Car Black Box

Thank you for considering contributing! This document describes the process for reporting bugs, proposing features, and submitting code changes.

---

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How to Report a Bug](#how-to-report-a-bug)
- [How to Request a Feature](#how-to-request-a-feature)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Commit Message Format](#commit-message-format)
- [Pull Request Checklist](#pull-request-checklist)

---

## Code of Conduct

By contributing you agree to abide by the [Code of Conduct](CODE_OF_CONDUCT.md).

---

## How to Report a Bug

1. Search [existing issues](../../issues) to avoid duplicates.
2. Open a **Bug Report** using the issue template.
3. Include: hardware revision, compiler version, observed behaviour, expected behaviour, and minimal reproduction steps.

---

## How to Request a Feature

1. Open a **Feature Request** using the issue template.
2. Describe the problem you are solving, not just the solution.
3. If applicable, provide a wiring diagram or reference to a datasheet.

---

## Development Workflow

```
main          ← stable, release-ready
└── develop   ← integration branch
    └── feature/<short-name>   ← your branch
    └── fix/<issue-number>     ← bug fix branch
```

1. Fork the repository and clone your fork.
2. Branch off `develop`:
   ```bash
   git checkout develop
   git checkout -b fix/42-nack-ds1307
   ```
3. Make your changes following the coding standards below.
4. Push and open a Pull Request targeting `develop`.

---

## Coding Standards

This project targets **XC8 / PIC18** and follows a subset of **MISRA-C:2012**.

### Naming
| Entity | Convention | Example |
|---|---|---|
| Functions | `module_verb_noun` | `uart_putchar`, `i2c_read` |
| Macros / Constants | `UPPER_SNAKE_CASE` | `FOSC`, `SEC_ADDRESS` |
| Local variables | `lower_snake_case` | `clock_reg`, `curr_speed` |
| Types | `lower_snake_case_t` | `gear_state_t` |

### Rules
- **No implicit integer conversions** — cast explicitly.
- **All array indices must be `unsigned`.**
- **No empty busy-wait loops** — use `__delay_ms()` / `__delay_us()`.
- **Module-private functions must be `static`** and must **not** appear in the public header.
- **No reserved identifiers** — do not name functions `getchar`, `putchar`, `puts`, `printf`, etc.
- **One `#define FOSC` only** — in `main_config.h`.
- Every public function must have a **Doxygen block** in the `.c` file.
- Every file must begin with the standard file header (see template below).

### File Header Template
```c
/**
 * @file    module.c
 * @author  <GitHub username>
 * @date    YYYY-MM-DD
 * @brief   One-line description of the module.
 *
 * @details
 *   Extended description if needed.
 */
```

---

## Commit Message Format

```
<type>(<scope>): <short summary>

[optional body — wrap at 72 chars]

[optional footer — Closes #<issue>]
```

**Types:** `feat`, `fix`, `docs`, `refactor`, `test`, `chore`

**Examples:**
```
fix(ds1307): send NACK instead of ACK on single-byte read

The I2C spec requires the master to NACK after the last received byte.
Passing 0 (ACK) caused the DS1307 to attempt further transmission.

Closes #7
```

```
feat(adc): add multi-channel read helper
```

---

## Pull Request Checklist

Before marking your PR ready for review, confirm:

- [ ] Code compiles with zero warnings at `-Wall`
- [ ] All new public functions have Doxygen comments
- [ ] No new `FOSC` definitions outside `main_config.h`
- [ ] No stdlib identifier conflicts introduced
- [ ] `LOG.md` updated if a bug was found and fixed
- [ ] PR description explains *what* changed and *why*
- [ ] Target branch is `develop`, not `main`