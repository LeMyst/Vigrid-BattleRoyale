"""
 *  Enfusion `.edds` container: inspect, verify, and losslessly shrink.
 *
 *  The twelve loading screens under GUI/textures/loading_screens were 120 MB, and TODO.md blamed
 *  duplication - "seven are byte-for-byte identical". They are not: all twelve are distinct git
 *  blobs. They share a byte count because they are all 1920x1080 with EVERY chunk stored
 *  UNCOMPRESSED, which is what this tool fixes. Bohemia's own loading screens use the identical
 *  pixel format and simply LZ4 the larger mips (P:\\dze\\gui\\textures\\loading_screens\\
 *  loading_screen_default.edds is 811 KB with 8 of its 11 chunks compressed), so this changes no
 *  pixel and follows vanilla practice rather than inventing anything.
 *
 *  THE FORMAT, established by decoding until every byte of all twelve files was accounted for:
 *
 *      0            128-byte DDS header. reserved dword[1] == "ENF1" is the Enfusion marker.
 *      128          mipmapCount entries of 8 bytes:
 *                     4-byte method, "COPY" or "LZ4 "
 *                     4-byte STORED size (raw size for COPY, payload size for "LZ4 ")
 *      128 + 8*N    the payloads, in table order, SMALLEST MIP FIRST.
 *
 *  A "COPY" payload is the mip verbatim. An "LZ4 " payload is itself framed:
 *
 *      u32          the mip's uncompressed size
 *      then, repeating:
 *      u32          block header: bit31 = LAST BLOCK, bits 0-30 = compressed size
 *      ...          that many bytes of LZ4 block data
 *
 *  Each block decodes to 65536 bytes except the last, which is the remainder. ⚠️ THE BLOCKS ARE
 *  LINKED: a block routinely matches backwards into the block before it (measured - every
 *  multi-block chunk in this repo and in vanilla's own file does it within the first few bytes),
 *  so they must be decoded into ONE shared buffer per chunk. Decoding them independently does not
 *  error cleanly, it silently produces short output.
 *
 *  Going the other way, independent blocks are a valid subset of linked ones - a decoder cannot
 *  tell - so the encoder here compresses each 65536-byte slice on its own and simply never emits
 *  a cross-block reference. That costs a little ratio at each boundary and nothing else.
 *
 *  A chunk's uncompressed size is therefore stored twice over: in the payload prelude, and
 *  derivable from the header's width, height and bits-per-pixel. Both are checked against each
 *  other on load, because a disagreement means the format is not what this file thinks it is.
 *
 *  Python 3 stdlib only, and enumeration is `git ls-files` - the two rules in Tools/README.md.
 *  The LZ4 block codec below is hand-written because neither the stdlib nor the `lz4` package is
 *  available here.
 *
 *  Usage:
 *      python Tools/edds.py info [path ...]      dump header and chunk table
 *      python Tools/edds.py selftest             prove the parser and the codec (run this first)
 *      python Tools/edds.py pack [path ...]      recompress in place, losslessly
 *      python Tools/edds.py unpack <in> <out>    write the raw mip payload out (debugging)
"""

from __future__ import annotations

import argparse
import os
import struct
import subprocess
import sys

DDS_MAGIC = b"DDS "
ENF_MARKER = b"ENF1"
HEADER_SIZE = 128
ENTRY_SIZE = 8
METHOD_COPY = b"COPY"
METHOD_LZ4 = b"LZ4 "

FOURCC_NONE = b"\0\0\0\0"
FOURCC_DXT1 = b"DXT1"

DDSD_PITCH = 0x00000008
DDSD_LINEARSIZE = 0x00080000
DDPF_FOURCC = 0x00000004

DEFAULT_GLOB = "GUI/textures/loading_screens/*.edds"

#  A vanilla file whose "LZ4 " blocks this repo did not produce. Decoding it is the only evidence
#  that the decoder is right about somebody else's data rather than merely self-consistent.
VANILLA_REFERENCE = r"P:\dze\gui\textures\loading_screens\loading_screen_default.edds"


# ---------------------------------------------------------------------------
#  LZ4 block format
#
#  A block is a run of sequences. Each is:
#      token byte: high nibble = literal length, low nibble = (match length - 4)
#      a nibble of 15 means "read more length bytes, 255 continues, <255 ends"
#      the literals
#      a 2-byte little-endian match offset, then the match-length extra bytes
#  The final sequence is literals only and carries no offset. Two rules make a block
#  decodable by any conforming decoder, and both are easy to get wrong:
#      the last 5 bytes of a block are always literals, and
#      no match may begin within the last 12 bytes.
# ---------------------------------------------------------------------------

