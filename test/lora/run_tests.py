"""Compile actual gateway code with host RTOS/UART fakes, then execute it.

Usage: python test/lora/run_tests.py [--cc <gcc/clang/zig executable>]
Zig is used as `zig cc`; GCC/Clang can be passed directly. No hardware is touched.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
args = parser.parse_args()
build = root / 'build' / 'lora_host_tests'
build.mkdir(parents=True, exist_ok=True)
for name in ('lora.c', 'lora.h'):
    shutil.copyfile(root / 'main' / name, build / name)
compiler = [args.cc]
if Path(args.cc).stem == 'zig':
    compiler.append('cc')
executable = build / ('test_lora.exe' if os.name == 'nt' else 'test_lora')
command = compiler + [
    '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
    '-Wno-pointer-sign', '-g', '-O0',
    '-I', str(build), '-I', str(root / 'test/lora/stubs'),
    str(root / 'test/lora/test_lora.c'), '-o', str(executable),
]
subprocess.run(command, check=True, cwd=root)
subprocess.run([str(executable)], check=True, cwd=root)
