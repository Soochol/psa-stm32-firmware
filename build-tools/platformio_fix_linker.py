Import("env")

# Fix linker flags for hard float ABI
# PlatformIO sometimes doesn't pass FPU flags to linker correctly

print("=" * 60)
print("Fixing linker flags for hard float ABI (FPv5-D16)")
print("=" * 60)

# Remove any soft float flags from linker
env["LINKFLAGS"] = [
    flag for flag in env["LINKFLAGS"]
    if "soft" not in str(flag).lower()
]

# Ensure hard float FPU flags are in linker command
required_flags = [
    "-mcpu=cortex-m7",
    "-mthumb",
    "-mfpu=fpv5-d16",
    "-mfloat-abi=hard",
]

for flag in required_flags:
    if flag not in env["LINKFLAGS"]:
        env.Append(LINKFLAGS=[flag])
        print(f"  Added to LINKFLAGS: {flag}")

# Ensure specs are present
if "--specs=nano.specs" not in env["LINKFLAGS"]:
    env.Append(LINKFLAGS=["--specs=nano.specs"])
    print("  Added to LINKFLAGS: --specs=nano.specs")

if "--specs=nosys.specs" not in env["LINKFLAGS"]:
    env.Append(LINKFLAGS=["--specs=nosys.specs"])
    print("  Added to LINKFLAGS: --specs=nosys.specs")

print("\nFinal LINKFLAGS:")
print("  " + " ".join([str(f) for f in env["LINKFLAGS"]]))
print("=" * 60)
