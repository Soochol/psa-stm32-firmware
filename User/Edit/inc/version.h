#ifndef __JH_VERSION_H
#define __JH_VERSION_H

// STM32 firmware version, reported verbatim in reqDeviceVersion(0x46). Bumped
// by hand at release only, and carries a -dev suffix in between: the response
// has no build-timestamp field, so without the marker two different in-progress
// builds are indistinguishable on the wire. Drop the suffix when releasing.
// See CLAUDE.md, Versioning.
//
// The wire field is 16 B ASCII with 0x00 padding, and all-zero is reserved by
// the spec for "unknown" -- so this string must never be empty.
#define STM_FW_VERSION		"1.0.1-dev"

// The wire field is 16 B; sizeof counts the terminating NUL the wire omits.
_Static_assert(sizeof(STM_FW_VERSION) <= 17, "STM_FW_VERSION exceeds the 16 B wire field");

#endif
