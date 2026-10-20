"""Builds the playtest zip both PCs play from: the committed project tree at HEAD (git archive) plus this machine's local imports, which git ignores (the vault imports, the
Mannequin and hero clips, the roars), so P2 sees exactly what this machine shows.
    python Tools/export_playtest_zip.py [version]      (default: the Version in Source/AshenSpiral/Public/ASData.h)
Writes C:/Games/AshenSpiral_UE5_<version>_Playtest.zip next to the repository, replacing one of the same name. The generated content (the SourceArt meshes, M_Surface, M_Glow,
the map) is not shipped: the zip's own Setup builds it. Check a new zip from a fresh folder (extract, Tools/Setup.ps1 -Mode ContentOnly, Tools/RunDevScript.ps1) before sending it.
"""
import re
import subprocess
import sys
import zipfile
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
REPO = Path(subprocess.check_output(["git", "-C", str(PROJECT), "rev-parse", "--show-toplevel"], text=True).strip())
PROJECT_REL = PROJECT.relative_to(REPO).as_posix()
# local content folders (all of each) and single local files, relative to Content/
LOCAL_FOLDERS = ("Art/Imported", "Audio/Imported", "Characters")
LOCAL_GLOBS = ("Art/Meshes/SM_DC_*.uasset", "Art/Meshes/SM_MoltenHammer.uasset", "Audio/E_*.uasset")


def version():
    if len(sys.argv) > 1:
        return sys.argv[1]
    text = (PROJECT / "Source/AshenSpiral/Public/ASData.h").read_text(encoding="utf-8")
    return re.search(r'Version=TEXT\("([^"]+)"\)', text).group(1)


def main():
    name = "AshenSpiral_UE5_%s_Playtest" % version()
    out = REPO.parent / (name + ".zip")
    prefix = name + "/"
    head = subprocess.check_output(["git", "-C", str(REPO), "rev-parse", "--short", "HEAD"], text=True).strip()
    tmp = out.with_suffix(".tmp.zip")
    if tmp.exists():
        tmp.unlink()
    subprocess.check_call(["git", "-C", str(REPO), "archive", "--format=zip", "--prefix=" + prefix, "-o", str(tmp), "HEAD:" + PROJECT_REL])
    content = PROJECT / "Content"
    extra = [p for folder in LOCAL_FOLDERS for p in sorted((content / folder).rglob("*")) if p.is_file()]
    extra += [p for pattern in LOCAL_GLOBS for p in sorted(content.glob(pattern))]
    added = size = 0
    with zipfile.ZipFile(tmp, "a", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        existing = set(z.namelist())
        for path in extra:
            arc = prefix + "Content/" + path.relative_to(content).as_posix()
            if arc in existing:
                continue
            z.write(path, arc)
            existing.add(arc)
            added += 1
            size += path.stat().st_size
    if out.exists():
        out.unlink()
    tmp.rename(out)
    with zipfile.ZipFile(out) as z:
        count = len(z.namelist())
    print("zip %s from HEAD %s: %d entries, %d local files added (%.1f MB raw), %.1f MB on disk" % (out, head, count, added, size / 1e6, out.stat().st_size / 1e6))


if __name__ == "__main__":
    main()
