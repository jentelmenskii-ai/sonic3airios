"""Check the real font metrics against the compact menu's visible bounds."""
import json
import re
import sys
from pathlib import Path
root = Path(sys.argv[1])
def load_font(path):
    text = re.sub(r"//[^\n]*", "", (root / path).read_text())
    return json.loads(text)
def width(font, text, extra=0):
    return sum(int(font['characters'][ch].split()[2]) + int(font['space']) + extra for ch in text)
large = load_font('Oxygen/sonic3air/data/font/sonic3_fontC.json')
regular = load_font('Oxygen/oxygenengine/data/font/oxyfont_regular.json')
small = load_font('Oxygen/oxygenengine/data/font/oxyfont_small.json')
for text in ['NORMAL GAME','ACT SELECT','TIME ATTACK']:
    assert 138 + width(large,text) + 4 <= 320, text
for text in ['OPTIONS','EXTRAS','MODS','CLASSIC MODE','ANNIVERSARY MODE']:
    assert 152 + width(regular,text,1) + 4 <= 320, text
for text in ['Character Rotation:','Time Display:','Lives Display:','Speed Shoes Effect:','Fast Run Animation:','Flicker Effects:']:
    assert 130 - width(small,text) >= 0, text
for text in ['Original','Mobile','None (Original)','As Original']:
    assert width(small,text) <= 160, text
print('Compact menu font bounds passed: main entries, edition toggle and locked preset columns')

# Imported titles use actual glyph metrics, including Classic's narrow cards.
catalog = (root / 'Oxygen/sonic3air/source/sonic3air/data/RetroAchievementCatalog.h').read_text()
for line in catalog.splitlines():
    if line.startswith('\t{'):
        name = json.loads('[' + line.strip()[1:-2] + ']')[0].upper()
        assert width(regular, name) <= 206, name
print('All 53 imported achievement titles fit Classic cards')
