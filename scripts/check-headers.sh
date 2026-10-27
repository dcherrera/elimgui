#!/bin/sh
# SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 David (Corey) Herrera
#
# Verify every source file ADDED in a range carries the SPDX pair and the file
# header block. See CONTRIBUTING.md.
#
#   scripts/check-headers.sh [range]     # default: origin/main..HEAD
#
# Only files the range ADDS are checked. Files that already exist are left
# alone deliberately: the rule is that everyone submitting code stays in
# compliance, not that the whole tree gets retrofitted.
#
# Exempt: tests, generated sources, and files carrying upstream third-party
# attribution (those keep their own header).
#
# Run it locally before opening a pull request; CI runs the same script.

set -eu

cd "$(git rev-parse --show-toplevel)"

RANGE="${1:-${BASE_REF:-origin/main}..HEAD}"

files=$(git diff --diff-filter=A --name-only "$RANGE" -- \
	'*.c' '*.h' '*.cu' '*.cuh' '*.m')

if [ -z "$files" ]; then
	echo "check-headers: ok (no source files added)"
	exit 0
fi

fail=0

note() {
	echo "check-headers: $1" >&2
	echo "               $2" >&2
	fail=$((fail + 1))
}

OLDIFS=$IFS
IFS='
'
for f in $files; do
	IFS=$OLDIFS

	# Added and then removed again within the range.
	[ -f "$f" ] || continue

	case "$f" in
	tests/*|*/tests/*) continue ;;
	lib/*) continue ;;   # vendored third-party code (lib/<name>/ + its LICENSE)
	*_ptx.h|*_ptx.cuh) continue ;;
	esac

	# Third-party-derived files keep their upstream attribution.
	if head -5 "$f" | grep -qE 'SPDX-FileCopyrightText|Apache-2\.0'; then
		continue
	fi

	if ! head -3 "$f" | grep -q 'SPDX-License-Identifier'; then
		note "$f: missing the SPDX identifier" \
		     "add: /* SPDX-License-Identifier: AGPL-3.0-or-later */"
		continue
	fi

	if ! head -16 "$f" | grep -q '@file'; then
		note "$f: missing the @file header block" \
		     "run: ./scripts/add-file-headers.sh --apply $f"
		continue
	fi

	brief=$(head -16 "$f" | sed -n 's|.*@brief[[:space:]]*||p' | head -1)
	case "$brief" in
	'')
		note "$f: @brief is empty" \
		     "one line saying what this file is for"
		;;
	TODO*|todo*)
		note "$f: @brief is still TODO" \
		     "replace it with one line saying what this file is for"
		;;
	esac

	for tag in @status @issues @todo; do
		if ! head -16 "$f" | grep -q -- "$tag"; then
			note "$f: header block is missing $tag" \
			     "all three of @status/@issues/@todo are present, 'None' when empty"
		fi
	done
done
IFS=$OLDIFS

if [ "$fail" -gt 0 ]; then
	echo "check-headers: $fail problem(s) — see CONTRIBUTING.md" >&2
	exit 1
fi

echo "check-headers: ok"
