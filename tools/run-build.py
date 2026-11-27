"""Two-slot, resource-bounded Windows build/expensive-tool runner (stdlib only).

Portable project copy of the accessibility-mod build-resource contract. The
kernel job contains the complete child tree before any child code can run.
"""
from __future__ import annotations

import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import mmap
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import uuid

DEFAULT_BYTES = 2_000_000_000
MUTEX_NAME = r"Global\Codex.Build.ResourceBudget.v1"
# Slot one keeps the original name: older serial runners occupy that slot and
# cannot race a new job into it. Never rename these per project or per chat.
SLOT_MUTEX_NAMES = (MUTEX_NAME, MUTEX_NAME + ".Slot2")
JOB_MEMORY = 0x200
KILL_ON_CLOSE = 0x2000
PRIORITY_LIMIT = 0x20
BELOW_NORMAL = 0x4000
SIZE_T = c.c_size_t


class BasicLimits(c.Structure):
    _fields_ = [("ProcessTime", c.c_longlong), ("JobTime", c.c_longlong),
                ("LimitFlags", w.DWORD), ("MinimumWorkingSet", SIZE_T),
                ("MaximumWorkingSet", SIZE_T), ("ActiveProcessLimit", w.DWORD),
                ("Affinity", SIZE_T), ("PriorityClass", w.DWORD),
                ("SchedulingClass", w.DWORD)]


class IoCounters(c.Structure):
    _fields_ = [(name, c.c_ulonglong) for name in
                ("ReadOps", "WriteOps", "OtherOps", "ReadBytes", "WriteBytes", "OtherBytes")]


class ExtendedLimits(c.Structure):
    _fields_ = [("Basic", BasicLimits), ("Io", IoCounters),
                ("ProcessMemoryLimit", SIZE_T), ("JobMemoryLimit", SIZE_T),
                ("PeakProcessMemoryUsed", SIZE_T), ("PeakJobMemoryUsed", SIZE_T)]


class CpuLimits(c.Structure):
    _fields_ = [("ControlFlags", w.DWORD), ("CpuRate", w.DWORD)]


class Accounting(c.Structure):
    _fields_ = [(name, c.c_longlong) for name in
                ("UserTime", "KernelTime", "PeriodUserTime", "PeriodKernelTime")] + [
                (name, w.DWORD) for name in
                ("PageFaults", "TotalProcesses", "ActiveProcesses", "TerminatedProcesses")]


class StartupInfo(c.Structure):
    _fields_ = [("cb", w.DWORD), ("Reserved", w.LPWSTR), ("Desktop", w.LPWSTR),
                ("Title", w.LPWSTR)] + [(name, w.DWORD) for name in
                ("X", "Y", "XSize", "YSize", "XChars", "YChars", "Fill", "Flags")] + [
                ("ShowWindow", w.WORD), ("ReservedBytes", w.WORD),
                ("ReservedData", c.c_void_p), ("StdInput", w.HANDLE),
                ("StdOutput", w.HANDLE), ("StdError", w.HANDLE)]


class ProcessInfo(c.Structure):
    _fields_ = [("Process", w.HANDLE), ("Thread", w.HANDLE),
                ("ProcessId", w.DWORD), ("ThreadId", w.DWORD)]


