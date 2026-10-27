# elimgui CLA signatures

This file is the signature roster for the elimgui Contributor License
Agreement. Adding your entry here, in a pull request signed off by you, **is
your signature** — see [`CLA.md`](../CLA.md) for the agreement itself and
[`CLA-CORPORATE.md`](../CLA-CORPORATE.md) for organizations.

Do not edit or remove anyone else's entry. Entries are append-only; the git
history of this file is the record of who agreed, to what, and when.

The CI check in `.github/workflows/contributions.yml` matches the **`git-email`**
values below against the author address on every commit in a pull request. If
you commit under more than one address, list each on its own `git-email:` line.

---

## Individual signatures

```yaml
- name: David (Corey) Herrera
  git-email: coreyh55@gmail.com
  forge-username: dcherrera
  commit-name: Zero          # git author name on this project's history — same person
  role: project owner and copyright holder
  date: 2026-08-11
  agreement: ICLA v1.0
  statement: I have read the elimgui ICLA and I agree to it.
```

<!-- Add your entry above this line, following the format exactly. -->

## Corporate signatures

Organizations sign by email (see `CLA-CORPORATE.md`); their entry is added here
after countersignature, with one `git-email:` line per authorized contributor
from Schedule A.

```yaml
# - organization: Example, Inc.
#   agreement: CCLA v1.0
#   signed: YYYY-MM-DD
#   schedule-a-updated: YYYY-MM-DD
#   contributors:
#     - name: Contributor Name
#       git-email: contributor@example.com
#       forge-username: their-username
```
