import os
import subprocess

PATCH_FORMAT = """--- {from_file}
+++ {to_file}
{diff}
"""


def patch_mutant(difference, location):
    directory = os.path.dirname(location)
    file_name = os.path.basename(location)
    patch_stdin = PATCH_FORMAT.format(
        from_file=file_name,
        to_file=file_name,
        diff=difference,
    )
    print(patch_stdin)
    patch_cmd = subprocess.Popen(
        [f'patch -p0 -d{directory}'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        shell=True,
    )
    patch_cmd.stdin.write(str.encode(patch_stdin))
    output, error = patch_cmd.communicate()

    # TODO fix error checking
    if error:
        raise Exception(error)

    return output