MIN_MATCH = 4
LAST_LITERALS = 5
MF_LIMIT = 12
SKIP_TRIGGER = 6
MAX_OFFSET = 65535

#  Uncompressed bytes per block. Every multi-block chunk observed uses exactly this, with the
#  final block carrying the remainder.
BLOCK_SIZE = 65536
LAST_BLOCK_FLAG = 0x80000000


def lz4_decompress_into(src: bytes, dst: bytearray, out_size: int) -> None:
    """Decode one LZ4 block, APPENDING to `dst`.

    Match offsets are resolved against everything already in `dst`, not just this block, which is
    what makes linked blocks work. `out_size` is how much this block must produce.
    """
    start = len(dst)
    dst += bytes(out_size)
    d = start
    s = 0
    n = len(src)

    while s < n:
        token = src[s]
        s += 1

        lit = token >> 4
        if lit == 15:
            while True:
                b = src[s]
                s += 1
                lit += b
                if b != 255:
                    break
        if lit:
            dst[d:d + lit] = src[s:s + lit]
            s += lit
            d += lit

        #  A block that ends here ended on its literal run - that is the normal terminator.
        if s >= n:
            break

        offset = src[s] | (src[s + 1] << 8)
        s += 2
        if offset == 0:
            raise ValueError("LZ4: zero match offset")

        length = token & 0x0F
        if length == 15:
            while True:
                b = src[s]
                s += 1
                length += b
                if b != 255:
                    break
        length += MIN_MATCH

        p = d - offset
        if p < 0:
            raise ValueError("LZ4: match offset points before the start of the stream")
        if offset >= length:
            dst[d:d + length] = dst[p:p + length]
            d += length
        else:
            #  Overlapping match (run-length): must be copied byte by byte.
            for _ in range(length):
                dst[d] = dst[p]
                d += 1
                p += 1

    if d != start + out_size:
        raise ValueError("LZ4: block produced %d bytes, expected %d" % (d - start, out_size))
    del dst[d:]


def lz4_decompress(src: bytes, out_size: int) -> bytes:
    """Decode one standalone LZ4 block."""
    dst = bytearray()
    lz4_decompress_into(src, dst, out_size)
    return bytes(dst)


def _emit_length(out: bytearray, value: int) -> None:
    while value >= 255:
        out.append(255)
        value -= 255
    out.append(value)


def lz4_compress(src: bytes) -> bytes:
    """Encode one LZ4 block. Output is always valid; it may be larger than the input."""
    n = len(src)
    out = bytearray()

    if n < MF_LIMIT + 1:
        #  Too short to hold a match at all - one literal-only sequence.
        _emit_token_literals(out, src, 0, n)
        return bytes(out)

    mf_limit = n - MF_LIMIT
    match_limit = n - LAST_LITERALS

    table: dict[bytes, int] = {}
    anchor = 0
    ip = 0
    search_misses = 0

    while ip < mf_limit:
        key = src[ip:ip + MIN_MATCH]
        candidate = table.get(key, -1)
        table[key] = ip

        if candidate < 0 or ip - candidate > MAX_OFFSET:
            #  No usable candidate. Step forward, accelerating while we keep missing, exactly as
            #  the reference encoder does - without it, incompressible data costs a hash probe per
            #  byte and this pure-Python loop never finishes.
            search_misses += 1
            ip += 1 + (search_misses >> SKIP_TRIGGER)
            continue

        search_misses = 0

        #  Extend the match forward, in slices so the comparison runs at C speed.
        end = ip + MIN_MATCH
        while end < match_limit:
            step = min(64, match_limit - end)
            if src[candidate + (end - ip):candidate + (end - ip) + step] == src[end:end + step]:
                end += step
                continue
            while end < match_limit and src[candidate + (end - ip)] == src[end]:
                end += 1
            break

        match_len = end - ip
        offset = ip - candidate

        _emit_sequence(out, src, anchor, ip, offset, match_len)

        #  Index the interior of the match so later positions can still find it.
        for k in range(ip + 1, min(end, mf_limit)):
            table[src[k:k + MIN_MATCH]] = k

        ip = end
        anchor = ip

    _emit_token_literals(out, src, anchor, n)
    return bytes(out)


def _emit_token_literals(out: bytearray, src: bytes, start: int, stop: int) -> None:
    """The final, literals-only sequence. No offset follows it."""
    lit = stop - start
    if lit >= 15:
        out.append(0xF0)
        _emit_length(out, lit - 15)
    else:
        out.append(lit << 4)
    out += src[start:stop]


