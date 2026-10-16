"""Compile the actual fern trig expressions against float/double overloads.

This targets C2668 from the supplied UE 5.8 build log. The signatures below
match the ones printed by that compiler diagnostic; they are stand-ins, not
Unreal headers. This does NOT compile the Unreal module or exercise rendering.
Optional positional argument: alternate ASWorld.cpp for a negative control.
"""
from pathlib import Path
import json
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / 'Source/AshenSpiral/Private/ASWorld.cpp'
    fern_lines = [line for line in source.read_text().splitlines() if 'TEXT("Ferns")' in line]
    if len(fern_lines) != 1:
        raise RuntimeError('Expected one fern-placement statement; update this focused fixture if placement is restructured.')
    calls = re.findall(r'FMath::(?:Cos|Sin)\((?:[^()]|\([^()]*\))*\)', fern_lines[0])
    if len(calls) != 2:
        raise RuntimeError('Could not extract both runtime fern trigonometry calls.')
    cpp = '''#include <cmath>
struct FMath {
    static float Cos(float x) { return std::cos(x); }
    static double Cos(double x) { return std::cos(x); }
    static float Sin(float x) { return std::sin(x); }
    static double Sin(double x) { return std::sin(x); }
};
int main() {
    for (int J=0; J<7; ++J) {
        const auto X = COS_CALL;
        const auto Y = SIN_CALL;
        if (!std::isfinite(X) || !std::isfinite(Y) || std::abs(X*X+Y*Y-1.0)>1e-6) return 1;
    }
}
'''.replace('COS_CALL', calls[0]).replace('SIN_CALL', calls[1])
    compiler = next((shutil.which(x) for x in ('g++', 'clang++', 'cl') if shutil.which(x)), None)
    if not compiler:
        print('NOT RUN: a native C++ compiler is required.')
        return 2
    with tempfile.TemporaryDirectory(prefix='ashen_trig_') as directory:
        path = Path(directory)
        fixture = path / 'trig.cpp'
        fixture.write_text(cpp)
        exe = path / ('trig.exe' if sys.platform == 'win32' else 'trig')
        if Path(compiler).stem.lower() == 'cl':
            cmd = [compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', str(fixture), '/Fe:'+str(exe)]
        else:
            cmd = [compiler, '-std=c++17', '-Wall', '-Wextra', '-pedantic', str(fixture), '-o', str(exe)]
        result = subprocess.run(cmd, cwd=directory, text=True, capture_output=True)
        if result.returncode:
            print(result.stdout + result.stderr)
            return 1
        subprocess.run([str(exe)], check=True)
    report = {'status': 'PASS', 'expressions_from_runtime': calls,
              'positions_checked': 7, 'compiler': Path(compiler).name,
              'scope': 'Extracted expressions with diagnostic-matched overload stand-ins; not an Unreal module compile.'}
    if len(sys.argv) == 1:
        (ROOT/'Tests/trig_compile_results.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: both runtime fern expressions compile with float/double overloads; seven positions are finite and on the unit circle. Unreal module compilation was not run.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