def api():
    if os.name != "nt":
        raise RuntimeError("No resource guard for this platform: do not run the build unbounded.")
    k = c.WinDLL("kernel32", use_last_error=True)
    signatures = {
        "CreateMutexW": ([c.c_void_p, w.BOOL, w.LPCWSTR], w.HANDLE),
        "ReleaseMutex": ([w.HANDLE], w.BOOL),
        "CreateJobObjectW": ([c.c_void_p, w.LPCWSTR], w.HANDLE),
        "OpenJobObjectW": ([w.DWORD, w.BOOL, w.LPCWSTR], w.HANDLE),
        "SetInformationJobObject": ([w.HANDLE, c.c_int, c.c_void_p, w.DWORD], w.BOOL),
        "QueryInformationJobObject": ([w.HANDLE, c.c_int, c.c_void_p, w.DWORD, c.c_void_p], w.BOOL),
        "IsProcessInJob": ([w.HANDLE, w.HANDLE, c.POINTER(w.BOOL)], w.BOOL),
        "GetCurrentProcess": ([], w.HANDLE),
        "GetStdHandle": ([w.DWORD], w.HANDLE),
        "DuplicateHandle": ([w.HANDLE, w.HANDLE, w.HANDLE, c.POINTER(w.HANDLE), w.DWORD, w.BOOL, w.DWORD], w.BOOL),
        "CreateProcessW": ([w.LPCWSTR, w.LPWSTR, c.c_void_p, c.c_void_p, w.BOOL, w.DWORD,
                            c.c_void_p, w.LPCWSTR, c.POINTER(StartupInfo), c.POINTER(ProcessInfo)], w.BOOL),
        "AssignProcessToJobObject": ([w.HANDLE, w.HANDLE], w.BOOL),
        "ResumeThread": ([w.HANDLE], w.DWORD),
        "WaitForSingleObject": ([w.HANDLE, w.DWORD], w.DWORD),
        "WaitForMultipleObjects": ([w.DWORD, c.POINTER(w.HANDLE), w.BOOL, w.DWORD], w.DWORD),
        "GetExitCodeProcess": ([w.HANDLE, c.POINTER(w.DWORD)], w.BOOL),
        "TerminateProcess": ([w.HANDLE, w.UINT], w.BOOL),
        "TerminateJobObject": ([w.HANDLE, w.UINT], w.BOOL),
        "CloseHandle": ([w.HANDLE], w.BOOL),
    }
    for name, (arguments, result) in signatures.items():
        function = getattr(k, name)
        function.argtypes, function.restype = arguments, result
    return k


def checked(value):
    if not value:
        raise c.WinError(c.get_last_error())
    return value


def query(k, job, kind, structure):
    value = structure()
    checked(k.QueryInformationJobObject(job, kind, c.byref(value), c.sizeof(value), None))
    return value


def verify_current(k):
    name = os.environ.get("CODEX_BUILD_JOB_NAME")
    if not name:
        raise RuntimeError("Use python scripts/run-build.py -- <build command>; an enforced build budget is required.")
    job = checked(k.OpenJobObjectW(4, False, name))
    try:
        inside = w.BOOL()
        checked(k.IsProcessInJob(k.GetCurrentProcess(), job, c.byref(inside)))
        limits = query(k, job, 9, ExtendedLimits)
        cpu = query(k, job, 15, CpuLimits)
        expected = int(os.environ.get("CODEX_BUILD_MEMORY_BYTES", "0"))
        minimum = 32_000_000 // mmap.PAGESIZE * mmap.PAGESIZE
        if not inside.value or not minimum <= expected <= DEFAULT_BYTES or limits.JobMemoryLimit != expected:
            raise RuntimeError("Build is not inside its declared hard memory limit.")
        if limits.Basic.LimitFlags & (JOB_MEMORY | KILL_ON_CLOSE) != JOB_MEMORY | KILL_ON_CLOSE:
            raise RuntimeError("Build job lacks enforced memory/descendant cleanup limits.")
        if cpu.ControlFlags & 5 != 5 or not 1 <= cpu.CpuRate <= 2500:
            raise RuntimeError("Build CPU limit is missing.")
    finally:
        k.CloseHandle(job)


def build_command(command):
    if not command:
        raise RuntimeError("Pass a build command after --.")
    executable = shutil.which(command[0])
    if not executable:
        raise RuntimeError("Build executable not found: " + command[0])
    result = [executable, *command[1:]]
    if Path(executable).stem.lower() == "dotnet" and len(result) > 1 and result[1] in ("build", "publish", "msbuild", "restore", "test", "pack"):
        result += ["-m:1", "-nr:false", "-p:BuildInParallel=false", "-p:UseSharedCompilation=false",
                   "-p:ConcurrentBuild=false", "-p:UseRazorBuildServer=false"]
        if result[1] == "restore":
            result += ["--disable-parallel"]
    return result