def _emit_sequence(out: bytearray, src: bytes, anchor: int, ip: int, offset: int, match_len: int) -> None:
    lit = ip - anchor
    extra = match_len - MIN_MATCH

    token = (15 if lit >= 15 else lit) << 4
    token |= 15 if extra >= 15 else extra
    out.append(token)

    if lit >= 15:
        _emit_length(out, lit - 15)
    out += src[anchor:ip]

    out.append(offset & 0xFF)
    out.append((offset >> 8) & 0xFF)

    if extra >= 15:
        _emit_length(out, extra - 15)


# ---------------------------------------------------------------------------
#  The "LZ4 " chunk payload: u32 raw size, then framed blocks
# ---------------------------------------------------------------------------


def decode_lz4_chunk(payload: bytes, raw_size: int) -> bytes:
    declared = struct.unpack("<I", payload[0:4])[0]
    if declared != raw_size:
        raise ValueError("chunk declares %d raw bytes, header implies %d" % (declared, raw_size))

    out = bytearray()
    s = 4
    saw_last = False
    while s < len(payload):
        if saw_last:
            raise ValueError("data follows the block marked last")
        header = struct.unpack("<I", payload[s:s + 4])[0]
        s += 4
        size = header & ~LAST_BLOCK_FLAG
        saw_last = bool(header & LAST_BLOCK_FLAG)

        want = min(BLOCK_SIZE, declared - len(out))
        if want <= 0:
            raise ValueError("more blocks than the declared size accounts for")
        lz4_decompress_into(payload[s:s + size], out, want)
        s += size

    if not saw_last:
        raise ValueError("no block carried the last-block flag")
    if len(out) != declared:
        raise ValueError("chunk produced %d bytes, declared %d" % (len(out), declared))
    return bytes(out)


def encode_lz4_chunk(raw: bytes) -> bytes:
    """Frame `raw` as an "LZ4 " payload.

    Each 65536-byte slice is compressed on its own. The decoder treats blocks as linked, and
    independent blocks are a legal subset of that, so this is correct - just marginally less
    dense than compressing with the previous slice as history.
    """
    out = bytearray(struct.pack("<I", len(raw)))
    offset = 0
    n = len(raw)
    while True:
        end = min(offset + BLOCK_SIZE, n)
        block = lz4_compress(raw[offset:end])
        last = end >= n
        out += struct.pack("<I", len(block) | (LAST_BLOCK_FLAG if last else 0))
        out += block
        offset = end
        if last:
            break
    return bytes(out)


# ---------------------------------------------------------------------------
#  The container
# ---------------------------------------------------------------------------


