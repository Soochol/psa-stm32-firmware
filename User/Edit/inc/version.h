#ifndef __JH_VERSION_H
#define __JH_VERSION_H

// STM32 firmware version, reported verbatim in reqDeviceVersion(0x46) and
// bumped by hand on every release. The wire field is 16 B ASCII with 0x00
// padding, and all-zero is reserved by the spec for "unknown" -- so this
// string must never be empty.
#define STM_FW_VERSION		"1.0.0"

// The wire field is 16 B; sizeof counts the terminating NUL the wire omits.
_Static_assert(sizeof(STM_FW_VERSION) <= 17, "STM_FW_VERSION exceeds the 16 B wire field");

#endif
