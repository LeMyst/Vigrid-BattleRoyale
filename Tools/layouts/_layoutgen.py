"""Shared output step for the layout generators in this folder.

Each generator builds a layout as a list of lines and hands it to emit(), which writes it to the
layouts folder (GUI/layouts/ unless told otherwise) as CRLF UTF-8, or with --check compares it with
the file and fails on a difference.
"""
import os
import sys

REPO = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
CHECK = '--check' in sys.argv


def emit(name, lines, folder='GUI/layouts'):
    path = os.path.normpath(os.path.join(REPO, folder, name))
    data = ("\r\n".join(lines) + "\r\n").encode("utf-8")

    if CHECK:
        current = open(path, 'rb').read() if os.path.exists(path) else b''
        if current != data:
            print(f"DIFFERS {name}: the layout was edited by hand or the generator changed")
            sys.exit(1)
        print(f"same    {name}")
        return

    open(path, 'wb').write(data)
    print(f"written {name} ({len(lines)} lines)")
