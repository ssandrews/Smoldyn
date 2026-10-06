#!/usr/bin/env python3
"""Generate source/Smoldyn/smolhelp_data.h from the Smoldyn User's Manual.

This reads the "Statements" and "Runtime commands" chapters of
docs/Smoldyn/SmoldynManual.tex, converts each \\item entry to plain text, and
writes a C header with the help table used by "smoldyn help <topic>".

Run this whenever those chapters of the manual change, and commit the
regenerated header:

    python3 scripts/make_smolhelp.py

Options:
    --manual PATH   LaTeX manual to read (default: docs/Smoldyn/SmoldynManual.tex)
    --output PATH   header to write (default: source/Smoldyn/smolhelp_data.h)
    --width N       wrap width for help text (default: 79)
"""

import argparse
import os
import re
import sys
import textwrap

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_MANUAL = os.path.join(REPO, "docs", "Smoldyn", "SmoldynManual.tex")
DEFAULT_OUTPUT = os.path.join(REPO, "source", "Smoldyn", "smolhelp_data.h")

# Chapters to extract, mapped to the category name stored in the table
CHAPTERS = {"Statements": "statement", "Runtime commands": "command"}

# Math-mode macros and their plain-text replacements
MATH_MACROS = {
    "langle": "<", "rangle": ">", "geq": ">=", "leq": "<=", "cdot": "*",
    "times": "*", "cos": "cos", "sin": "sin", "pi": "pi", "alpha": "alpha",
    "phi": "phi", "theta": "theta", "psi": "psi", "chi": "chi",
    "Delta": "delta_",
}

# Text-mode macros that are replaced by fixed text
TEXT_MACROS = {"Smoldyn": "Smoldyn", "BioNetGen": "BioNetGen", "BNG": "BNG"}

# Text-mode macros whose single argument is kept and the macro dropped
ARG_MACROS = ("ttt", "textit", "textbf", "emph", "texttt", "mbox", "text")

warnings = []


def warn(msg):
    warnings.append(msg)


def strip_comments(line):
    """Remove a LaTeX % comment, ignoring escaped \\%."""
    return re.sub(r"(?<!\\)%.*$", "", line)


def script_arg(m, mark):
    """Format a sub- or superscript; drop braces around simple tokens."""
    arg = m.group(1)
    if re.fullmatch(r"[A-Za-z0-9]+", arg):
        return mark + arg
    return mark + "(" + arg + ")"


def math2text(s):
    """Convert the contents of $...$ to plain text."""
    s = s.replace(r"\ ", " ")
    s = re.sub(r"\s*\\rangle", ">", s)
    s = re.sub(r"\\([A-Za-z]+)\s*",
               lambda m: MATH_MACROS.get(m.group(1), None) or
               (warn("unknown math macro \\" + m.group(1)) or m.group(1)), s)
    s = re.sub(r"_\{([^{}]*)\}", lambda m: script_arg(m, "_"), s)
    s = re.sub(r"\^\{([^{}]*)\}", lambda m: script_arg(m, "^"), s)
    s = s.replace("delta_ ", "delta_")
    return s


def tex2text(s):
    """Convert one paragraph or syntax line of LaTeX to plain text."""
    s = s.replace("\\\\", "\x00")                  # literal backslash in text
    s = s.replace(r"\$", "\x01")
    s = s.replace(r"\^{}", "^").replace(r"\~{}", "\x03")
    s = re.sub(r"\$([^$]*)\$", lambda m: math2text(m.group(1)), s)
    s = re.sub(r"[Ss]ection\s*\\ref\{[^}]*\}", "the Smoldyn User's Manual", s)
    s = re.sub(r"\\ref\{[^}]*\}", "the Smoldyn User's Manual", s)
    pattern = r"\\(?:" + "|".join(ARG_MACROS) + r")\{([^{}]*)\}"
    while re.search(pattern, s):
        s = re.sub(pattern, r"\1", s)
    for esc in "_#%&{}":
        s = s.replace("\\" + esc, "\x02" + esc)
    s = re.sub(r"\\([A-Za-z]+)\b\s?",
               lambda m: TEXT_MACROS.get(m.group(1), None) or
               (warn("unknown text macro \\" + m.group(1)) or m.group(1)), s)
    s = s.replace("{", "").replace("}", "")
    s = s.replace("\x02", "")
    s = s.replace("``", '"').replace("''", '"').replace("~", " ")
    s = s.replace("\x00", "\\").replace("\x01", "$").replace("\x03", "~")
    return re.sub(r"\s+", " ", s).strip()


