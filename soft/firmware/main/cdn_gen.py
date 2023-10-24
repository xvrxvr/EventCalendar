import argparse
import os
import re
import sys

from dataclasses import dataclass
from enum import Enum

class Trie:
    def __init__(self):
        self.child = {}
        self.leaf = None

    def add(self, fname, target_name):
        root = self
        for s in fname:
            root = root.child.setdefault(s, self.__class__())
        root.leaf = target_name

    def gen_switch(self, outf, I = '    ', char_index = 0, pref = ''):
        # I = '    ' + indent * '        '
        unique_key, next_not_unique = self.detect_unique()
        if len(unique_key) > 1: # We really has chain - test for it
            if not next_not_unique.child: # Chain ended by leaf - test and return here
                print(f' if (strcmp(key+{char_index}, "{unique_key}") == 0) {{{next_not_unique.get_value("")}}} else {{{next_not_unique.get_empty("")}}}', file=outf);
            else:
                print(f' if (memcmp(key+{char_index}, "{unique_key}", {len(unique_key)}) != 0) {{{next_not_unique.get_empty("")}}}', file=outf);
                next_not_unique.gen_switch(outf, I + '      ', char_index + len(unique_key))
            return
        print(pref, file=outf, end='')
        print(f'{I}switch(key[{char_index}])', file=outf)
        print(f'{I}{{', file=outf)
        II = I + '    '
        if self.leaf is not None:
            print(f"{II}case 0: {self.get_value(II)}", file=outf)
        for key, nxt in sorted(self.child.items()):
            print(f"{II}case '{key}':", file=outf, end='')
            nxt.gen_switch(outf, I+'        ', char_index+1, '\n')
        print(f'{II}default: {self.get_empty(II)}', file=outf)
        print(f'{I}}}', file=outf)

    def detect_unique(self):
        "Test for chain on unique items (only 1 child). Returns tuple of combined key and last item in chain (not unique or really last)"
        key = ''
        root = self
        while True:
            if root.leaf is not None:
                return key, root
            if len(root.child) != 1:
                return key, root
            for sym, root in root.child.items():
                key += sym

    def test_unique(self, outf, char_index):
        key = ''
        root = self
        while True:
            if root.leaf is not None:
                print(f' if (strcmp(key+{char_index}, "{key}") == 0) {{{root.get_value("")}}} else {{{root.get_empty("")}}}', file=outf);
                return True
            if len(root.child) != 1:
                return False
            for sym, root in root.child.items():
                key += sym

class TrieDir(Trie):
    def get_value(self, indent):
        return f"return CDNDef{{{self.leaf}_start, {self.leaf}_end}};"

    def get_empty(self, indent):
        return 'return CDNDef{};'

def scan_dir(dir_name, outf):
    files = os.listdir(dir_name)
    trie = TrieDir()

    for f in files:
        fname = re.sub(r'\W', '_', f'{dir_name}/{f}')
        trie.add(f, fname)
        print(f'    extern const unsigned char {fname}_start[] asm("_binary_{fname}_start");', file=outf)
        print(f'    extern const unsigned char {fname}_end[] asm("_binary_{fname}_end");', file=outf)
    print(file=outf)
    trie.gen_switch(outf)



def generate_dir(out_base_name, dir_to_scan):
    with open(f'{out_base_name}.cpp', 'w') as f:
        print(f"""#include "{out_base_name}.h"
#include <string.h>
#include <stdint.h>

CDNDef decode_{out_base_name}_function(const char* key)
{{""", file=f);
        scan_dir(dir_to_scan, f)
        print("}", file=f)
#################################################################################################################################################################
VarType = Enum('VarType', ['INT', 'FUNC', 'INTFUNC'])

@dataclass
class VarEntry:
    name: str
    type: VarType

class VarsTrie(Trie):
    def get_value(self, indent):
        if self.is_string:
            if self.leaf.type is VarType.INT:
                return f"ans.write_int({self.leaf.name}); return;"
            elif self.leaf.type is VarType.FUNC:
                return f"{self.leaf.name}(ans); return;"
            else:
                return f"ans.write_int({self.leaf.name}()); return;"
        else:
            if self.leaf.type is VarType.FUNC:
                return 'err_type_wrong(key); return 0;'
            elif self.leaf.type is VarType.INT:
                return f"return {self.leaf.name};"
            else:
                return f"return {self.leaf.name}();"
                
    def get_empty(self, indent):
        return f'err_novar(key); return{"" if self.is_string else " 0"};'
    

def load_vars_file(fname, outf):
    start_seen = False
    trie = VarsTrie()
    with open(fname, "r", encoding='UTF8') as f:
        for line in f:
            if re.match(r'\s*struct\s+WebOptions\b',line):
                start_seen = True
                continue
            if not start_seen:
                continue
            if re.match(r'\s*};|\s*private', line):
                break
            mtch = re.match(r'\s*void\s+(\w+)\(Ans&\);', line)
            if mtch:
                trie.add(mtch.group(1), VarEntry(mtch.group(1), VarType.FUNC))
                continue
            mtch = re.match(r'\s*u?int\d+_t\s+(\w+);', line)
            if mtch:
                trie.add(mtch.group(1), VarEntry(mtch.group(1), VarType.INT))
                continue
            mtch = re.match(r'\s*u?int\d+_t\s+(\w+)\(\);', line)
            if mtch:
                trie.add(mtch.group(1), VarEntry(mtch.group(1), VarType.INTFUNC))
                continue

    VarsTrie.is_string = True
    for tp, fid in (('void', 'decode_inline'), ('uint32_t', 'get_condition')):
        print(f"{tp} WebOptions::{fid}(const char* key, Ans& ans)\n{{", file=outf)
        trie.gen_switch(outf)
        print("}\n", file=outf)
        VarsTrie.is_string = False


parser = argparse.ArgumentParser(prog='TextGen', description='Generates source codes for different parts of EventCalendar system')

parser.add_argument('action', 
    choices=['webswitch','dirscan'], 
    help='Defines action to do: webswitch - Generates source code for WEB pages substitution switch, dirscan - Scan directory with WEB pages and generates access source code'
)
parser.add_argument('source', help="Source file or dir")
parser.add_argument('output', help="Base output file name (without extension)")

args = parser.parse_args()

if args.action == 'dirscan':
    generate_dir(args.output, args.source)
elif args.action == 'webswitch':
    with open(args.output+".cpp", "w") as f:
        print(f'#include <string.h>\n#include "{args.source}"\n', file=f)
        load_vars_file(args.source, f)
else:
    assert False, f"Unknown command {args.action}"