class Edds:
    def __init__(self, header: bytes, chunks: list[tuple[bytes, bytes]], width: int,
                 height: int, mips: int, bpp: int, fourcc: bytes):
        self.header = header
        #  (method, stored payload) in file order: smallest mip first.
        self.chunks = chunks
        self.width = width
        self.height = height
        self.mips = mips
        self.bpp = bpp
        self.fourcc = fourcc

    @property
    def is_dxt1(self) -> bool:
        return self.fourcc == FOURCC_DXT1

    def level_of(self, index: int) -> int:
        """Mip level for a chunk-table position. The table runs smallest mip first."""
        return self.mips - 1 - index

    def level_dims(self, level: int) -> tuple[int, int]:
        return max(1, self.width >> level), max(1, self.height >> level)

    def raw_size(self, index: int) -> int:
        """Uncompressed byte size of the chunk at table position `index`."""
        w, h = self.level_dims(self.level_of(index))
        if self.is_dxt1:
            return ((w + 3) // 4) * ((h + 3) // 4) * 8
        return w * h * (self.bpp // 8)

    def payload(self, index: int) -> bytes:
        """The chunk's decoded pixel bytes."""
        method, stored = self.chunks[index]
        if method == METHOD_COPY:
            return stored
        if method == METHOD_LZ4:
            return decode_lz4_chunk(stored, self.raw_size(index))
        raise ValueError("unknown chunk method %r" % method)

    def pixels_rgb(self, index: int) -> bytes:
        """The chunk as w*h*3 RGB bytes, whatever the stored pixel format."""
        w, h = self.level_dims(self.level_of(index))
        payload = self.payload(index)
        if self.is_dxt1:
            return bc1_decode(payload, w, h)
        #  BGRA8 -> RGB, stdlib only.
        out = bytearray(w * h * 3)
        out[0::3] = payload[2::4]
        out[1::3] = payload[1::4]
        out[2::3] = payload[0::4]
        return bytes(out)

    def to_bytes(self) -> bytes:
        out = bytearray(self.header)
        for method, stored in self.chunks:
            out += method
            out += struct.pack("<I", len(stored))
        for _, stored in self.chunks:
            out += stored
        return bytes(out)


def parse(data: bytes) -> Edds:
    if data[:4] != DDS_MAGIC:
        raise ValueError("not a DDS file")
    height, width = struct.unpack("<II", data[12:20])
    mips = struct.unpack("<I", data[28:32])[0]
    if mips < 1:
        raise ValueError("no mipmaps declared")
    if data[36:40] != ENF_MARKER:
        raise ValueError("missing ENF1 marker - not an Enfusion .edds")

    fourcc = data[84:88]
    bpp = struct.unpack("<I", data[88:92])[0]
    if fourcc == FOURCC_NONE:
        if bpp != 32:
            raise ValueError("unsupported uncompressed depth: %d bpp" % bpp)
    elif fourcc != FOURCC_DXT1:
        #  DXT5/BC3 and DX10 exist in vanilla but nothing here needs them, and guessing a chunk
        #  size for a format this tool cannot verify is how a silent corruption starts.
        raise ValueError("unsupported pixel format fourcc=%r" % fourcc)

    header = data[:HEADER_SIZE]
    table = []
    for i in range(mips):
        off = HEADER_SIZE + i * ENTRY_SIZE
        method = data[off:off + 4]
        size = struct.unpack("<I", data[off + 4:off + 8])[0]
        table.append((method, size))

    chunks = []
    off = HEADER_SIZE + mips * ENTRY_SIZE
    for method, size in table:
        chunks.append((method, data[off:off + size]))
        off += size

    if off != len(data):
        raise ValueError("chunk table accounts for %d bytes, file is %d" % (off, len(data)))

    return Edds(header, chunks, width, height, mips, bpp, fourcc)


# ---------------------------------------------------------------------------
#  BC1 / DXT1
#
#  4 bits per pixel against BGRA8's 32, so the pixel data drops 8x. Each 4x4 block stores two
#  RGB565 endpoints and sixteen 2-bit indices into a 4-entry palette interpolated between them.
#
#  ⚠️ THIS IS LOSSY, IN TWO WAYS, AND BOTH WERE DELIBERATE CHOICES:
#
#    Colour   endpoints are quantised to 5:6:5 and every pixel snaps to one of four interpolated
#             values per block. Gradients are where this shows, which is why `bc1` reports PSNR
#             per file rather than asking anyone to take it on trust.
#    Alpha    BC1 in 4-colour mode is fully opaque, so the alpha channel is DISCARDED. Measured
#             before choosing it: every one of the twelve loading screens is at least 99.7%
#             alpha-255, and no pixel anywhere falls below 250 in more than 0.1% of the image.
#             There is no transparency design here to lose - a loading screen is a full-screen
#             backdrop. Anything with real alpha needs DXT5/BC3, which this tool does not write.
#
#  numpy is imported lazily and ONLY for this: a per-block Python loop over 1.5M blocks does not
#  finish in reasonable time. `info`, `selftest` and `pack` stay stdlib-only.
# ---------------------------------------------------------------------------


def _numpy():
    try:
        import numpy  # noqa: PLC0415
    except ImportError:
        raise SystemExit(
            "the bc1 command needs numpy (a per-block Python loop is far too slow).\n"
            "Everything else in this tool is stdlib-only; install numpy or skip this command.")
    return numpy


def _to_565(np, c):
    return ((c[..., 0] >> 3).astype(np.uint32) << 11) | \
           ((c[..., 1] >> 2).astype(np.uint32) << 5) | \
            (c[..., 2] >> 3).astype(np.uint32)


def _from_565(np, v):
    r = (v >> 11) & 0x1F
    g = (v >> 5) & 0x3F
    b = v & 0x1F
    #  Replicate the high bits into the low ones, which is what the hardware does.
    return np.stack([(r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)],
                    axis=-1).astype(np.int32)


def _blockify(np, rgb, w, h):
    bw, bh = (w + 3) // 4, (h + 3) // 4
    pad_h, pad_w = bh * 4 - h, bw * 4 - w
    if pad_h or pad_w:
        #  Edge replication, so a partial block at the right/bottom edge does not drag the
        #  endpoints towards black.
        rgb = np.pad(rgb, ((0, pad_h), (0, pad_w), (0, 0)), mode="edge")
    return rgb.reshape(bh, 4, bw, 4, 3).transpose(0, 2, 1, 3, 4).reshape(-1, 16, 3), bw, bh


def bc1_encode(bgra: bytes, w: int, h: int) -> bytes:
    np = _numpy()
    img = np.frombuffer(bgra, dtype=np.uint8).reshape(h, w, 4)
    rgb = img[:, :, [2, 1, 0]].astype(np.int32)  # BGRA -> RGB
    blocks, bw, bh = _blockify(np, rgb, w, h)

    out = np.empty((blocks.shape[0], 8), dtype=np.uint8)

    #  Batched so the (n, 16, 4, 3) distance tensor stays small on the 8 MB mips.
    for lo in range(0, blocks.shape[0], 16384):
        blk = blocks[lo:lo + 16384]
        hi_c = blk.max(axis=1)
        lo_c = blk.min(axis=1)

        #  Inset the bounding box slightly. The extremes of a block are usually outliers, and
        #  pulling the endpoints in trades a little error at the ends for less in the middle.
        inset = (hi_c - lo_c) >> 4
        hi_c = np.clip(hi_c - inset, 0, 255)
        lo_c = np.clip(lo_c + inset, 0, 255)

        e0 = _to_565(np, hi_c)
        e1 = _to_565(np, lo_c)
        #  c0 > c1 selects the 4-colour OPAQUE mode. Sorting guarantees c0 >= c1; the equal case
        #  is handled below, because c0 == c1 silently selects the 3-colour mode whose index 3 is
        #  transparent black.
        c0 = np.maximum(e0, e1)
        c1 = np.minimum(e0, e1)

        p0 = _from_565(np, c0)
        p1 = _from_565(np, c1)
        pal = np.stack([p0, p1, (2 * p0 + p1) // 3, (p0 + 2 * p1) // 3], axis=1)

        diff = blk[:, :, None, :] - pal[:, None, :, :]
        idx = (diff * diff).sum(axis=-1).argmin(axis=2).astype(np.uint32)

        #  Degenerate block (one flat colour): force index 0 so we can never emit the transparent
        #  index of the 3-colour mode.
        idx[c0 == c1] = 0

        packed = np.zeros(blk.shape[0], dtype=np.uint32)
        for k in range(16):
            packed |= idx[:, k] << np.uint32(2 * k)

        chunk = out[lo:lo + 16384]
        chunk[:, 0] = c0 & 0xFF
        chunk[:, 1] = (c0 >> 8) & 0xFF
        chunk[:, 2] = c1 & 0xFF
        chunk[:, 3] = (c1 >> 8) & 0xFF
        chunk[:, 4] = packed & 0xFF
        chunk[:, 5] = (packed >> 8) & 0xFF
        chunk[:, 6] = (packed >> 16) & 0xFF
        chunk[:, 7] = (packed >> 24) & 0xFF

    return out.tobytes()


def bc1_decode(data: bytes, w: int, h: int) -> bytes:
    """Decode to RGB bytes (w*h*3). Used to score the encoder, not to ship anything."""
    np = _numpy()
    bw, bh = (w + 3) // 4, (h + 3) // 4
    raw = np.frombuffer(data, dtype=np.uint8).reshape(-1, 8)

    c0 = raw[:, 0].astype(np.uint32) | (raw[:, 1].astype(np.uint32) << 8)
    c1 = raw[:, 2].astype(np.uint32) | (raw[:, 3].astype(np.uint32) << 8)
    packed = (raw[:, 4].astype(np.uint32) | (raw[:, 5].astype(np.uint32) << 8) |
              (raw[:, 6].astype(np.uint32) << 16) | (raw[:, 7].astype(np.uint32) << 24))

    p0 = _from_565(np, c0)
    p1 = _from_565(np, c1)
    four = (c0 > c1)[:, None]
    p2 = np.where(four, (2 * p0 + p1) // 3, (p0 + p1) // 2)
    p3 = np.where(four, (p0 + 2 * p1) // 3, 0)
    pal = np.stack([p0, p1, p2, p3], axis=1)

    idx = np.stack([(packed >> np.uint32(2 * k)) & np.uint32(3) for k in range(16)], axis=1)
    pix = np.take_along_axis(pal, idx[:, :, None], axis=1)

    grid = pix.reshape(bh, bw, 4, 4, 3).transpose(0, 2, 1, 3, 4).reshape(bh * 4, bw * 4, 3)
    return grid[:h, :w].astype(np.uint8).tobytes()


def bc1_psnr(original_bgra: bytes, decoded_rgb: bytes, w: int, h: int) -> float:
    np = _numpy()
    a = np.frombuffer(original_bgra, dtype=np.uint8).reshape(h, w, 4)[:, :, [2, 1, 0]].astype(np.int32)
    b = np.frombuffer(decoded_rgb, dtype=np.uint8).reshape(h, w, 3).astype(np.int32)
    mse = ((a - b) ** 2).mean()
    if mse <= 0:
        return float("inf")
    import math  # noqa: PLC0415
    return 10.0 * math.log10(255.0 * 255.0 / mse)


def dxt1_header(src: bytes, width: int, height: int) -> bytes:
    """Rewrite a BGRA8 header as DXT1, field for field as vanilla's own DXT1 files do it."""
    out = bytearray(src)
    flags = struct.unpack("<I", out[8:12])[0]
    flags = (flags & ~DDSD_PITCH) | DDSD_LINEARSIZE
    struct.pack_into("<I", out, 8, flags)
    #  pitchOrLinearSize becomes the top level's total block bytes.
    struct.pack_into("<I", out, 20, ((width + 3) // 4) * ((height + 3) // 4) * 8)
    struct.pack_into("<I", out, 80, DDPF_FOURCC)
    out[84:88] = FOURCC_DXT1
    struct.pack_into("<I", out, 88, 0)          # RGBBitCount
    struct.pack_into("<IIII", out, 92, 0, 0, 0, 0)  # the four channel masks
    return bytes(out)


# ---------------------------------------------------------------------------
#  Commands
# ---------------------------------------------------------------------------


def repo_root() -> str:
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.dirname(here)


def tracked(pattern: str) -> list[str]:
    """Enumerate with git, never a filesystem walk - a walk descends into .claude/worktrees."""
    root = repo_root()
    out = subprocess.run(["git", "ls-files", pattern], cwd=root, capture_output=True, text=True)
    if out.returncode != 0:
        raise SystemExit("git ls-files failed: %s" % out.stderr.strip())
    return [os.path.join(root, line) for line in out.stdout.split("\n") if line.strip()]


def resolve(paths: list[str]) -> list[str]:
    return paths if paths else tracked(DEFAULT_GLOB)


def cmd_info(paths: list[str]) -> int:
    for path in resolve(paths):
        data = open(path, "rb").read()
        edds = parse(data)
        print("%s  %d bytes  %dx%d  %d mips  %dbpp" %
              (os.path.basename(path), len(data), edds.width, edds.height, edds.mips, edds.bpp))
        for i, (method, stored) in enumerate(edds.chunks):
            level = edds.mips - 1 - i
            raw = edds.raw_size(i)
            ratio = "" if method == METHOD_COPY else "  %.0f%% of raw" % (100.0 * len(stored) / raw)
            print("    mip %-2d %-5s stored %9d  raw %9d%s" %
                  (level, method.decode("latin1").strip(), len(stored), raw, ratio))
    return 0


def cmd_pack(paths: list[str], dry_run: bool) -> int:
    targets = resolve(paths)
    if not targets:
        print("no .edds files matched")
        return 1

    before = after = 0
    for path in targets:
        data = open(path, "rb").read()
        edds = parse(data)

        rebuilt = []
        changed = 0
        for i, (method, stored) in enumerate(edds.chunks):
            if method != METHOD_COPY:
                rebuilt.append((method, stored))
                continue
            packed = encode_lz4_chunk(stored)
            #  Only take it when it actually wins. A chunk that does not compress stays exactly
            #  as it is, byte for byte.
            if len(packed) < len(stored):
                rebuilt.append((METHOD_LZ4, packed))
                changed += 1
            else:
                rebuilt.append((method, stored))

        edds.chunks = rebuilt
        out = edds.to_bytes()

        #  Never write a file we cannot read back to the identical pixels.
        check = parse(out)
        for i in range(check.mips):
            if check.payload(i) != parse(data).payload(i):
                raise SystemExit("%s: round-trip mismatch on chunk %d - REFUSING to write" %
                                 (path, i))

        before += len(data)
        after += len(out)
        print("%-24s %10d -> %10d  (%3.0f%%, %d chunk(s) compressed)" %
              (os.path.basename(path), len(data), len(out), 100.0 * len(out) / len(data), changed))

        if not dry_run and len(out) < len(data):
            with open(path, "wb") as fh:
                fh.write(out)

    print()
    print("total %d -> %d bytes (%.0f%%, saved %.1f MB)%s" %
          (before, after, 100.0 * after / before, (before - after) / 1048576.0,
           "  [dry run, nothing written]" if dry_run else ""))
    return 0


def cmd_bc1(paths: list[str], dry_run: bool) -> int:
    targets = resolve(paths)
    before = after = 0
    worst = float("inf")

    for path in targets:
        data = open(path, "rb").read()
        src = parse(data)
        if src.is_dxt1:
            print("%-24s already DXT1, skipping" % os.path.basename(path))
            continue

        chunks = []
        top_psnr = 0.0
        for i in range(src.mips):
            level = src.level_of(i)
            w, h = src.level_dims(level)
            blocks = bc1_encode(src.payload(i), w, h)
            chunks.append((METHOD_COPY, blocks))
            if level == 0:
                top_psnr = bc1_psnr(src.payload(i), bc1_decode(blocks, w, h), w, h)

        out_edds = Edds(dxt1_header(src.header, src.width, src.height), chunks,
                        src.width, src.height, src.mips, 0, FOURCC_DXT1)

        #  LZ4 on top, kept only where it wins. Block-compressed data is high entropy so this
        #  usually declines, exactly as vanilla's own DXT1 files do (all-COPY).
        packed = []
        for i, (method, stored) in enumerate(out_edds.chunks):
            cand = encode_lz4_chunk(stored)
            packed.append((METHOD_LZ4, cand) if len(cand) < len(stored) else (method, stored))
        out_edds.chunks = packed

        blob = out_edds.to_bytes()

        #  Re-read what we are about to write and check every chunk decodes to the right size.
        check = parse(blob)
        for i in range(check.mips):
            if len(check.payload(i)) != check.raw_size(i):
                raise SystemExit("%s: chunk %d does not decode - REFUSING to write" % (path, i))

        before += len(data)
        after += len(blob)
        worst = min(worst, top_psnr)
        print("%-24s %10d -> %8d  (%3.0f%%)  PSNR %.1f dB" %
              (os.path.basename(path), len(data), len(blob), 100.0 * len(blob) / len(data),
               top_psnr))

        if not dry_run:
            with open(path, "wb") as fh:
                fh.write(blob)

    if before:
        print()
        print("total %d -> %d bytes (%.0f%%, saved %.1f MB), worst PSNR %.1f dB%s" %
              (before, after, 100.0 * after / before, (before - after) / 1048576.0, worst,
               "  [dry run, nothing written]" if dry_run else ""))
    return 0


def cmd_unpack(src: str, dst: str) -> int:
    edds = parse(open(src, "rb").read())
    with open(dst, "wb") as fh:
        for i in range(edds.mips):
            fh.write(edds.payload(i))
    return 0


def _mip_mismatch(fine: bytes, fine_w: int, coarse: bytes, coarse_w: int, coarse_h: int) -> float:
    """Mean absolute channel difference between `coarse` and a 2x box filter of `fine`.

    Sampled rather than exhaustive - this runs over 8 MB mips and only has to distinguish
    "these are the same picture" from "one of them is garbage".
    """
    step = max(1, coarse_h // 64)
    total = 0
    count = 0
    for y in range(0, coarse_h, step):
        for x in range(0, coarse_w, max(1, coarse_w // 64)):
            ci = (y * coarse_w + x) * 3
            fi = ((y * 2) * fine_w + (x * 2)) * 3
            for c in range(3):
                a = (fine[fi + c] + fine[fi + 3 + c] +
                     fine[fi + fine_w * 3 + c] + fine[fi + fine_w * 3 + 3 + c]) // 4
                total += abs(a - coarse[ci + c])
                count += 1
    return total / count if count else 0.0


def cmd_selftest() -> int:
    failures = 0

    def check(name: str, ok: bool, detail: str = "") -> None:
        nonlocal failures
        print("  %-58s %s%s" % (name, "ok" if ok else "FAIL", ("  " + detail) if detail else ""))
        if not ok:
            failures += 1

    print("LZ4 codec")
    cases = {
        "empty": b"",
        "one byte": b"a",
        "short, under MF_LIMIT": b"abcdefgh",
        "highly repetitive": b"ABCD" * 5000,
        "run of one byte": b"\x00" * 100000,
        "literal-only, incompressible": bytes((i * 37 + (i >> 3)) & 0xFF for i in range(70000)),
        "overlapping match": b"xyz" + b"ab" * 30000,
    }
    for name, blob in cases.items():
        packed = lz4_compress(blob)
        try:
            back = lz4_decompress(packed, len(blob))
            check("round-trip: %s" % name, back == blob, "%d -> %d" % (len(blob), len(packed)))
        except Exception as exc:  # noqa: BLE001
            check("round-trip: %s" % name, False, str(exc))

    print("container: parse -> rebuild is byte-identical")
    targets = tracked(DEFAULT_GLOB)
    if not targets:
        check("tracked .edds found", False, "git ls-files matched nothing")
    for path in targets:
        data = open(path, "rb").read()
        try:
            edds = parse(data)
            check(os.path.basename(path), edds.to_bytes() == data)
        except Exception as exc:  # noqa: BLE001
            check(os.path.basename(path), False, str(exc))

    print("container: every chunk decodes to its declared mip size")
    for path in targets:
        edds = parse(open(path, "rb").read())
        try:
            ok = all(len(edds.payload(i)) == edds.raw_size(i) for i in range(edds.mips))
            check(os.path.basename(path), ok)
        except Exception as exc:  # noqa: BLE001
            check(os.path.basename(path), False, str(exc))

    print("decoded mips agree with each other")
    #  The strongest check available, and the only one that is not self-referential: a decoded
    #  mip, box-filtered down by two, must resemble the next mip down - which for these files is
    #  a chain that ENDS in COPY chunks nobody had to decode. If the LZ4 decode were subtly wrong
    #  the two would not line up, where a size check would still pass.
    for path in targets:
        e = parse(open(path, "rb").read())
        try:
            worst = 0.0
            for j in range(e.mips - 1):
                fine = e.pixels_rgb(j + 1)       # larger mip (the table is smallest-first)
                coarse = e.pixels_rgb(j)         # the one below it
                level = e.level_of(j)
                cw, ch = e.level_dims(level)
                fw = e.level_dims(level - 1)[0]
                if cw * 2 > fw or ch < 1:
                    continue
                worst = max(worst, _mip_mismatch(fine, fw, coarse, cw, ch))
            check(os.path.basename(path), worst <= 24.0, "worst mean channel delta %.1f" % worst)
        except Exception as exc:  # noqa: BLE001
            check(os.path.basename(path), False, str(exc))

    print("decoder against data this repo did not produce")
    if os.path.exists(VANILLA_REFERENCE):
        try:
            e = parse(open(VANILLA_REFERENCE, "rb").read())
            lz4_chunks = sum(1 for m, _ in e.chunks if m == METHOD_LZ4)
            sizes_ok = all(len(e.payload(i)) == e.raw_size(i) for i in range(e.mips))
            check("decode %s" % os.path.basename(VANILLA_REFERENCE), sizes_ok and lz4_chunks > 0,
                  "%d of %d chunks are LZ4" % (lz4_chunks, e.mips))

            #  Their bytes -> our pixels -> our bytes -> our pixels. Proves the encoder against an
            #  independent encoder's output, which recompressing our own output cannot.
            raw = e.payload(e.mips - 2)
            check("re-encode a vanilla mip", decode_lz4_chunk(encode_lz4_chunk(raw), len(raw)) == raw)
        except Exception as exc:  # noqa: BLE001
            check("decode %s" % os.path.basename(VANILLA_REFERENCE), False, str(exc))
    else:
        print("  SKIPPED - %s not present (P: not mounted?)." % VANILLA_REFERENCE)
        print("  This is the only check that proves the decoder against foreign data; do not")
        print("  treat a run without it as a full pass.")

    print()
    if failures:
        print("%d check(s) FAILED" % failures)
    else:
        print("all checks passed")
    return 1 if failures else 0


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description="Enfusion .edds inspect / verify / shrink")
    sub = ap.add_subparsers(dest="cmd", required=True)

    p_info = sub.add_parser("info", help="dump header and chunk table")
    p_info.add_argument("paths", nargs="*")

    sub.add_parser("selftest", help="prove the parser and the LZ4 codec")

    p_pack = sub.add_parser("pack", help="losslessly recompress in place")
    p_pack.add_argument("paths", nargs="*")
    p_pack.add_argument("-n", "--dry-run", action="store_true", help="report, write nothing")

    p_bc1 = sub.add_parser("bc1", help="convert BGRA8 to DXT1 in place (LOSSY - see the header)")
    p_bc1.add_argument("paths", nargs="*")
    p_bc1.add_argument("-n", "--dry-run", action="store_true", help="report, write nothing")

    p_unpack = sub.add_parser("unpack", help="write the decoded mip payload out")
    p_unpack.add_argument("src")
    p_unpack.add_argument("dst")

    args = ap.parse_args(argv)
    if args.cmd == "info":
        return cmd_info(args.paths)
    if args.cmd == "selftest":
        return cmd_selftest()
    if args.cmd == "pack":
        return cmd_pack(args.paths, args.dry_run)
    if args.cmd == "bc1":
        return cmd_bc1(args.paths, args.dry_run)
    if args.cmd == "unpack":
        return cmd_unpack(args.src, args.dst)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
