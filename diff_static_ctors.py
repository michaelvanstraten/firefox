#!/usr/bin/env python

import sys
import subprocess


def get_static_ctors(filename):
    ctors = []
    proc = subprocess.Popen(
        ["objdump", "--syms", filename],
        stdout=subprocess.PIPE,
    )
    for line in proc.stdout:
        if b"_GLOBAL__sub_" in line:
            symbol_name = line.split()[5].decode()
            ctors.append(symbol_name)
    return ctors


def main(old_file, new_file):
    old_symbols = get_static_ctors(old_file)
    new_symbols = get_static_ctors(new_file)

    for new_symbol in filter(lambda symbol: symbol not in old_symbols, new_symbols):
        print(
            subprocess.run(
                ["objdump", f"--disassemble-symbols={new_symbol}", new_file], capture_output=True
            ).stdout.decode()
        )


if __name__ == "__main__":
    main(*sys.argv[1:])
