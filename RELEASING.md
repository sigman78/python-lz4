# Release configuration and procedure

The workflow is `.github/workflows/ci.yml`. Pushes, pull requests, and manual
runs test/build only. A **published GitHub release** starts a fresh build and
can publish its tested artifacts to PyPI. Creating a tag or pushing a branch
alone does not publish. The tag must exactly equal `v` plus the project version.

The matrix tests CPython 3.11–3.14 on Linux, Windows, and macOS with
`PYTHONMALLOC=debug`. cibuildwheel tests installed wheels on Linux x86_64
(manylinux and musllinux), Windows AMD64, and macOS x86_64/arm64. It selects
normal CPython builds, excluding free-threaded builds. The source distribution
is installed and tested in a clean environment. Distribution artifacts from
each workflow run are available for review; remote checks must actually run
before platform compatibility is considered verified.

## One-time external setup

These settings must be configured by the repository/PyPI owner; adding this
workflow does not create them or publish anything.

1. Enable GitHub Actions in `sigman78/python-lz4` and create the GitHub
   environment named exactly `pypi` under repository Settings → Environments.
   Restrict deployment tags to release tags such as `v*`. Configure required
   reviewers if the maintainers want an additional release approval gate.
2. In the `lz4ext` project on PyPI, open Manage → Publishing and add a GitHub
   Trusted Publisher with these exact values:
   - Owner: `sigman78`
   - Repository: `python-lz4`
   - Workflow filename: `ci.yml` (filename only)
   - Environment: `pypi`
3. If the PyPI project does not exist or is not owned by the maintainer, resolve
   ownership first or register a pending publisher using these same fields.
   Do not substitute the unrelated `lz4` project. Forks must configure their
   own owner/repository and PyPI project before enabling publication.

See the [PyPI Trusted Publisher instructions](https://docs.pypi.org/trusted-publishers/adding-a-publisher/).
No PyPI password or API token is needed. OIDC `id-token: write` is granted only
to the isolated publish job, which does not check out or execute this source.
It downloads only this run's `dist-*` artifacts after every required job passes.
Build jobs have read-only repository permissions; pull requests do not publish.

## Release

1. Update `project.version` in `pyproject.toml`, the direct-build fallback in
   `src/python-lz4.c`, the version test, and the changelog.
2. Run `python -m build`, install the built wheel in a clean environment, run
   `python tests/test.py`, and run `python -m twine check dist/*` (install `build`
   and `twine` first). On PowerShell, enumerate artifacts for pip rather than
   assuming shell expansion of `*.whl`.
3. Push the reviewed commit and confirm the GitHub Actions test/build run passes.
4. Create the matching tag, for example `v1.0` for version `1.0`, and publish
   its GitHub release. Publishing the release is the action that enables PyPI
   upload. The workflow rebuilds and tests artifacts for that tagged commit.
5. Inspect the release run and PyPI files. Failed publishing should be diagnosed
   before rerunning; existing files on PyPI cannot be replaced. The workflow
   does not silently skip existing artifacts.

Actions are pinned to verified upstream commit IDs with version comments.
Review new action versions and update pins deliberately. No remote publication,
tag creation, or external-account configuration is performed by local builds.
