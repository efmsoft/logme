#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Release prep checker/updater for logme.
# Comments intentionally in English.

from __future__ import annotations

import argparse
import re
import sys
import uuid
from dataclasses import dataclass
from pathlib import Path
from typing import List, Set, Tuple


@dataclass
class TextFile:
  path: Path
  text: str
  newline: str
  has_bom: bool


def ReadTextFile(path: Path) -> TextFile:
  data = path.read_bytes()
  has_bom = data.startswith(b"\xef\xbb\xbf")
  text = data.decode("utf-8-sig", errors="replace")
  newline = "\r\n" if "\r\n" in text else "\n"
  return TextFile(
    path=path,
    text=text,
    newline=newline,
    has_bom=has_bom,
  )


def WriteTextFile(tf: TextFile) -> None:
  data = tf.text.encode("utf-8")
  if tf.has_bom:
    data = b"\xef\xbb\xbf" + data
  tf.path.write_bytes(data)


def NormalizeSlashesToBackslash(s: str) -> str:
  return s.replace("/", "\\")


def CollectFiles(root: Path, rel_dir: str, exts: Tuple[str, ...]) -> List[str]:
  base = root / rel_dir
  if not base.exists():
    return []
  out: List[str] = []
  for p in base.rglob("*"):
    if not p.is_file():
      continue
    if p.suffix.lower() not in exts:
      continue
    rel = p.relative_to(root / rel_dir)
    out.append(NormalizeSlashesToBackslash(str(Path(rel_dir) / rel)))
  return sorted(out)


def ParseVersion(version: str) -> Tuple[int, int, int]:
  m = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", version.strip())
  if not m:
    raise ValueError(f"Invalid version '{version}', expected X.Y.Z")
  return (int(m.group(1)), int(m.group(2)), int(m.group(3)))


def CheckChangelog(tf: TextFile, version: str) -> bool:
  pat = re.compile(rf"(?m)^\s*##\s+{re.escape(version)}\s*$")
  return pat.search(tf.text) is not None


def UpdateCMakeProjectVersion(tf: TextFile, version: str) -> Tuple[bool, str]:
  pat = re.compile(
    r"(?m)^(?P<prefix>\s*project\s*\(\s*logme\s+VERSION\s+)"
    r"(?P<ver>\d+\.\d+\.\d+)"
    r"(?P<suffix>\s*\)\s*)$"
  )
  m = pat.search(tf.text)
  if not m:
    return (False, "Cannot find project(logme VERSION X.Y.Z) in CMakeLists.txt")
  cur = m.group("ver")
  if cur == version:
    return (False, "")
  tf.text = pat.sub(rf"\g<prefix>{version}\g<suffix>", tf.text, count=1)
  return (True, f"CMakeLists.txt: project version {cur} -> {version}")


def ExtractVcxprojClCompileIncludes(text: str) -> Set[str]:
  return set(re.findall(r'<ClCompile\s+Include="([^"]+)"', text))


def InsertBeforeClosingTag(block: str, insert_text: str, closing_tag: str) -> Tuple[str, bool]:
  idx = block.rfind(closing_tag)
  if idx < 0:
    return (block, False)
  return (block[:idx] + insert_text + block[idx:], True)


def EnsureVcxprojHasAllCpp(tf: TextFile, expected_cpp: List[str], prefix: str) -> Tuple[bool, List[str]]:
  expected_includes = [prefix + p for p in expected_cpp]
  present = ExtractVcxprojClCompileIncludes(tf.text)
  missing = [p for p in expected_includes if p not in present]
  if not missing:
    return (False, [])

  text = tf.text
  first = re.search(
    r"<ItemGroup>\s*(?:\r?\n|\r)(?:.*(?:\r?\n|\r))*?\s*<ClCompile\s+Include=",
    text
  )
  if not first:
    return (False, [f"{tf.path}: Cannot find <ItemGroup> with <ClCompile> entries"])

  start_pos = first.start()
  end_pos = text.find("</ItemGroup>", first.end())
  if end_pos < 0:
    return (False, [f"{tf.path}: Cannot find closing </ItemGroup> for ClCompile group"])

  end_pos += len("</ItemGroup>")
  group = text[start_pos:end_pos]

  nl = tf.newline
  ins = ""
  for inc in missing:
    ins += f"    <ClCompile Include=\"{inc}\" />{nl}"

  new_group, ok = InsertBeforeClosingTag(group, ins, "</ItemGroup>")
  if not ok:
    return (False, [f"{tf.path}: Failed to insert into ClCompile group"])

  tf.text = text[:start_pos] + new_group + text[end_pos:]
  return (True, [f"{tf.path}: added {len(missing)} .cpp entries"])


