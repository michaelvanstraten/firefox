#!/usr/bin/env python

import sys
import os
from pathlib import Path

from buildconfig import topsrcdir

THIS_DIR = Path(os.path.dirname(__file__))
TEMPLATES = THIS_DIR / "templates"


def main(include_dir, outpath):
    exports = {}
    for dirname, _, filenames in os.walk(include_dir):
        headers = list(filter(lambda f: f.endswith((".h", ".hh", ".inc")), filenames))
        if not headers:
            continue

        absdir = Path(dirname).resolve().relative_to(topsrcdir)
        includes = [absdir / filename for filename in headers]
        exports[str(Path(dirname).relative_to(include_dir)).replace("/", ".")] = sorted(includes)

    from jinja2 import Environment, FileSystemLoader

    env = Environment(loader=FileSystemLoader(TEMPLATES))
    template = env.get_template("moz.build.jinja2")

    with open(outpath, "w") as f:
        f.write(template.render(exports=exports))


if __name__ == "__main__":
    main(*sys.argv[1:])
