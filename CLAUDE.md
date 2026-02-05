# CLAUDE.md

Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.


# Coding Style

Code to be generalized and multiply functional not specific. Don't
hard code calculations as final values - show the calculation in
compile time constant expressions with variable names or comments or
both.

Use camelCase instead of snake_case everywhere and the "m_" prefix for
instance members is not to be used. I want 2-space indentation rules,
one space after each C++ keyword like if, for, switch, while, etc.
before the following expression in parentheses. I want to use locally
declared variables in for loops instead of declaring these outside the
for loop where possible. I want generic index variables to be int. I
want to use C string variables and arrays instead of std::string or
std::string_view where possible to avoid all the verbosity and
conversions to and from C string representation. Make all template
parameters names all uppercase. Instead of names like "n_call" use
e.g., "nCall".

To ensure consistency across
separate chat sessions and prevent context bloat, we will adhere to
these core architectural rules.

* Indentation: 2-space indentation.

* Naming: camelCase for all variables/functions. No m_ prefix for
members.

* Templates: Template parameters must be ALLUPPERCASE.

* Memory: Manual management via Thread-Local Arenas for routing passes.

* Strings: Use C-style strings (const char*) and buffers to avoid
std::string overhead. If there's a big advantage of using std::string
instead, ask me my preference.

* Constants: Use constexpr to show the derivation of values (no "magic
numbers").

* Primitives: Prefer int for generic indices and fixed-point math for
geometry.


# Dictionary for Type Translation

* java.awt.geom.Point2D -->  struct Vector2<T> (Custom template).

* java.util.List<T> -->  std::vector<T> (Contiguous memory).

* java.util.Map<K, V> -->  std::flat_map<K, V> (C++23; cache-friendly).

* java.awt.geom.Area -->  GeometryRegion (Clipping-based structure).

* java.util.concurrent -->  std::jthread and std::atomic (C++20/23).

Consider use of std::mdspan to view a flat piece of memory as a
multi-dimensional array without performance loss.

# Phases

To minimize impact of context limits, execute the rewrite in Modular Phases.

Phase 1: Foundation Primitives (Vectors, Matrices, Bounding Boxes).

Phase 2: KiCad S-Expression Parser (Replacing the Java DSN parser).

Phase 3: Topological Map (The core "Connectivity Graph").

Phase 4: The Push-and-Shove Engine (Recursive search logic).

Phase 5: UI & Visualization (Dear ImGui integration).

# Source

The Java we're translating - the freerouting.app code - is in the ./freerouting.app directory. 
