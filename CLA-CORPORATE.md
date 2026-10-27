# elimgui Corporate Contributor License Agreement

**Version 1.0**

Use this agreement if contributions to elimgui are being made **by employees or
contractors on behalf of an organization** that owns the resulting intellectual
property. Individuals contributing on their own behalf should use
[`CLA.md`](./CLA.md) instead.

This agreement mirrors the Individual CLA. Its purpose is to make sure that the
entity that actually owns the work has granted the rights, so that neither of us
is relying on an individual employee's authority to license their employer's IP.

**The organization keeps its copyright.** This is a license, not an assignment.

---

## 1. Definitions

**"I"**, **"me"**, **"my"** — David (Corey) Herrera, the copyright holder of elimgui,
and my successors and assigns.

**"You"**, **"your"** — the organization signing this agreement, together with
all entities that control, are controlled by, or are under common control with
it. "Control" means ownership of more than 50% of outstanding shares, or the
power to direct the entity's management.

**"Contribution"** — any original work of authorship, including any modification
or addition to an existing work, intentionally submitted to me for inclusion in
elimgui by you or by anyone authorized to submit on your behalf, except
anything conspicuously marked **"Not a Contribution."**

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
your Contribution to others under **any terms I choose**: the project's current
outbound license (AGPL-3.0-or-later), a different open-source license if the project's
outbound license changes, or **proprietary or closed-source terms**.

I may also **assign or transfer** these rights, in whole or in part, together
with the project or with the copyrights in it, to a successor, acquirer, or
foundation, who then holds them on these same terms. A sale, merger, or
relicensing of elimgui does not require me to come back to you for permission.

I am not obligated to use your Contribution, and I may modify or remove it at
any time.

## 5. Your representations

You represent that:

1. **You own it and can license it.** Each Contribution is an original creation
   owned by you, and the person signing below is authorized to bind you to this
   agreement.
2. **Your people are authorized.** The individuals listed in Schedule A are
   authorized to submit Contributions on your behalf. You will keep that list
   current, and you remain responsible for Contributions submitted by anyone
   you have authorized, whether or not they are listed.
3. **No hidden third-party code.** Contributions do not knowingly include
   anyone else's code, except as identified under section 6.
4. **You'll tell me** if any of the above stops being true.

## 6. Third-party submissions

Work that is not your original creation may be submitted only if it is
identified separately in the pull request, retains its original copyright and
license notices, and is conspicuously marked
`Submitted on behalf of a third party: <author> — license: <SPDX id>`. I may
decline it. **GPL- or AGPL-only third-party code cannot be accepted**, because
it would prevent the project from being relicensed in the future.

## 7. No warranty, no obligation

Contributions are provided **"AS IS"**, without warranty or condition of any
kind, express or implied, to the maximum extent permitted by law. You are not
expected to provide support, maintenance, or updates.

## 8. Governing law

This agreement is governed by the laws of the State of Missouri, United States,
without regard to its conflict-of-law rules.

## 9. Entire agreement

This is the entire agreement between us about your Contributions, and it
supersedes any earlier understanding on the subject. If any part of it is held
unenforceable, the rest stays in force.

---

## How to sign

Corporate signatures are handled by email rather than by pull request, because
they need a signature from someone authorized to bind the organization.

1. Email **coreyh55@gmail.com** with the subject `elimgui CCLA — <organization>`,
   including the completed block below and Schedule A. A scanned signature, a
   digital signature, or a reply from an authorized signer's corporate email
   address all work.
2. Once countersigned, I add an entry for your organization to
   [`.cla/signatures.md`](./.cla/signatures.md), listing each authorized
   contributor's commit email so CI can verify their pull requests.
3. Your contributors still sign off every commit with the DCO (`git commit -s`)
   — see [`CONTRIBUTING.md`](./CONTRIBUTING.md).

```
Organization:      ____________________________________________
Mailing address:   ____________________________________________
Point of contact:  ____________________________________________
Contact email:     ____________________________________________

Authorized signer
  Name:            ____________________________________________
  Title:           ____________________________________________
  Signature:       ____________________________________________
  Date:            ____________________________________________

Agreement:         elimgui CCLA v1.0
```

### Schedule A — authorized contributors

| Full name | Commit email | Forge username |
|-----------|--------------|----------------|
|           |              |                |

To add or remove people later, email an updated Schedule A. Contributions from
someone not covered by a current Schedule A will be held until the list is
updated.