def parse_manual(path):
    """Return a list of entries (dicts) from the reference chapters."""
    with open(path, encoding="utf-8") as fh:
        lines = [strip_comments(ln.rstrip("\n")) for ln in fh]

    entries = []
    category = None
    section = ""
    item = None                     # entry currently being collected
    in_header = False

    def finish():
        nonlocal item
        if item is not None:
            entries.append(item)
            item = None

    for ln in lines:
        m = re.match(r"\s*\\chapter\{(.*)\}", ln)
        if m:
            finish()
            category = CHAPTERS.get(m.group(1).strip())
            continue
        if category is None:
            continue
        m = re.match(r"\s*\\section\{(.*)\}", ln)
        if m:
            finish()
            section = tex2text(m.group(1))
            continue
        if re.match(r"\s*\\(begin|end)\{description\}", ln):
            finish()
            continue
        if re.match(r"\s*\\item", ln):
            if item is None or not in_header:
                finish()
                item = {"category": category, "section": section,
                        "header": [], "body": [[]]}
            in_header = True
        if item is None:
            continue                # text between sections and items
        if in_header:
            if ln.strip():
                item["header"].append(ln)
            else:
                in_header = False
        elif ln.strip():
            item["body"][-1].append(ln.strip())
        elif item["body"][-1]:
            item["body"].append([])
    finish()

    for e in entries:
        header = " ".join(e.pop("header"))
        syntax = []
        for piece in re.split(r"\\\\|\\item", header):
            text = tex2text(piece)
            if text:
                syntax.append(text)
        keys = []
        for text in syntax:
            word = text.lstrip("*").split()
            if word and word[0] not in keys:
                keys.append(word[0])
        e["syntax"] = syntax
        e["keys"] = keys
        e["block"] = syntax[0].startswith("*") if syntax else False
        e["body"] = [tex2text(" ".join(p)) for p in e["body"] if p]
        if not keys:
            warn("entry with no keyword in section " + e["section"])
    return [e for e in entries if e["keys"]]


def c_string(s, indent="\t\t"):
    """Return s as one or more concatenated C string literals."""
    s = s.replace("\\", "\\\\").replace('"', '\\"').replace("??", "?\\?")
    lines = s.split("\n")
    out = []
    for i, ln in enumerate(lines):
        nl = "\\n" if i < len(lines) - 1 else ""
        out.append('"' + ln + nl + '"')
    return ("\n" + indent).join(out)


def write_header(entries, path, width, manual):
    out = []
    out.append("/* smolhelp_data.h -- GENERATED FILE, DO NOT EDIT.")
    out.append(" Generated by scripts/make_smolhelp.py from")
    out.append(" " + os.path.relpath(manual, REPO).replace(os.sep, "/") +
               ". To update, edit the")
    out.append(" manual and rerun the script. */")
    out.append("")
    out.append("#define SMOLHELP_NENTRY %d" % len(entries))
    out.append("")
    out.append("static const smolhelpentry SmolHelpData[SMOLHELP_NENTRY]={")
    for e in entries:
        body = "\n\n".join(textwrap.fill(p, width=width,
                                         initial_indent="    ",
                                         subsequent_indent="    ",
                                         break_on_hyphens=False,
                                         break_long_words=False)
                           for p in e["body"])
        syntax = "\n".join("  " + s for s in e["syntax"])
        out.append("\t{%s,%s,%d,%s,\n\t\t%s,\n\t\t%s}," % (
            c_string(e["category"]), c_string(e["section"]),
            1 if e["block"] else 0, c_string(" ".join(e["keys"])),
            c_string(syntax), c_string(body)))
    out.append("\t};")
    out.append("")
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(out))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--manual", default=DEFAULT_MANUAL)
    ap.add_argument("--output", default=DEFAULT_OUTPUT)
    ap.add_argument("--width", type=int, default=79)
    args = ap.parse_args()

    entries = parse_manual(args.manual)
    if not entries:
        sys.exit("error: no statements found in " + args.manual)
    write_header(entries, args.output, args.width, args.manual)
    for w in sorted(set(warnings)):
        print("warning: " + w, file=sys.stderr)
    ncmd = sum(e["category"] == "command" for e in entries)
    print("wrote %d entries (%d statements, %d commands) to %s" % (
        len(entries), len(entries) - ncmd, ncmd, args.output))


if __name__ == "__main__":
    main()
