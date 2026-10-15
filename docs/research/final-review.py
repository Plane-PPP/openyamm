"""Read-only source audit; writes analytical CSV/Markdown only. Run from repository root.
Requires the 61 MM8 scene blobs in the local Git object store; no builds or gameplay.
"""
from pathlib import Path
import subprocess,os,hashlib,statistics,json
scene_names=subprocess.check_output(['git','ls-tree','-r','--name-only','HEAD','assets_dev/worlds/mm8/maps']).decode().splitlines()
scene_paths=[Path(p) for p in scene_names if p.endswith('.scene.yml')]
inputs=[]
import re,csv,collections
stats=list(csv.DictReader(Path('docs/research/monster-stats.csv').open()))
by_pic={r['picture'].lower():int(r['monster_id']) for r in stats}
rows=list(csv.reader(Path('assets_dev/engine/data_tables/map_stats.txt').open(),delimiter='\t'))
maps={Path(r[2]).stem:r for r in rows if r and r[0].isdigit() and int(r[0])<=61}
evidence=collections.defaultdict(set)
for p in scene_paths:
 key=p.name.removesuffix('.scene.yml'); base=key.removesuffix('_1'); r=maps[base]
 if key.endswith("_1"): continue
 s=subprocess.check_output(['git','show','HEAD:assets_dev/worlds/mm8/maps/'+p.name], env={**os.environ,'GIT_NO_LAZY_FETCH':'1'}).decode()
 inputs.append({'path':'assets_dev/worlds/mm8/maps/'+p.name,'sha256':hashlib.sha256(s.encode()).hexdigest()})
 for mid in re.findall(r'^\s+monster_info_id: (\d+)$',s,re.M): evidence[int(mid)].add(key+':placed')
 def encounter(slot,tiers,source):
  name=r[16+4*(slot-1)].strip()
  for tier in tiers:
   mid=by_pic.get((name+' '+tier).lower())
   if mid: evidence[mid].add(key+':'+source)
 for b in re.findall(r'  - spawn_index:.*?(?=  - spawn_index:|^[a-z_]+:|\Z)',s,re.S|re.M):
  t=re.search(r'^    type_id: (\d+)$',b,re.M); ix=re.search(r'^    index: (\d+)$',b,re.M)
  if not t or int(t[1])!=3: continue
  n=int(ix[1])
  if 1<=n<=3: encounter(n,'ABC' if int(r[18+4*(n-1)])>0 else 'A','spawn')
  elif 4<=n<=12: encounter((n-4)%3+1,'ABC'[(n-4)//3],'fixed-spawn')
 # Event slot/tier declarations are potential spawns, not proof every branch runs.
 ev=Path('assets_dev/worlds/mm8/events/maps')/(base+'.lua')
 if ev.exists():
  for slot,tier in re.findall(r'evt.SummonMonsters\((\d+), (\d+),',ev.read_text(encoding='latin1')):
   if 1<=int(slot)<=3 and 1<=int(tier)<=3: encounter(int(slot),'ABC'[int(tier)-1],'event')

def write(name,rs):
 with Path('docs/research',name).open('w',newline='') as f:
  w=csv.DictWriter(f,fieldnames=list(rs[0]),lineterminator='\n');w.writeheader();w.writerows(rs)
assert len(inputs)==61
native=stats[:198]
assert [int(r['monster_id']) for r in native]==list(range(1,199))
used=[r for r in native if int(r['monster_id']) in evidence]
assert len(used)==184 and not(set(evidence)-set(range(1,199)))
write('mm8-campaign-occurrences.csv',[dict(monster_id=r['monster_id'],name=r['name'],family=r['family'],campaign_evidence=int(int(r['monster_id']) in evidence),evidence='|'.join(sorted(evidence[int(r['monster_id'])]))) for r in native])
for p in ['assets_dev/engine/data_tables/map_stats.txt','docs/research/monster-stats.csv']+sorted(str(p) for p in Path('assets_dev/worlds/mm8/events/maps').glob('*.lua') if not p.stem.endswith('_mmmerge')):
 inputs.append(dict(path=p,sha256=hashlib.sha256(Path(p).read_bytes()).hexdigest()))
write('final-review-inputs.csv',inputs)
# Retain earlier scenarios; add I1. Impossible ranks remain explicitly marked.
matrix=[]
for r in native:
 for pl in [10,20,30,35,50]:
  for skill in [1,5,10,15,20]:
   for mastery,mult,minimum in [('Normal',1,1),('Expert',2,4),('Master',3,7),('Grandmaster',4,10)]:
    power=pl+mult*skill; difficulty=max(0,int(r['level'])-pl)
    matrix.append(dict(monster_id=r['monster_id'],name=r['name'],monster_level=r['level'],player_level=pl,skill_level=skill,mastery=mastery,power=power,difficulty=difficulty,success=int(power>=difficulty),mastery_numeric_requirement_met=int(skill>=minimum)))
write('identification-matrix.csv',matrix)
coverage=[]
for pl in [10,20,30,50]:
 for skill in [1,5,10]:
  prev=None
  for mastery,mult,minimum in [('Normal',1,1),('Expert',2,4),('Master',3,7),('Grandmaster',4,10)]:
   count=sum(int(r['level'])<=2*pl+mult*skill for r in used)
   coverage.append(dict(player_level=pl,skill_level=skill,mastery=mastery,max_monster_level=2*pl+mult*skill,identified=count,total=len(used),percent=round(100*count/len(used),4),new_vs_previous_rank='' if prev is None else count-prev,mastery_numeric_requirement_met=int(skill>=minimum)))
   prev=count
write('identification-coverage.csv',coverage)
metrics=[]
for label,xs in [('merged-all',stats),('mm8-descriptors',native),('mm8-campaign-evidence',used),('mm8-campaign-placed',[r for r in used if any(':placed' in e for e in evidence[int(r['monster_id'])])])]:
 for a,b in [('level','full_hp'),('height','level'),('height','full_hp'),('radius','level'),('radius','full_hp')]:
  metrics.append(dict(sample=label,n=len(xs),x=a,y=b,pearson=round(statistics.correlation([int(r[a]) for r in xs],[int(r[b]) for r in xs]),12)))
write('monster-correlations.csv',metrics)
lines=['| PL | I | Normal | Expert | Master | GM | Δ M/E | Δ GM/M |','| --- | --- | --- | --- | --- | --- | --- | --- |']
for i in range(0,len(coverage),4):
 rs=coverage[i:i+4]; cells=[f"{r['identified']}/184 ({r['percent']:.2f}%)"+(' †' if not r['mastery_numeric_requirement_met'] else '') for r in rs]
 lines.append('| '+' | '.join(map(str,[rs[0]['player_level'],rs[0]['skill_level']]+cells+[rs[2]['new_vs_previous_rank'],rs[3]['new_vs_previous_rank']]))+' |')
Path('/tmp/mm8-coverage-table.md').write_text('\n'.join(lines)+'\n')
print('61 scenes; 184 campaign-evidenced variants; matrix',len(matrix),'rows; deterministic outputs written')
