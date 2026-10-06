# Reporting a vulnerability

Avoid putting sensitive exploit details in a public issue. Check the repository's
Security tab for a private vulnerability reporting option. Its availability
depends on the repository owner's configuration; this file does not enable it.
If unavailable, contact the maintainers privately using the contact details in
`pyproject.toml` to arrange disclosure.

Use the current release. Historical versions before this modernization contain
known native-wrapper memory handling defects. The README describes decoder
allocation limits and format compatibility. This block format does not provide
integrity or authenticity; use application-level verification when needed.
