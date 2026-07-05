"""PlatformIO pre-build hook: inject PANPILOT_FW_VERSION from git (base spec §13).

Adds -D PANPILOT_FW_VERSION="<git describe>" so About/boot show semver+hash.
Runs for device envs; harmless if git is absent (falls back to a dev string).
"""
Import("env")  # noqa: F821  (injected by PlatformIO/SCons)
import subprocess


def git_version() -> str:
    try:
        return (
            subprocess.check_output(
                ["git", "describe", "--tags", "--always", "--dirty"],
                stderr=subprocess.DEVNULL,
            )
            .decode()
            .strip()
        )
    except Exception:
        return "0.0.0-nogit"


env.Append(CPPDEFINES=[("PANPILOT_FW_VERSION", env.StringifyMacro(git_version()))])
