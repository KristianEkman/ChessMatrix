# Copilot Instructions

When working in this repository, follow test-driven development by default.

## TDD workflow

1. Start by reproducing the requested behavior or bug with a test before changing production code.
2. Make the smallest code change needed to get that new or updated test passing.
3. Run the narrowest relevant test first, then the broader suite once the focused test passes.
4. Refactor only after the behavior is covered and passing.

## Test placement and style

- Put new tests under `src/tests/`.
- Use `TEST(Name)` for core behavior and `SEARCH_TEST(Name)` for search-related behavior.
- Match the existing test style: short function-style test names, focused setup, direct assertions.
- Prefer existing helpers such as `Assert`, `AssertAreEqualInts`, `AssertAreEqualLongs`, `AssertBestMove`, `AssertBestMoveTimed`, and FEN-based test setup.
- Keep each test targeted at one behavior. Do not combine unrelated scenarios into one test.
- For chess-position bugs, prefer a minimal FEN that reproduces the issue.
- Keep tests deterministic. Avoid introducing randomness, time-sensitive expectations, or broad benchmark assertions unless the existing test pattern already requires them.

## Execution order

- For a new or failing test, run the exact test first with `make test ExactTestName`.
- The equivalent form `make test testName=ExactTestName` is also valid.
- After the focused test passes, run `make test`.
- If a change is limited to a clearly isolated area, avoid unrelated refactors while getting the test green.

## Editing expectations

- Do not change assertions just to make a failing test pass unless the expected behavior was proven wrong.
- Prefer adding or updating tests before touching files under `src/`.
- Keep production changes minimal and local to the behavior under test.
- When fixing a regression, leave behind a test that would fail without the fix.

## Repo-specific guidance

- Search behavior can be timing-sensitive. Prefer exact-behavior assertions over timed assertions unless the test already depends on a time budget.
- Use exact test-name filtering when iterating so failures are fast to reproduce.
- Follow the repository's existing C style and keep changes small enough that the new test explains why the production change exists.
