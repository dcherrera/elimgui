#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 David (Corey) Herrera
#
# Verify every commit in a range carries a Developer Certificate of Origin
# sign-off matching its author. See CONTRIBUTING.md.
#
#   scripts/check-dco.sh [range]        # default: origin/main..HEAD
#
# Run it locally before opening a pull request; CI runs the same script.

set -eu

cd "$(git rev-parse --show-toplevel)"

RANGE="${1:-${BASE_REF:-origin/main}..HEAD}"

commits=$(git rev-list --no-merges "$RANGE")
if [ -z "$commits" ]; then
	echo "check-dco: no commits in $RANGE — nothing to check"
	exit 0
fi

fail=0
for sha in $commits; do
	author_email=$(git show -s --format='%ae' "$sha")
	author_name=$(git show -s --format='%an' "$sha")
	subject=$(git show -s --format='%s' "$sha")

	signoffs=$(git show -s --format='%B' "$sha" |
		sed -n 's/^[Ss]igned-off-by:[[:space:]]*//p' |
		tr '[:upper:]' '[:lower:]')

	if [ -z "$signoffs" ]; then
		echo "FAIL $sha  $subject"
		echo "     no Signed-off-by line"
		fail=1
		continue
	fi

	want=$(printf '%s' "$author_email" | tr '[:upper:]' '[:lower:]')
	if printf '%s\n' "$signoffs" | grep -qF "<$want>"; then
		echo "ok   $sha  $author_name <$author_email>"
	else
		echo "FAIL $sha  $subject"
		echo "     sign-off does not match the author <$author_email>"
		printf '     found: %s\n' "$signoffs"
		fail=1
	fi
done

if [ "$fail" -ne 0 ]; then
	cat <<'EOF'

Every commit needs a Developer Certificate of Origin sign-off matching its
author -- see https://developercertificate.org/ and CONTRIBUTING.md.

  git commit -s -m "..."             new commits
  git commit --amend -s --no-edit    the most recent commit
  git rebase --signoff <base>        a whole branch

EOF
	exit 1
fi

echo "check-dco: all commits signed off"
