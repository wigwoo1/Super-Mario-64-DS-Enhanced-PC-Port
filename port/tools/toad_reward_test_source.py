"""Extract the original reward worker for a standalone test with explicit seams."""
import argparse
from pathlib import Path


def worker(source):
    anchor='void daKinopio_c::St_Talk_Main()\n{'
    if source.count(anchor)!=1:
        raise ValueError('Original Toad reward worker anchor changed')
    body=source[source.index(anchor):]
    end=body.index('\n#pragma opt_propagation on')
    body=body[:end]
    if not body.rstrip().endswith('}'):
        raise ValueError('Original Toad reward worker boundary changed')
    return body+'\n'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    text='// Generated verbatim original worker; test seams live in the runner.\n'+worker(args.source.read_text())
    args.output.parent.mkdir(parents=True,exist_ok=True)
    if not args.output.exists() or args.output.read_text()!=text:
        args.output.write_text(text)


if __name__=='__main__':main()
