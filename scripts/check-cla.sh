#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 David (Corey) Herrera
#
# Verify every commit author in a range appears on the CLA signature roster.
# See CLA.md (individuals) and CLA-CORPORATE.md (organizations).
#
#   scripts/check-cla.sh [range]        # default: origin/main..HEAD
#
# Run it locally before opening a pull request; CI runs the same script.

set -eu

cd "$(git rev-parse --show-toplevel)"

RANGE="${1:-${BASE_REF:-origin/main}..HEAD}"
ROSTER=".cla/signatures.md"

if [ ! -f "$ROSTER" ]; then
	echo "check-cla: $ROSTER is missing" >&2
	exit 1
fi

# Signed addresses: every `git-email:` value that is not inside a comment.
signed=$(grep -v '^[[:space:]]*#' "$ROSTER" |
	sed -n 's/^[[:space:]]*-\{0,1\}[[:space:]]*git-email:[[:space:]]*//p' |
	tr -d '\r' | tr '[:upper:]' '[:lower:]' | sort -u)

authors=$(git log --no-merges --format='%ae' "$RANGE" |
	tr '[:upper:]' '[:lower:]' | sort -u)

if [ -z "$authors" ]; then
	echo "check-cla: no commits in $RANGE — nothing to check"
	exit 0
fi

fail=0
for email in $authors; do
	if printf '%s\n' "$signed" | grep -qxF "$email"; then
		echo "ok   $email"
	else
		echo "FAIL $email  not on the CLA roster"
		fail=1
	fi
done

if [ "$fail" -ne 0 ]; then
	cat <<'EOF'

Contributions are accepted under the Contributor License Agreement in CLA.md.
Sign it by adding your entry to .cla/signatures.md in this pull request:

  - name: Your Full Name
    git-email: you@example.com
    forge-username: your-username
    date: YYYY-MM-DD
    agreement: ICLA v1.0
    statement: I have read the elimgui ICLA and I agree to it.

Use the same address you commit with. If you commit under more than one
address, list each on its own git-email: line. Organizations contributing on
behalf of employees sign CLA-CORPORATE.md by email instead.

EOF
	exit 1
fi

echo "check-cla: all authors have signed"