def ExtractFilters(text: str) -> Set[str]:
  return set(re.findall(r'<Filter\s+Include="([^"]+)"', text))


def ExtractFiltersClEntries(text: str, tag: str) -> Set[str]:
  return set(re.findall(rf'<{tag}\s+Include="([^"]+)"', text))


def MakeDeterministicGuid(name: str) -> str:
  g = uuid.uuid5(uuid.NAMESPACE_URL, "logme-release-prep:" + name)
  return "{" + str(g) + "}"


def EnsureFiltersDefinitions(tf: TextFile, needed_filters: List[str]) -> Tuple[bool, List[str]]:
  present = ExtractFilters(tf.text)
  missing = [f for f in needed_filters if f not in present]
  if not missing:
    return (False, [])

  text = tf.text
  m = re.search(r"<ItemGroup>\s*(?:\r?\n|\r)\s*<Filter\s+Include=", text)
  if not m:
    return (False, [f"{tf.path}: Cannot find <ItemGroup> with <Filter> definitions"])

  start_pos = m.start()
  end_pos = text.find("</ItemGroup>", m.end())
  if end_pos < 0:
    return (False, [f"{tf.path}: Cannot find closing </ItemGroup> for Filter definitions"])
  end_pos += len("</ItemGroup>")
  group = text[start_pos:end_pos]

  nl = tf.newline
  ins = ""
  for flt in missing:
    guid = MakeDeterministicGuid(str(tf.path) + "::" + flt)
    ins += f"    <Filter Include=\"{flt}\">{nl}"
    ins += f"      <UniqueIdentifier>{guid}</UniqueIdentifier>{nl}"
    ins += f"    </Filter>{nl}"

  new_group, ok = InsertBeforeClosingTag(group, ins, "</ItemGroup>")
  if not ok:
    return (False, [f"{tf.path}: Failed to insert Filter definitions"])

  tf.text = text[:start_pos] + new_group + text[end_pos:]
  return (True, [f"{tf.path}: added {len(missing)} filter definitions"])


def GuessFilterForPath_StaticProject(rel_path: str) -> str:
  p = rel_path.replace("/", "\\")
  d = str(Path(p).parent).replace("/", "\\")
  return d


def GuessFilterForPath_DynamicProject(rel_path: str) -> str:
  p = rel_path.replace("/", "\\")
  if p.startswith("..\\"):
    p2 = p[3:]
  else:
    p2 = p

  parts = p2.split("\\")
  if len(parts) >= 3 and parts[1].lower() == "source":
    group = parts[2]
    if group.lower() == "control" and len(parts) >= 4 and parts[3].lower() == "command":
      return "Control\\Command"
    return group
  if len(parts) >= 3 and parts[1].lower() == "include":
    return str(Path(p).parent).replace("/", "\\")
  return str(Path(p).parent).replace("/", "\\")


