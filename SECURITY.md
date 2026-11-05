# Security policy

## Supported versions

Security fixes target the latest published release. Older releases and development artifacts are not maintained separately. Please check whether a problem still affects the latest release when practical.

## Report a vulnerability privately

Do not publish vulnerabilities, exploit details, receiver identity keys, or sensitive logs in public issues or pull requests.

Use GitHub's **Security → Advisories → Report a vulnerability** for this repository:

https://github.com/lu3kyc0mb0/air2win/security/advisories/new

If private reporting is unavailable, do not post the details publicly. You may open an issue asking the maintainer to enable private vulnerability reporting, without describing the vulnerability.

Include the affected Air2Win version or commit, Windows and sender OS versions, reproduction steps, expected impact, and a minimal proof of concept if available. Redact personal information and secrets. Do not attach your entire `data/` folder.

Air2Win bundles UxPlay, Qt, and other libraries (see [third-party notices](THIRD_PARTY_NOTICES.md)). Identify the affected component if known; reports affecting the shipped package are welcome here. Coordinate disclosure privately with the maintainer before publishing technical details. Response and fix times depend on maintainer availability.

## Network use

Air2Win receives connections from devices on the local network. Use it on networks you trust, allow firewall access only where needed, and stop the receiver when it is not in use. PIN protection does not replace network isolation or security updates.
