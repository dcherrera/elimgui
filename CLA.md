# elimgui Individual Contributor License Agreement

**Version 1.0**

Thank you for contributing to elimgui.

This agreement lets me include your contribution in elimgui and distribute it
under the project's licenses — the one it ships under today (AGPL-3.0-or-later) and any license the
project moves to later.

**You keep your copyright.** This is a license, not an assignment. Your
contribution stays yours: you can use it, publish it, sell it, or license it to
anyone else, anywhere, forever, on any terms you like.

Sign once — see [How to sign](#how-to-sign) at the end. It covers everything you
contribute to elimgui, before and after signing.

---

## 1. Definitions

**"I"**, **"me"**, **"my"** — David (Corey) Herrera, the copyright holder of elimgui,
and my successors and assigns.

**"You"**, **"your"** — the individual who signs this agreement. If you are
signing on behalf of an organization, use
[`CLA-CORPORATE.md`](./CLA-CORPORATE.md) instead.

**"Contribution"** — any original work of authorship, including any modification
or addition to an existing work, that you intentionally submit to me for
inclusion in elimgui. "Submit" means any form of electronic, verbal, or written
communication sent to me or to the project — pull requests, patches, issue
comments, mailing-list or chat messages — except anything you conspicuously mark
**"Not a Contribution."**

## 2. Copyright license

You grant me a perpetual, worldwide, non-exclusive, no-charge, royalty-free,
irrevocable copyright license to reproduce, prepare derivative works of,
publicly display, publicly perform, sublicense, and distribute your Contribution
and derivative works of it.

## 3. Patent license

You grant me a perpetual, worldwide, non-exclusive, no-charge, royalty-free,
irrevocable (except as stated in this section) patent license to make, have
made, use, offer to sell, sell, import, and otherwise transfer your
Contribution. This license covers only those patent claims licensable by you
that are necessarily infringed by your Contribution alone, or by the combination
of your Contribution with elimgui.

If any entity institutes patent litigation alleging that your Contribution, or
elimgui itself, constitutes direct or contributory patent infringement, then
every patent license granted to that entity under this agreement terminates as
of the date the litigation is filed.

## 4. Outbound licensing — what I may do with your Contribution

The licenses in sections 2 and 3 include the right to license and sublicense
your Contribution to others under **any terms I choose**. Specifically:

- under the project's current outbound license — AGPL-3.0-or-later;
- under a different open-source license, if the project's outbound license
  changes;
- under **proprietary or closed-source terms**, as part of a commercial license
  or a product.

I may also **assign or transfer** the rights you grant here, in whole or in
part, together with the project or with the copyrights in it — to a successor,
an acquirer, or a foundation. Whoever holds them then holds them on these same
terms. A sale, merger, or relicensing of elimgui does not require me to come
back to you for permission.

I am not obligated to use your Contribution, and I may modify or remove it at
any time.

## 5. Your representations

You represent that:

1. **It's yours.** Each Contribution is your original creation and you are
   legally entitled to grant the licenses above.
2. **Your employer is handled.** If your employer has rights to intellectual
   property you create, you have either received permission to contribute on
   your own behalf, or your employer has waived those rights, or your employer
   has signed the Corporate CLA.
3. **No hidden third-party code.** Your Contribution does not knowingly include
   anyone else's code, except as identified under section 6.
4. **You'll tell me** if any of the above stops being true.

## 6. Third-party submissions

You may submit work that is not your original creation only if you:

- identify it separately in the pull request,
- keep its original copyright and license notices intact, and
- mark it conspicuously as
  `Submitted on behalf of a third party: <author> — license: <SPDX id>`.

I may decline it. Whether it can be accepted at all depends on its license
permitting commercial distribution alongside the rest of the tree — **GPL- or
AGPL-only third-party code cannot be accepted**, because it would prevent the
project from being relicensed in the future.

## 7. No warranty, no obligation

Your Contribution is provided **"AS IS"**, without warranty or condition of any
kind, express or implied, to the maximum extent permitted by law. You are not
expected to provide support, maintenance, or updates for it.

## 8. Governing law

This agreement is governed by the laws of the State of Missouri, United States,
without regard to its conflict-of-law rules.

## 9. Entire agreement

This is the entire agreement between us about your Contributions, and it
supersedes any earlier understanding on the subject. If any part of it is held
unenforceable, the rest stays in force.

---

## How to sign

**1. Add yourself to [`.cla/signatures.md`](./.cla/signatures.md)** as part of
your first pull request:

```yaml
- name: Your Full Name
  git-email: you@example.com
  forge-username: your-username
  date: YYYY-MM-DD
  agreement: ICLA v1.0
  statement: I have read the elimgui ICLA and I agree to it.
```

Use the **same email address you commit with** — the CI check matches on it. If
you commit under more than one address, list each on its own `git-email:` line.

**2. Sign off every commit** with the DCO (`git commit -s`) — see
[`CONTRIBUTING.md`](./CONTRIBUTING.md).

That pull request, carrying your entry and signed off by you, **is your
signature**. The git history is the record: who agreed, to what version, and
when. Nothing to print, scan, or email.

If you would rather not sign, open an issue describing your change instead of a
pull request. An idea costs nothing to accept.

---

*Questions about this agreement: coreyh55@gmail.com. If something here is
ambiguous, ask rather than guessing — I would rather clarify it than have
someone rely on a reading I did not intend.*