def EnsureFiltersHaveAllFiles(
  tf: TextFile,
  expected_cpp: List[str],
  expected_h: List[str],
  prefix: str,
  filter_mode: str
) -> Tuple[bool, List[str]]:
  expected_cpp_inc = [prefix + p for p in expected_cpp]
  expected_h_inc = [prefix + p for p in expected_h]

  present_cpp = ExtractFiltersClEntries(tf.text, "ClCompile")
  present_h = ExtractFiltersClEntries(tf.text, "ClInclude")

  missing_cpp = [p for p in expected_cpp_inc if p not in present_cpp]
  missing_h = [p for p in expected_h_inc if p not in present_h]

  if not missing_cpp and not missing_h:
    return (False, [])

  nl = tf.newline

  def guess_filter(p: str) -> str:
    if filter_mode == "dynamic":
      return GuessFilterForPath_DynamicProject(p)
    return GuessFilterForPath_StaticProject(p)

  needed_filters: List[str] = []
  for p in missing_cpp + missing_h:
    needed_filters.append(guess_filter(p))

  seen: Set[str] = set()
  needed_filters_u: List[str] = []
  for f in needed_filters:
    if f not in seen:
      seen.add(f)
      needed_filters_u.append(f)

  changed_defs, msgs = EnsureFiltersDefinitions(tf, needed_filters_u)

  text = tf.text
  changed_any = changed_defs

  if missing_h:
    m = re.search(r"<ItemGroup>\s*(?:\r?\n|\r)\s*<ClInclude\s+Include=", text)
    if not m:
      msgs.append(f"{tf.path}: Cannot find <ItemGroup> with <ClInclude> entries")
    else:
      start_pos = m.start()
      end_pos = text.find("</ItemGroup>", m.end())
      if end_pos < 0:
        msgs.append(f"{tf.path}: Cannot find closing </ItemGroup> for ClInclude group")
      else:
        end_pos += len("</ItemGroup>")
        group = text[start_pos:end_pos]
        ins = ""
        for inc in missing_h:
          flt = guess_filter(inc)
          ins += f"    <ClInclude Include=\"{inc}\">{nl}"
          ins += f"      <Filter>{flt}</Filter>{nl}"
          ins += f"    </ClInclude>{nl}"
        new_group, ok = InsertBeforeClosingTag(group, ins, "</ItemGroup>")
        if not ok:
          msgs.append(f"{tf.path}: Failed to insert missing ClInclude entries")
        else:
          text = text[:start_pos] + new_group + text[end_pos:]
          changed_any = True
          msgs.append(f"{tf.path}: added {len(missing_h)} header entries")

  if missing_cpp:
    m = re.search(
      r"<ItemGroup>\s*(?:\r?\n|\r)(?:.*(?:\r?\n|\r))*?\s*<ClCompile\s+Include=",
      text
    )
    if not m:
      msgs.append(f"{tf.path}: Cannot find <ItemGroup> with <ClCompile> entries")
    else:
      start_pos = m.start()
      end_pos = text.find("</ItemGroup>", m.end())
      if end_pos < 0:
        msgs.append(f"{tf.path}: Cannot find closing </ItemGroup> for ClCompile group")
      else:
        end_pos += len("</ItemGroup>")
        group = text[start_pos:end_pos]
        ins = ""
        for inc in missing_cpp:
          flt = guess_filter(inc)
          ins += f"    <ClCompile Include=\"{inc}\">{nl}"
          ins += f"      <Filter>{flt}</Filter>{nl}"
          ins += f"    </ClCompile>{nl}"
        new_group, ok = InsertBeforeClosingTag(group, ins, "</ItemGroup>")
        if not ok:
          msgs.append(f"{tf.path}: Failed to insert missing ClCompile entries")
        else:
          text = text[:start_pos] + new_group + text[end_pos:]
          changed_any = True
          msgs.append(f"{tf.path}: added {len(missing_cpp)} .cpp entries")

  tf.text = text
  return (changed_any, msgs)


def UpdateVersionHeader(tf: TextFile, version: str) -> Tuple[bool, str]:
  major, minor, patch = ParseVersion(version)
  text = tf.text

  def repl_define(name: str, value: str, t: str) -> Tuple[str, bool]:
    pat = re.compile(rf"(?m)^(#define\s+{re.escape(name)}\s+)(\d+|\"[^\"]*\")\s*$")
    m = pat.search(t)
    if not m:
      return (t, False)
    cur = m.group(2)
    if cur == value:
      return (t, False)
    t2 = pat.sub(rf"\g<1>{value}", t, count=1)
    return (t2, True)

  changed = False
  text, c1 = repl_define("LOGME_VERSION_MAJOR", str(major), text)
  changed |= c1
  text, c2 = repl_define("LOGME_VERSION_MINOR", str(minor), text)
  changed |= c2
  text, c3 = repl_define("LOGME_VERSION_PATCH", str(patch), text)
  changed |= c3
  text, c4 = repl_define("LOGME_VERSION_STRING", f"\"{version}\"", text)
  changed |= c4

  if not changed:
    return (False, "")
  tf.text = text
  return (True, f"{tf.path}: updated to {version}")


def FormatMissingList(items: List[str], indent: str = "    ") -> str:
  if not items:
    return ""
  lines = [indent + x for x in items]
  return "\n" + "\n".join(lines)


