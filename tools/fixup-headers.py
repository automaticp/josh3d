#!/usr/bin/env python3
"""
Run as:
    fixup-headers.py fixup_file [target_dir]

target_dir will default to src/.
fixup_file file is a simple JSON with either a replacement string
or null for removal for each match:

    {
        "From.hpp": "To.hpp",
        "ToRemove.hpp": null
    }
"""
from pathlib import Path
from typing import Iterable
from dataclasses import dataclass
import difflib
import json
import sys
import re

# A common issue is fixing up headers when moving files around.
# Normal people use IDEs for this, but I like my "text editors".
#
# Q: Couldn't you use IWYU instead?
# A: Yes, but it does not do exactly what I want from this.
#    Plus setting it all up to respect forward declarations and such is a PITA.

PROJECT_DIR = Path(__file__).parent.parent.resolve()
SRC_DIR     = (PROJECT_DIR / "src").resolve()

# NOTE: Includes the newline character. Only considers spaces, no tabs or other
# space-like characters. This is consistent with the codebase formatting style.
INCLUDE_PATTERN = re.compile(r'^ *# *include *(<.+>|".+") *\n', re.MULTILINE)


def main():
    # This is kinda dumb, whatever.
    if len(sys.argv) <= 1:
        print("Provide the fixup file.")
        exit(1)

    fixup_file = Path(sys.argv[1])

    if len(sys.argv) >= 3:
        target_dir = Path(sys.argv[2])
        if not target_dir.is_dir():
            print("Invalid target directory.")
            exit(1)
    else:
        target_dir = SRC_DIR

    fixups  = parse_fixup_file(fixup_file)
    sources = scan_source_files(target_dir)
    fixup_includes(sources, fixups)


def parse_fixup_file(file: Path) -> dict[str, str | None]:
    with open(file) as f:
        return json.load(f)

def scan_source_files(dir: Path) -> set[Path]:
    """
    Returns the set of all source files in `dir`, with paths resolved
    relative to the `PROJECT_DIR`. This is the canonical form used to
    identify files from now on.
    """
    result: set[Path] = set()
    def scan(dir: Path):
        for file in dir.iterdir():
            if file.is_dir():
                scan(file)
            elif file.is_file():
                if file.suffix in { ".hpp", ".cpp", ".tpp" }:
                    canonical_form = file.relative_to(PROJECT_DIR)
                    result.add(canonical_form)
    scan(dir)
    return result

def fixup_includes(
    sources: Iterable[Path],
    fixups:  dict[str, str | None],
    dry_run: bool = False
):
    for source in sources:
        def replace(m: re.Match) -> str:
            quoted = m.group(1)
            unquoted = quoted[1:-1]

            if unquoted not in fixups:
                return m.group(0)

            fixed = fixups[unquoted]

            if fixed is None:
                print(f"{str(source)}: {unquoted} -> (removed)")
                return "" # Does this remove the line?

            l, r = quoted[0], quoted[-1]
            if l == '"' and r == '"':
                is_system = False
            elif l == '<' and r == '>':
                is_system = True
            else: # Wait, how is this possible at all?
                is_system = False
                print(f"WARNING: Invalid quoted form: {quoted} in file {source}.")

            print(f"{str(source)}: {unquoted} -> {fixed}")
            if is_system:
                return f'#include <{fixed}>\n'
            else:
                return f'#include "{fixed}"\n'

        with open(PROJECT_DIR / source, "r+") as f:
            text = f.read()
            new_text = re.sub(INCLUDE_PATTERN, repl=replace, string=text)
            if not dry_run:
                f.seek(0)
                f.truncate()
                f.write(new_text)


# Old stuff that is not used.

@dataclass
class RawInclude:
    path:      Path
    """Exactly as specified in the file."""
    is_system: bool
    """Whether using quotes or angled brackets <>."""

def scan_includes_of(sources: Iterable[Path]) -> dict[Path, list[RawInclude]]:
    """
    Returns a list of includes per-source in the raw form.
    The raw form is the one where the path is as-given in the include,
    not resolved relative to any directory.
    """
    result: dict[Path, list[RawInclude]] = { s: [] for s in sources }
    for source, includes in result.items():
        with open(PROJECT_DIR / source) as f:
            text = f.read()
            for m in re.finditer(INCLUDE_PATTERN, text):
                quoted = m.group(1)
                if quoted[0] == '"' and quoted[-1] == '"':
                    is_system = False
                elif quoted[0] == '<' and quoted[-1] == '>':
                    is_system = True
                else: # Wait, how is this possible at all?
                    print(f"WARNING: Invalid quoted form: {quoted} in file {source}.")
                raw_include = RawInclude(path=Path(quoted[1:-1]), is_system=is_system)
                includes.append(raw_include)
    return result

if __name__ == "__main__":
    main()
