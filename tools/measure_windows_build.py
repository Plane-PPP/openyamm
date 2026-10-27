"""Measure one Windows CI workload; stop its process tree on sustained low memory.

RAM and CPU metrics describe the whole dedicated runner, not compiler RSS.
Windows commit is not disk swap; Linux PSI/cgroup metrics are unavailable here.
"""
import argparse
import ctypes
from ctypes import wintypes
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

GIB = 1024 ** 3


def choose_jobs(cpus, available):
    jobs = min(8, cpus, int((available - 2 * GIB) // (2 * GIB)))
    if jobs < 1:
        raise RuntimeError('Insufficient available memory for compilation with a 2 GiB reserve')
    return jobs


def low_memory(sample):
    return (sample['available_bytes'] < max(512 * 1024 ** 2, sample['total_bytes'] * 0.08)
            or sample['commit_available_bytes'] < 256 * 1024 ** 2)


class MemoryStatus(ctypes.Structure):
    _fields_ = [('length', wintypes.DWORD), ('load', wintypes.DWORD)] + [
        (name, ctypes.c_ulonglong) for name in (
            'total_phys', 'avail_phys', 'total_page', 'avail_page',
            'total_virtual', 'avail_virtual', 'avail_extended')]


def sample_windows():
    memory = MemoryStatus()
    memory.length = ctypes.sizeof(memory)
    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
    if not kernel32.GlobalMemoryStatusEx(ctypes.byref(memory)):
        raise ctypes.WinError(ctypes.get_last_error())
    idle, kernel, user = (wintypes.FILETIME() for _ in range(3))
    if not kernel32.GetSystemTimes(ctypes.byref(idle), ctypes.byref(kernel), ctypes.byref(user)):
        raise ctypes.WinError(ctypes.get_last_error())
    def ticks(value):
        return (value.dwHighDateTime << 32) | value.dwLowDateTime
    return dict(time=time.time(), total_bytes=memory.total_phys, available_bytes=memory.avail_phys,
                used_bytes=memory.total_phys - memory.avail_phys,
                commit_used_bytes=memory.total_page - memory.avail_page,
                commit_available_bytes=memory.avail_page,
                cpu_idle_ticks=ticks(idle), cpu_total_ticks=ticks(kernel) + ticks(user))


def cpu_percent(first, last):
    total = last['cpu_total_ticks'] - first['cpu_total_ticks']
    idle = last['cpu_idle_ticks'] - first['cpu_idle_ticks']
    return max(0.0, min(100.0, 100 * (total - idle) / total)) if total > 0 else None


def stop_tree(process):
    if process.poll() is None:
        subprocess.run(['taskkill.exe', '/PID', str(process.pid), '/T', '/F'], check=True)
        process.wait(timeout=30)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', action='store_true')
    parser.add_argument('--phase', default='build')
    parser.add_argument('--output', default='build-resources')
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if sys.platform != 'win32':
        parser.error('This monitor is for Windows CI; use safe-heavy-run on Ubuntu')
    first = sample_windows()
    cpus = os.cpu_count() or 1
    if args.probe:
        jobs = choose_jobs(cpus, first['available_bytes'])
        probe = dict(cpus=cpus, jobs=jobs, **first)
        output = Path(args.output)
        output.mkdir(parents=True, exist_ok=True)
        (output / 'runner.json').write_text(json.dumps(probe, indent=2), encoding='utf-8')
        print(json.dumps(probe, indent=2))
        if os.environ.get('GITHUB_ENV'):
            with open(os.environ['GITHUB_ENV'], 'a', encoding='utf-8') as env:
                env.write(f'MINOTAUR_BUILD_JOBS={jobs}\n')
        return 0
    command = args.command[1:] if args.command[:1] == ['--'] else args.command
    if not command:
        parser.error('A command after -- is required')
    command[0] = shutil.which(command[0]) or command[0]
    if low_memory(first):
        raise RuntimeError('Runner already under memory pressure; refusing workload')
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    start = time.monotonic()
    samples = [first]
    process = None
    reason = 'completed'
    exit_code = 1
    low_since = None
    try:
        with (output / f'{args.phase}.jsonl').open('w', encoding='utf-8') as log:
            log.write(json.dumps(first) + '\n')
            log.flush()
            process = subprocess.Popen(command)
            while True:
                try:
                    process.wait(timeout=1)
                except subprocess.TimeoutExpired:
                    pass
                current = sample_windows()
                current['cpu_percent'] = cpu_percent(samples[-1], current)
                samples.append(current)
                log.write(json.dumps(current) + '\n')
                log.flush()
                if process.poll() is not None:
                    exit_code = process.returncode
                    break
                if low_memory(current):
                    if low_since is None:
                        low_since = time.monotonic()
                    elif time.monotonic() - low_since >= 10:
                        reason = 'sustained_low_memory'
                        stop_tree(process)
                        exit_code = 137
                        break
                else:
                    low_since = None
    except BaseException:
        reason = 'monitor_or_launch_failure'
        if process is not None:
            stop_tree(process)
        raise
    finally:
        result = dict(phase=args.phase, duration_seconds=time.monotonic() - start,
                      exit_code=exit_code, reason=reason, cpus=cpus,
                      jobs=os.environ.get('MINOTAUR_BUILD_JOBS'),
                      scope='whole Windows runner; sampled every second',
                      baseline_used_bytes=first['used_bytes'],
                      peak_used_bytes=max(s['used_bytes'] for s in samples),
                      minimum_available_bytes=min(s['available_bytes'] for s in samples),
                      peak_commit_used_bytes=max(s['commit_used_bytes'] for s in samples),
                      mean_cpu_percent=cpu_percent(first, samples[-1]),
                      peak_cpu_percent=max((s.get('cpu_percent') or 0 for s in samples)),
                      psi=None, cgroup_events=None, disk_swap_peak=None)
        (output / f'{args.phase}.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
        print(json.dumps(result, indent=2), flush=True)
    return exit_code


if __name__ == '__main__':
    sys.exit(main())