def Main() -> int:
  ap = argparse.ArgumentParser(
    prog="release.py",
    description="Prepare logme release: validate and optionally update project files."
  )
  ap.add_argument("version", nargs="?", help="Release version X.Y.Z (e.g. 2.4.10)")
  ap.add_argument("--update", action="store_true", help="Apply updates for items 2-5 (changelog is only checked).")
  ap.add_argument("--root", default=".", help="Repository root (default: current directory)")
  args = ap.parse_args()

  if not args.version:
    ap.print_help(sys.stderr)
    print("\nERROR: missing required argument: version", file=sys.stderr)
    return 2

  try:
    ParseVersion(args.version)
  except Exception as e:
    print(f"ERROR: {e}", file=sys.stderr)
    return 2

  root = Path(args.root).resolve()

  changelog_path = root / "changelog.md"
  cmake_path = root / "CMakeLists.txt"

  static_vcxproj = root / "logme" / "logme.vcxproj"
  static_filters = root / "logme" / "logme.vcxproj.filters"

  dynamic_vcxproj = root / "dynamic" / "logmed.vcxproj"
  dynamic_filters = root / "dynamic" / "logmed.vcxproj.filters"

  version_h_path = root / "logme" / "include" / "Logme" / "version.h"

  required = [
    changelog_path,
    cmake_path,
    static_vcxproj,
    static_filters,
    dynamic_vcxproj,
    dynamic_filters,
    version_h_path,
  ]

  missing_files: List[Path] = []
  for p in required:
    if not p.exists():
      missing_files.append(p)

  if missing_files:
    print("ERROR: Missing expected files:", file=sys.stderr)
    for p in missing_files:
      print(f"  - {p}", file=sys.stderr)
    return 3

  expected_cpp_full = CollectFiles(root, "logme/source", (".cpp",))
  expected_h_full = CollectFiles(root, "logme/include", (".h",))
  expected_h_full += CollectFiles(root, "logme/source", (".h",))
  expected_h_full = sorted(set(expected_h_full))

  expected_cpp_rel = [p.replace("logme\\", "") for p in expected_cpp_full]
  expected_h_rel = [p.replace("logme\\", "") for p in expected_h_full]

  changelog = ReadTextFile(changelog_path)
  ok_changelog = CheckChangelog(changelog, args.version)

  cmake = ReadTextFile(cmake_path)
  cmake_ok = re.search(
    rf"(?m)^\s*project\s*\(\s*logme\s+VERSION\s+{re.escape(args.version)}\s*\)\s*$",
    cmake.text
  ) is not None

  st_vcx = ReadTextFile(static_vcxproj)
  dy_vcx = ReadTextFile(dynamic_vcxproj)

  st_present = ExtractVcxprojClCompileIncludes(st_vcx.text)
  dy_present = ExtractVcxprojClCompileIncludes(dy_vcx.text)

  st_expected_cpp_includes = sorted([NormalizeSlashesToBackslash(p) for p in expected_cpp_rel])
  dy_expected_cpp_includes = sorted(["..\\logme\\" + NormalizeSlashesToBackslash(p) for p in expected_cpp_rel])

  st_missing_cpp = [p for p in st_expected_cpp_includes if p not in st_present]
  dy_missing_cpp = [p for p in dy_expected_cpp_includes if p not in dy_present]

  st_flt = ReadTextFile(static_filters)
  dy_flt = ReadTextFile(dynamic_filters)

  st_present_f_cpp = ExtractFiltersClEntries(st_flt.text, "ClCompile")
  st_present_f_h = ExtractFiltersClEntries(st_flt.text, "ClInclude")

  dy_present_f_cpp = ExtractFiltersClEntries(dy_flt.text, "ClCompile")
  dy_present_f_h = ExtractFiltersClEntries(dy_flt.text, "ClInclude")

  st_expected_h_includes = sorted([NormalizeSlashesToBackslash(p) for p in expected_h_rel])
  dy_expected_h_includes = sorted(["..\\logme\\" + NormalizeSlashesToBackslash(p) for p in expected_h_rel])

  st_missing_f_cpp = [p for p in st_expected_cpp_includes if p not in st_present_f_cpp]
  st_missing_f_h = [p for p in st_expected_h_includes if p not in st_present_f_h]

  dy_missing_f_cpp = [p for p in dy_expected_cpp_includes if p not in dy_present_f_cpp]
  dy_missing_f_h = [p for p in dy_expected_h_includes if p not in dy_present_f_h]

  ver_h = ReadTextFile(version_h_path)
  major, minor, patch = ParseVersion(args.version)
  ok_ver_h = (
    re.search(rf"(?m)^\s*#define\s+LOGME_VERSION_MAJOR\s+{major}\s*$", ver_h.text) is not None
    and re.search(rf"(?m)^\s*#define\s+LOGME_VERSION_MINOR\s+{minor}\s*$", ver_h.text) is not None
    and re.search(rf"(?m)^\s*#define\s+LOGME_VERSION_PATCH\s+{patch}\s*$", ver_h.text) is not None
    and re.search(rf"(?m)^\s*#define\s+LOGME_VERSION_STRING\s+\"{re.escape(args.version)}\"\s*$", ver_h.text) is not None
  )

  issues: List[str] = []

  if not ok_changelog:
    issues.append(f"1) changelog.md: missing section '## {args.version}'")

  if not cmake_ok:
    issues.append(f"2) CMakeLists.txt: project(logme VERSION {args.version}) not found")

  if st_missing_cpp:
    issues.append(
      f"3) {static_vcxproj}: missing .cpp entries ({len(st_missing_cpp)}):"
      + FormatMissingList(st_missing_cpp)
    )

  if dy_missing_cpp:
    issues.append(
      f"3) {dynamic_vcxproj}: missing .cpp entries ({len(dy_missing_cpp)}):"
      + FormatMissingList(dy_missing_cpp)
    )

  if st_missing_f_cpp or st_missing_f_h:
    msg = f"4) {static_filters}: missing filter entries:"
    if st_missing_f_cpp:
      msg += f"\n  - .cpp ({len(st_missing_f_cpp)}):" + FormatMissingList(st_missing_f_cpp, indent="      ")
    if st_missing_f_h:
      msg += f"\n  - .h   ({len(st_missing_f_h)}):" + FormatMissingList(st_missing_f_h, indent="      ")
    issues.append(msg)

  if dy_missing_f_cpp or dy_missing_f_h:
    msg = f"4) {dynamic_filters}: missing filter entries:"
    if dy_missing_f_cpp:
      msg += f"\n  - .cpp ({len(dy_missing_f_cpp)}):" + FormatMissingList(dy_missing_f_cpp, indent="      ")
    if dy_missing_f_h:
      msg += f"\n  - .h   ({len(dy_missing_f_h)}):" + FormatMissingList(dy_missing_f_h, indent="      ")
    issues.append(msg)

  if not ok_ver_h:
    issues.append(f"5) {version_h_path}: version macros do not match {args.version}")

  if issues and not args.update:
    print("FAILED checks:")
    for s in issues:
      print("  - " + s.replace("\n", "\n    "))
    return 1

  if args.update:
    changes: List[str] = []

    changed, msg = UpdateCMakeProjectVersion(cmake, args.version)
    if changed:
      WriteTextFile(cmake)
      if msg:
        changes.append(msg)

    changed_s, msgs_s = EnsureVcxprojHasAllCpp(st_vcx, expected_cpp_rel, prefix="")
    if changed_s:
      WriteTextFile(st_vcx)
      changes.extend(msgs_s)

    changed_d, msgs_d = EnsureVcxprojHasAllCpp(dy_vcx, expected_cpp_rel, prefix="..\\logme\\")
    if changed_d:
      WriteTextFile(dy_vcx)
      changes.extend(msgs_d)

    st_changed, st_msgs = EnsureFiltersHaveAllFiles(
      st_flt,
      expected_cpp=expected_cpp_rel,
      expected_h=expected_h_rel,
      prefix="",
      filter_mode="static"
    )
    if st_changed:
      WriteTextFile(st_flt)
      changes.extend(st_msgs)

    dy_changed, dy_msgs = EnsureFiltersHaveAllFiles(
      dy_flt,
      expected_cpp=expected_cpp_rel,
      expected_h=expected_h_rel,
      prefix="..\\logme\\",
      filter_mode="dynamic"
    )
    if dy_changed:
      WriteTextFile(dy_flt)
      changes.extend(dy_msgs)

    vchanged, vmsg = UpdateVersionHeader(ver_h, args.version)
    if vchanged:
      WriteTextFile(ver_h)
      if vmsg:
        changes.append(vmsg)

    if changes:
      print("UPDATED:")
      for c in changes:
        if c:
          print("  - " + c)
    else:
      print("Nothing to update (2-5 already match).")

    changelog2 = ReadTextFile(changelog_path)
    if not CheckChangelog(changelog2, args.version):
      print(f"NOTE: changelog.md still missing section '## {args.version}'")
      return 1

  print("OK")
  return 0


if __name__ == "__main__":
  sys.exit(Main())