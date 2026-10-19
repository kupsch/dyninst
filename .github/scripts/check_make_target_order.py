#!/usr/bin/env python3
"""Check that a Makefile build tree builds every target after the objects it uses.

  check_make_target_order.py BUILD_DIR

With the Makefile generator, a target's link or archive step can use the
object files of another target -- a static library archiving the objects
of the object libraries it links, a shared library linking them -- but
only the other target's own Makefile knows how to build them.  If the
target is not ordered after that other target, make fails with "No rule
to make target ..." whenever the link step runs first, so a parallel
build fails or not depending on timing.  CMAKE_OPTIMIZE_DEPENDENCIES
drops such orderings for static libraries, which is how #2509 broke
ENABLE_STATIC_LIBS=ON builds.

This reads the configured tree instead of building it, so the answer does
not depend on timing and nothing is compiled: for every target, the
object files its build.make lists that belong to another target, and the
targets CMakeFiles/Makefile2 makes it wait for, directly or through other
targets.  Every target whose objects are used must be among those waited
for.  Configure first; there is no need to build.

Exits 0 when every target is ordered, 1 when some are not, and 2 when
BUILD_DIR is not a configured Makefile build tree.
"""

import argparse
import glob
import os
import re
import sys

RULE = "-" * 50

# "a/CMakeFiles/x.dir/all: b/CMakeFiles/y.dir/all" in Makefile2: x waits for y.
ORDER = re.compile(r"^(\S+\.dir)/all: (\S+\.dir)/all$")

# "<output>: <dir>/CMakeFiles/<target>.dir/<file>.o" in a build.make: an
# object the output is made from, and the target directory that builds it.
OBJECT = re.compile(r"^\S+: (\S+/CMakeFiles/[^/]+\.dir)/\S+\.o$")


def section(title):
    return [RULE, "-- " + title, RULE]


def target_name(target_dir):
    """The target's name from its directory, "instructionAPI_static" from
    "instructionAPI/CMakeFiles/instructionAPI_static.dir"."""
    return os.path.basename(target_dir)[:-len(".dir")]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("build_dir", help="a build tree configured with the Unix Makefiles generator")
    args = ap.parse_args()
    build = os.path.abspath(args.build_dir)

    makefile2 = os.path.join(build, "CMakeFiles", "Makefile2")
    if not os.path.isfile(makefile2):
        print(f"{build} is not a configured Makefile build tree: no CMakeFiles/Makefile2",
              file=sys.stderr)
        return 2

    waits = {}
    with open(makefile2) as f:
        for line in f:
            m = ORDER.match(line)
            if m:
                waits.setdefault(m.group(1), set()).add(m.group(2))

    def waited_for(target_dir):
        seen, todo = set(), [target_dir]
        while todo:
            for d in waits.get(todo.pop(), ()):
                if d not in seen:
                    seen.add(d)
                    todo.append(d)
        return seen

    missing = []
    checked = 0
    for build_make in glob.glob(os.path.join(build, "**", "CMakeFiles", "*.dir", "build.make"),
                                recursive=True):
        target_dir = os.path.relpath(os.path.dirname(build_make), build)
        uses = set()
        with open(build_make) as f:
            for line in f:
                m = OBJECT.match(line)
                if m and m.group(1) != target_dir:
                    uses.add(m.group(1))
        if not uses:
            continue
        checked += 1
        for other in sorted(uses - waited_for(target_dir)):
            missing.append((target_name(target_dir), target_name(other)))

    out = section("Targets that use another target's objects")
    out.append(f"{checked} checked in {build}")
    if not missing:
        out.append("every one waits for the targets whose objects it uses")
        print("\n".join(out))
        return 0

    out += ["", ""] + section("Not ordered after the objects they use")
    width = max(len(t) for t, _ in missing)
    out += [f"{t:<{width}}  uses the objects of {o}" for t, o in sorted(missing)]
    out += ["", ""] + section("To fix")
    out += ["make can try to link these before the objects exist, and fail with",
            "\"No rule to make target ...\".  Make each wait for the target whose",
            "objects it uses, for example by keeping OPTIMIZE_DEPENDENCIES off",
            "for it, or with add_dependencies(<target> <other>)."]
    print("\n".join(out))
    return 1


if __name__ == "__main__":
    sys.exit(main())
