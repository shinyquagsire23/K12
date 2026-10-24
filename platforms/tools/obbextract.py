#!/usr/bin/env python3
"""Extract an Aspyr Android .obb (libObbVfs, "ASL::ObbVfs" v1) into a directory.

Format, read off libObbVfs.so (ReadVfs::ReadVfs, v1::Serializer<...>::deserialize):

  file tail, 16 bytes:  u64 index_begin, u64 index_uncompressed_size
  [index_begin, size-16): one zlib stream holding the file table:
      u64 count; count x { u64 len; char path[len]; u64 offset; u64 usize; u64 csize }
  a file with usize == csize is stored raw at [offset, offset+usize).
  a file with usize != csize is a ChunkCompressed stream over [offset, offset+csize):
      its own 16-byte tail (u64 meta_begin, u64 meta_usize) points at a zlib'd
      StreamMetadata: u64 x5 (compressedBegin, compressedSize, uncompressedSize,
      uncompressedChunkSize, uncompressedFinalChunkSize), u64 n, n x {u64 begin, u64 csize}.
      Aspyr's KOTOR II OBBs (main.213, patch.14) contain no such file, so this tool
      refuses them rather than guess at the chunk addressing.

Usage: obbextract.py OBB... -o OUTDIR [--list]
"""
import argparse, os, struct, sys, zlib

def read_index(f):
    f.seek(0, 2); size = f.tell()
    f.seek(size - 16)
    begin, usize = struct.unpack('<QQ', f.read(16))
    f.seek(begin)
    d = zlib.decompress(f.read(size - 16 - begin))
    if len(d) != usize:
        raise ValueError('index size %d != %d' % (len(d), usize))
    (n,), p, out = struct.unpack_from('<Q', d), 8, []
    for _ in range(n):
        (l,) = struct.unpack_from('<Q', d, p); p += 8
        path = d[p:p + l].decode(); p += l
        off, us, cs = struct.unpack_from('<QQQ', d, p); p += 24
        out.append((path, off, us, cs))
    if p != len(d):
        raise ValueError('index has %d trailing bytes' % (len(d) - p))
    return out

def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('obb', nargs='+')
    ap.add_argument('-o', '--out')
    ap.add_argument('--list', action='store_true')
    a = ap.parse_args()
    if not a.list and not a.out:
        ap.error('need -o OUTDIR or --list')
    for obb in a.obb:
        with open(obb, 'rb') as f:
            ents = read_index(f)
            if a.list:
                for path, off, us, cs in ents:
                    print('%12d %12d %s' % (off, us, path))
                continue
            done = 0
            for path, off, us, cs in ents:
                if us != cs:
                    sys.exit('%s: %s is chunk-compressed; not supported' % (obb, path))
                dst = os.path.join(a.out, *path.split('/'))
                if os.path.exists(dst) and os.path.getsize(dst) == us:
                    continue
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                f.seek(off)
                with open(dst + '.part', 'wb') as o:
                    left = us
                    while left:
                        buf = f.read(min(left, 1 << 22))
                        if not buf:
                            sys.exit('%s: %s truncated' % (obb, path))
                        o.write(buf); left -= len(buf)
                os.replace(dst + '.part', dst)
                done += 1
            print('%s: %d files (%d written)' % (obb, len(ents), done))

if __name__ == '__main__':
    main()
