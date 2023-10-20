import os
import re
import sys

class Trie:
    def __init__(self):
        self.child = {}
        self.leaf = None

    def add(self, fname, target_name):
        root = self
        for s in fname:
            root = root.child.setdefault(s, Trie())
        root.leaf = target_name

    def gen_switch(self, outf, char_index = 0, pref = ''):
        I = '    ' + char_index * '        '
        if self.test_unique(outf, char_index):
            return
        print(pref, file=outf, end='')
        print(f'{I}switch(key[{char_index}])', file=outf)
        print(f'{I}{{', file=outf)
        II = I + '    '
        if self.leaf is not None:
            print(f"{II}case 0: return CDNDef{{{self.leaf}_start, {self.leaf}_end}};", file=outf)
        for key, nxt in sorted(self.child.items()):
            print(f"{II}case '{key}':", file=outf, end='')
            nxt.gen_switch(outf, char_index+1, '\n')
        print(f'{II}default: return CDNDef{{}};', file=outf)
        print(f'{I}}}', file=outf)

    def test_unique(self, outf, char_index):
        key = ''
        root = self
        while True:
            if root.leaf is not None:
                print(f' return strcmp(key+{char_index}, "{key}") == 0 ? CDNDef{{{root.leaf}_start, {root.leaf}_end}} : CDNDef{{}};', file=outf);
                return True
            if len(root.child) != 1:
                return False
            for sym, root in root.child.items():
                key += sym
            

def scan_dir(dir_name, outf):
    files = os.listdir(dir_name)
    trie = Trie()

    for f in files:
        fname = re.sub(r'\W', '_', f'{dir_name}/{f}')
        trie.add(f, fname)
        print(f'    extern const unsigned char {fname}_start[] asm("_binary_{fname}_start");', file=outf)
        print(f'    extern const unsigned char {fname}_end[] asm("_binary_{fname}_end");', file=outf)
    print(file=outf)
    trie.gen_switch(outf)



def generate(out_base_name, dir_to_scan):
    with open(f'{out_base_name}.cpp', 'w') as f:
        print(f"""#include "{out_base_name}.h"
#include <string.h>
#include <stdint.h>

CDNDef decode_{out_base_name}_function(const char* key)
{{""", file=f);
        scan_dir(dir_to_scan, f)
        print("}", file=f)

if len(sys.argv) < 3:
    print("Usage: cdn_gen.py <input dir to scan> <output file to create (without extension)>")
else:
    generate(sys.argv[2], sys.argv[1])
