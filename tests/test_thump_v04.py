"""Legacy v0.4 EQ and limiter regression protections, stdlib only."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
cpp=(root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
head=(root/'Source/PluginProcessor.h').read_text(encoding='utf-8')
assert 'lowDivot[3]' in head
assert 'b.lowDivot[i].bell' in cpp
assert 'glueGrDb' in cpp
assert 'limiterBlend' in cpp
assert 'overGlueDb' in cpp
assert 'lookaheadOS / 4' in cpp
assert 'const float release = std::exp' in cpp
print('PASS: 1073-family divot, proportional-Q, glue and lookahead limiter retained')
