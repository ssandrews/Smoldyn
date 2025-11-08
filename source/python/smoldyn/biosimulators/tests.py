import os
import json
from tempfile import mkdtemp as temp_dir
from smoldyn.biosimulators.combine import (
    exec_sedml_docs_in_combine_archive as exec_archive,
)


def test_simularium():
    archive_path = (
        "/Users/alex/Desktop/uchc_work/repos/Smoldyn/examples/S99_more/Min/Min1.omex"
    )
    # output_dir = '/Users/alex/Desktop/uchc_work'
    output_dir = temp_dir()

    exec_archive(archive_path, output_dir)
    files = [f for f in os.listdir(output_dir)]
    print(f"THE FILES:\n{files}\n")
    for f in files:
        if "simularium" in f:
            with open(os.path.join(output_dir, f), "r") as fp:
                data = json.load(fp)
                print(f"THE SIMULARIUM:\n{data}")


if __name__ == "__main__":
    test_simularium()