def run(args, k, receipt):
    if os.environ.get("CODEX_BUILD_JOB_NAME"):
        raise RuntimeError("Already budgeted: run nested commands serially inside the existing job; do not start another runner.")
    requested = args.memory_mb * 1_000_000
    if not 32_000_000 <= requested <= DEFAULT_BYTES:
        raise RuntimeError("Build budget must be32–2000 MB. A larger necessary limit requires an explicit reviewed exception, not an automatic retry.")
    # Job commitments are page-granular. Round down, never above the owner's cap.
    memory = requested // mmap.PAGESIZE * mmap.PAGESIZE
    command = build_command(args.command[1:] if args.command[:1] == ["--"] else args.command)
    mutexes = []
    held_slot = None
    job = None
    info = ProcessInfo()
    duplicates = []
    try:
        for mutex_name in SLOT_MUTEX_NAMES:
            mutexes.append(checked(k.CreateMutexW(None, False, mutex_name)))
        handles = (w.HANDLE * len(mutexes))(*mutexes)
        deadline = time.monotonic() + args.wait_seconds
        while True:
            remaining = max(0, deadline - time.monotonic())
            wait_ms = min(30_000, max(0, int(remaining * 1000)))
            state = k.WaitForMultipleObjects(len(handles), handles, False, wait_ms)
            if 0 <= state < len(handles):
                held_slot = state
                break
            if 0x80 <= state < 0x80 + len(handles):
                held_slot = state - 0x80
                break
            if state != 258:
                raise RuntimeError("Could not acquire a shared build slot.")
            if time.monotonic() >= deadline:
                receipt.update(busy=True, sharedSlots=len(mutexes))
                print("Both shared build/resource slots are occupied; no child launched.", file=sys.stderr)
                return 75
            print("Waiting for either shared resource slot; no child launched.", flush=True)
        name = "Local\\Codex.Build." + uuid.uuid4().hex
        job = checked(k.CreateJobObjectW(None, name))
        limits = ExtendedLimits()
        limits.Basic.LimitFlags = JOB_MEMORY | KILL_ON_CLOSE | PRIORITY_LIMIT
        limits.Basic.PriorityClass = BELOW_NORMAL
        limits.JobMemoryLimit = memory
        checked(k.SetInformationJobObject(job, 9, c.byref(limits), c.sizeof(limits)))
        rate = max(1, min(2500, 10_000 // max(1, os.cpu_count() or 1)))
        cpu = CpuLimits(5, rate)
        checked(k.SetInformationJobObject(job, 15, c.byref(cpu), c.sizeof(cpu)))
        actual = query(k, job, 9, ExtendedLimits)
        actual_cpu = query(k, job, 15, CpuLimits)
        if actual.JobMemoryLimit != memory or actual_cpu.CpuRate != rate or actual_cpu.ControlFlags & 5 != 5:
            raise RuntimeError(f"Kernel resource limits did not match: memory={actual.JobMemoryLimit}/{memory}, CPU={actual_cpu.CpuRate}/{rate}.")
        environment = dict(os.environ)
        environment.update({
            "CODEX_BUILD_JOB_NAME": name, "CODEX_BUILD_MEMORY_BYTES": str(memory),
            "CODEX_BUILD_SLOT": str(held_slot + 1),
            "CODEX_BUILD_PYTHON": sys.executable, "DOTNET_PROCESSOR_COUNT": "1",
            # Leave native/compiler-host headroom while allowing a single
            # managed tool to use the existing budget. The kernel still caps
            # the aggregate tree at the owner-approved2 GB maximum.
            "DOTNET_GCHeapHardLimit": format(memory * args.managed_heap_percent // 100, "X"), "DOTNET_gcServer": "0",
            "MSBUILDDISABLENODEREUSE": "1", "DOTNET_CLI_USE_MSBUILD_SERVER": "false",
            "DOTNET_CLI_DO_NOT_USE_MSBUILD_SERVER": "1", "MSBUILDUSESERVER": "0",
            "UseSharedCompilation": "false", "ConcurrentBuild": "false", "BuildInParallel": "false",
            "CMAKE_BUILD_PARALLEL_LEVEL": "1", "CARGO_BUILD_JOBS": "1", "MAX_JOBS": "1",
            "MAKEFLAGS": "-j1", "OMP_NUM_THREADS": "1", "OPENBLAS_NUM_THREADS": "1",
        })
        block = c.create_unicode_buffer("\0".join(key + "=" + value for key, value in sorted(environment.items())) + "\0\0")
        startup = StartupInfo()
        startup.cb, startup.Flags = c.sizeof(startup), 0x100
        current = k.GetCurrentProcess()
        for number, field in ((-10, "StdInput"), (-11, "StdOutput"), (-12, "StdError")):
            source = k.GetStdHandle(number)
            if source and source != c.c_void_p(-1).value:
                duplicate = w.HANDLE()
                checked(k.DuplicateHandle(current, source, current, c.byref(duplicate), 0, True, 2))
                duplicates.append(duplicate)
                setattr(startup, field, duplicate.value)
        line = c.create_unicode_buffer(subprocess.list2cmdline(command))
        # Suspended creation closes the otherwise unbounded launch/assignment race.
        checked(k.CreateProcessW(command[0], line, None, None, True,
                                0x4 | 0x400 | 0x08000000 | BELOW_NORMAL,
                                block, str(Path.cwd()), c.byref(startup), c.byref(info)))
        checked(k.AssignProcessToJobObject(job, info.Process))
        inside = w.BOOL()
        checked(k.IsProcessInJob(info.Process, job, c.byref(inside)))
        if not inside.value:
            raise RuntimeError("Child containment failed; build remains suspended.")
        receipt.update(requestedMemoryLimitBytes=requested,memoryLimitBytes=memory, cpuPercent=rate / 100, childPid=info.ProcessId,
                       managedHeapLimitBytes=memory * args.managed_heap_percent // 100,
                       managedHeapPercent=args.managed_heap_percent,
                       limitsVerified=True, tool=Path(command[0]).name, singleWorker=True,
                       sharedSlots=len(mutexes), slot=held_slot + 1,
                       slotMutex=SLOT_MUTEX_NAMES[held_slot])
        print(f"Resource slot {held_slot + 1}/{len(mutexes)}: {args.memory_mb} MB total child-tree ceiling, one worker, {rate / 100:g}% CPU cap, below-normal priority.", flush=True)
        if k.ResumeThread(info.Thread) == 0xFFFFFFFF:
            raise c.WinError(c.get_last_error())
        k.CloseHandle(info.Thread)
        info.Thread = None
        while k.WaitForSingleObject(info.Process, 250) == 258:
            pass
        code = w.DWORD()
        checked(k.GetExitCodeProcess(info.Process, c.byref(code)))
        receipt["exitCode"] = code.value
        return int(code.value)
    finally:
        if job:
            if info.Process:
                # Also covers assignment failure: the suspended child may not
                # have joined the job yet and must never be left orphaned.
                k.TerminateProcess(info.Process, 125)
                k.TerminateJobObject(job, 125)
                k.WaitForSingleObject(info.Process, 5000)
                deadline = time.monotonic() + 5
                while query(k, job, 1, Accounting).ActiveProcesses and time.monotonic() < deadline:
                    time.sleep(.01)
                receipt["childrenDrained"] = query(k, job, 1, Accounting).ActiveProcesses == 0
            try:
                final = query(k, job, 9, ExtendedLimits)
                receipt["peakJobMemoryBytes"] = final.PeakJobMemoryUsed
                receipt["peakProcessMemoryBytes"] = final.PeakProcessMemoryUsed
            finally:
                k.CloseHandle(job)
        elif info.Process:
            k.TerminateProcess(info.Process, 125)
        for handle in (*duplicates, info.Thread, info.Process):
            if handle:
                k.CloseHandle(handle)
        if held_slot is not None:
            k.ReleaseMutex(mutexes[held_slot])
        for mutex in mutexes:
            k.CloseHandle(mutex)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify", action="store_true", help="Verify this process is already inside the enforced job.")
    parser.add_argument("--memory-mb", type=int, default=2000, help="Decimal MB,32–2000; default2 GB for the entire child tree.")
    parser.add_argument("--wait-seconds", type=int, choices=range(0, 3601), default=0,
                        metavar="0..3600", help="Finite wait for either of two shared slots; default0 rejects busy slots immediately.")
    parser.add_argument("--managed-heap-percent", type=int, choices=range(25, 91), default=75,
                        metavar="25..90", help="Managed share within the same hard job budget; default75, never changes the aggregate limit.")
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    receipt = {"schema": 2, "limitsVerified": False}
    started = time.monotonic()
    try:
        k = api()
        if args.verify:
            verify_current(k)
            return 0
        return run(args, k, receipt)
    except (OSError, RuntimeError, ValueError) as error:
        receipt["error"] = str(error)
        print("Build not launched/completed safely: " + str(error), file=sys.stderr)
        return 125
    finally:
        if args.receipt:
            receipt["seconds"] = round(time.monotonic() - started, 3)
            args.receipt.parent.mkdir(parents=True, exist_ok=True)
            args.receipt.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    raise SystemExit(main())
