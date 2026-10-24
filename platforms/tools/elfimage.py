"""elfimage -- just enough ELF64 (AArch64) reading to lift initialized data out of libkotor2.so.

    img = ElfImage(path)
    img.read(vaddr, n)          bytes as loaded (zeros for .bss / past p_filesz)
    img.symbols                 {name: (vaddr, size, type)} from .dynsym (defined only)
    img.relocs_in(lo, hi)       [(vaddr, target_vaddr or None, target_symbol or None)] for the
                                pointer-sized relocations inside [lo, hi): RELATIVE resolves to an
                                address, ABS64/GLOB_DAT to a symbol (+ addend)
    img.symbol_at(vaddr)        (name, offset) of the defined symbol containing vaddr, or None
    img.section_of(vaddr)       section name
    img.cstring(vaddr)          NUL-terminated bytes at vaddr, or None if it does not look like text
"""
import bisect
import struct

R_AARCH64_ABS64 = 257
R_AARCH64_GLOB_DAT = 1025
R_AARCH64_RELATIVE = 1027


class ElfImage:
    def __init__(self, path):
        self.data = open(path, 'rb').read()
        d = self.data
        assert d[:4] == b'\x7fELF' and d[4] == 2, path
        e_phoff, e_shoff = struct.unpack_from('<QQ', d, 0x20)
        e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHHHH', d, 0x36)

        self.loads = []
        for i in range(e_phnum):
            p_type, _, p_offset, p_vaddr, _, p_filesz, p_memsz, _ = struct.unpack_from(
                '<IIQQQQQQ', d, e_phoff + i * e_phentsize)
            if p_type == 1:
                self.loads.append((p_vaddr, p_memsz, p_offset, p_filesz))

        shdrs = []
        for i in range(e_shnum):
            shdrs.append(struct.unpack_from('<IIQQQQIIQQ', d, e_shoff + i * e_shentsize))
        strtab_off = shdrs[e_shstrndx][4]

        def sname(off, base):
            return d[base + off:d.index(b'\0', base + off)].decode('latin-1')

        self.sections = []   # (addr, size, name)
        by_name = {}
        for sh in shdrs:
            name = sname(sh[0], strtab_off)
            by_name[name] = sh
            if sh[3]:
                self.sections.append((sh[3], sh[5], name))
        self.sections.sort()

        dynsym, dynstr = by_name['.dynsym'], by_name['.dynstr']
        self.symbols, self._symlist, symnames = {}, [], []
        for i in range(dynsym[5] // 24):
            st_name, st_info, _, st_shndx, st_value, st_size = struct.unpack_from(
                '<IBBHQQ', d, dynsym[4] + i * 24)
            name = sname(st_name, dynstr[4]) if st_name else ''
            symnames.append(name)
            typ = {1: 'OBJECT', 2: 'FUNC'}.get(st_info & 0xf, 'OTHER')
            if name and st_shndx != 0:
                self.symbols[name] = (st_value, st_size, typ)
                self._symlist.append((st_value, st_size, name))
        self._symlist.sort()
        self._symaddrs = [s[0] for s in self._symlist]

        self.relocs = {}   # vaddr -> (target_vaddr | None, target_symbol | None, addend)
        for name in ('.rela.dyn',):
            sh = by_name.get(name)
            if not sh:
                continue
            for i in range(sh[5] // 24):
                r_offset, r_info, r_addend = struct.unpack_from('<QQq', d, sh[4] + i * 24)
                rtype, rsym = r_info & 0xffffffff, r_info >> 32
                if rtype == R_AARCH64_RELATIVE:
                    self.relocs[r_offset] = (r_addend, None, 0)
                elif rtype in (R_AARCH64_ABS64, R_AARCH64_GLOB_DAT):
                    self.relocs[r_offset] = (None, symnames[rsym] or None, r_addend)
        self._reloc_addrs = sorted(self.relocs)

    def read(self, vaddr, n):
        out = bytearray(n)
        for p_vaddr, p_memsz, p_offset, p_filesz in self.loads:
            lo, hi = max(vaddr, p_vaddr), min(vaddr + n, p_vaddr + p_filesz)
            if lo < hi:
                out[lo - vaddr:hi - vaddr] = self.data[p_offset + lo - p_vaddr:p_offset + hi - p_vaddr]
        return bytes(out)

    def relocs_in(self, lo, hi):
        i = bisect.bisect_left(self._reloc_addrs, lo)
        out = []
        while i < len(self._reloc_addrs) and self._reloc_addrs[i] < hi:
            a = self._reloc_addrs[i]
            out.append((a,) + self.relocs[a])
            i += 1
        return out

    def symbol_at(self, vaddr):
        i = bisect.bisect_right(self._symaddrs, vaddr) - 1
        while i >= 0:
            addr, size, name = self._symlist[i]
            if addr <= vaddr < addr + max(size, 1):
                return name, vaddr - addr
            if addr < vaddr - 0x100000:
                break
            i -= 1
        return None

    def next_symbol_after(self, vaddr, extra=()):
        """The lowest symbol address (dynsym, or in the sorted `extra`) above vaddr."""
        i = bisect.bisect_right(self._symaddrs, vaddr)
        best = self._symaddrs[i] if i < len(self._symaddrs) else vaddr + 16
        j = bisect.bisect_right(extra, vaddr)
        if j < len(extra):
            best = min(best, extra[j])
        return max(best, vaddr + 1)

    def section_of(self, vaddr):
        for addr, size, name in self.sections:
            if addr <= vaddr < addr + size:
                return name
        return None

    def cstring(self, vaddr, limit=1 << 20):
        s = self.read(vaddr, min(limit, 4096))
        end = s.find(b'\0')
        while end < 0 and len(s) < limit:
            s = self.read(vaddr, min(limit, len(s) * 4))
            end = s.find(b'\0')
        if end < 0:
            return None
        s = s[:end]
        if any(c < 0x20 and c not in (9, 10, 13) for c in s):
            return None
        return s
