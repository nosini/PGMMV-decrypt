#!/usr/bin/env python3
"""Copy the shared libraries a program needs into a folder next to it.

usage: ci/bundle-libs.py PROGRAM LIBDIR LICENSEDIR

Every library PROGRAM loads, directly or through another bundled library, is
copied into LIBDIR unless ci/host-libraries.txt leaves it to the system.
PROGRAM gets the run path $ORIGIN/../lib and each copy $ORIGIN, so the copies
are found wherever the folder is unpacked. PROGRAM's references to libcurl
lose their symbol versions, which differ between distributions. The Debian copyright file of each
bundled library's package goes into LICENSEDIR.
"""
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

program, libdir, licensedir = (Path(arg) for arg in sys.argv[1:4])
patterns = [re.compile(line) for line in
            (Path(__file__).parent / 'host-libraries.txt').read_text().splitlines()
            if line.strip() and not line.startswith('#')]


def run(*args):
    return subprocess.run(args, capture_output=True, text=True, check=True).stdout


# Where the dynamic linker finds each library the program loads.
paths = {}
for line in run('ldd', str(program)).splitlines():
    parts = line.split()
    if len(parts) >= 3 and parts[1] == '=>' and parts[2].startswith('/'):
        paths[parts[0]] = parts[2]

libdir.mkdir(parents=True, exist_ok=True)
licensedir.mkdir(parents=True, exist_ok=True)
bundled = set()
queue = [program]
while queue:
    for soname in run('patchelf', '--print-needed', str(queue.pop())).split():
        if soname in bundled or any(p.search(soname) for p in patterns):
            continue
        if soname not in paths:
            sys.exit(f'{soname}: not found')
        copy = libdir / soname
        shutil.copyfile(paths[soname], copy)
        run('patchelf', '--set-rpath', '$ORIGIN', str(copy))
        bundled.add(soname)
        queue.append(copy)
        # ldd reports /lib paths, dpkg knows the merged /usr/lib ones.
        owner = subprocess.run(['dpkg', '-S', '/usr/lib/' + paths[soname].split('/lib/', 1)[1]],
                               capture_output=True, text=True)
        if owner.returncode == 0:
            package = owner.stdout.split(':')[0]
            notice = Path('/usr/share/doc', package, 'copyright')
            if notice.exists():
                shutil.copyfile(notice, licensedir / f'{package}-copyright')

run('patchelf', '--set-rpath', '$ORIGIN/../lib', str(program))



def drop_version_need(path, library):
    """Unlink LIBRARY's entry from the version needs of the ELF64 file PATH."""
    data = bytearray(path.read_bytes())
    phoff, = struct.unpack_from('<Q', data, 0x20)
    shoff, = struct.unpack_from('<Q', data, 0x28)
    phentsize, phnum, shentsize, shnum = struct.unpack_from('<HHHH', data, 0x36)
    loads, dynamic = [], None
    for i in range(phnum):
        kind, _, offset, vaddr, _, filesz, _, _ = struct.unpack_from('<IIQQQQQQ', data, phoff + i * phentsize)
        if kind == 1:  # PT_LOAD
            loads.append((vaddr, offset, filesz))
        elif kind == 2:  # PT_DYNAMIC
            dynamic = offset, filesz

    def file_offset(address):
        return next(o + address - v for v, o, size in loads if v <= address < v + size)

    tags = {}
    for i in range(dynamic[1] // 16):
        tag, value = struct.unpack_from('<qQ', data, dynamic[0] + 16 * i)
        if tag == 0:
            break
        tags[tag] = value, dynamic[0] + 16 * i + 8
    strtab = file_offset(tags[5][0])
    (verneed, verneed_slot), (count, count_slot) = tags[0x6ffffffe], tags[0x6fffffff]

    position, previous = file_offset(verneed), None
    for _ in range(count):
        _, _, name, _, following = struct.unpack_from('<HHIII', data, position)
        if data[strtab + name:data.index(0, strtab + name)].decode() == library:
            if previous is None:
                struct.pack_into('<Q', data, verneed_slot, verneed + following)
            else:
                link, = struct.unpack_from('<I', data, previous + 12)
                struct.pack_into('<I', data, previous + 12, link + following if following else 0)
            struct.pack_into('<Q', data, count_slot, count - 1)
            for i in range(shnum):  # keep the section header's entry count in step
                header = shoff + i * shentsize
                if struct.unpack_from('<I', data, header + 4)[0] == 0x6ffffffe:
                    struct.pack_into('<I', data, header + 44, count - 1)
            path.write_bytes(data)
            return
        previous, position = position, position + following


# Debian's libcurl tags its symbols with the version CURL_OPENSSL_4, while
# other distributions' libcurl has no versions, and the dynamic linker then
# warns on every start. Untagged references work with both.
for line in run('objdump', '-T', str(program)).splitlines():
    fields = line.split()
    if len(fields) >= 2 and fields[-2] == '(CURL_OPENSSL_4)':
        run('patchelf', '--clear-symbol-version', fields[-1], str(program))
drop_version_need(program, 'libcurl.so.4')
print('\n'.join(sorted(bundled)))
