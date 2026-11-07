# Signing the release

The release uses the free options only (decision D8 of [`plan.md`](plan.md)). There is no paid certificate and no
notarization, so a downloaded zip still makes the operating systems ask for confirmation. This page says what is signed,
what users see, and how to apply for the one signature that is still missing.

| Platform | What the release does | What the user sees |
|---|---|---|
| all | `SHA256SUMS` and a GitHub provenance attestation of each zip | nothing, until they verify it |
| macOS | ad-hoc `codesign -s -` of every binary and library | Gatekeeper blocks a zip downloaded in a browser |
| Windows | SignPath Foundation signature, once accepted | SmartScreen warns until then |

## Verify a download

```sh
sha256sum --check --ignore-missing SHA256SUMS
gh attestation verify aylo-engine-<version>-<os>-<arch>.zip --repo Charles-Mahoudeau/rtype
```

The attestation proves the zip was built by the release workflow of this repository at a given commit.

## macOS

`.github/scripts/codesign-macos.sh <folder>` signs every Mach-O file of a folder with an ad-hoc signature, and
`--verify` checks them. The package task runs it after stripping the binaries and before it writes `manifest.json` and
the zip: `strip` invalidates the signature the linker made, and an arm64 binary without a valid signature is killed by
the kernel. The release workflow then verifies every binary of the extracted zip.

An ad-hoc signature is not an identity. A zip downloaded in a browser carries the quarantine flag, and Gatekeeper
refuses its binaries ("cannot be opened because the developer cannot be verified"). The user removes the flag once, on
the extracted folder:

```sh
xattr -dr com.apple.quarantine aylo-engine-<version>-macos-arm64/
```

Downloads made with `gh` or `curl` are not quarantined. Only notarization, which needs a paid Apple Developer account,
removes the prompt; it is out of scope. This command belongs in the README of the zip and in the "Shipping your game"
page of the manual (REL-09): a game developer shipping the runtime gives their players the same instruction, or signs
and notarizes their own build.

## Windows

An unsigned `.exe` makes SmartScreen show "Windows protected your PC": the user clicks **More info**, then **Run
anyway**. [SignPath Foundation](https://signpath.org/) signs open-source projects for free; the workflow has the steps
for it, switched off until the repository defines its variables.

### Apply

This is a manual step, not done by the release code.

1. Check the conditions on <https://signpath.org/>: the repository is public, under an OSI-approved license, with a
   documented release process (the workflow and [`release-process.md`](release-process.md)) and no malware or
   proprietary components in what is signed. State that the project is a game engine.
2. Apply with the repository, the license, the maintainers, and the three zips of a release as the artifacts to sign.
3. When accepted, on SignPath create the project and a signing policy, with an artifact configuration that signs the
   `.exe` and `.dll` files inside a zip.
4. In the GitHub repository, add the secret `SIGNPATH_API_TOKEN` and the variables `SIGNPATH_ORGANIZATION_ID`,
   `SIGNPATH_PROJECT_SLUG` and `SIGNPATH_SIGNING_POLICY_SLUG`.

From then on, the Windows job uploads the unsigned zip, waits for SignPath to sign it and continues with the signed one.
No variable, no signing step: the steps are skipped.

### Known limit

SignPath signs the finished zip, after the package task wrote `manifest.json`. The manifest keeps the hashes of the
files as they were before the signature, so the signed `.exe` and `.dll` differ from it. To fix it the package task would
have to be split in two (stage, then zip and write the manifest after the signature). Until then the manifest of a signed
Windows zip is stale for those files.
