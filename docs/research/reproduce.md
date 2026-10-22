> Дополнение2026-10-22: актуальный генератор v0.4 учитывает Luck0/90: B=0.5*floor(max(0,Luck)/10) до KL/cap. Старые v0.2 расчёты ниже исторические.

# Воспроизведение исследований

> **Историческая методика v0.2.** Встроенный генератор ниже сохранён для восстановления
> предыдущих исследований; он перезаписывает исторические формулы и предварительную классификацию.
> Для текущей версии используйте только `python3 tools/generate_lethal_strike_research.py`,
> затем `python3 docs/research/final-review.py` для повторения инвентарного coverage/correlation audit.
> Новый генератор берёт утверждённую классификацию из lethal-strike-classification.csv,
> обновляет compiled registry, категории monster-stats.csv и lethal-strike-probabilities-v0.4.csv.
> death-strike-probabilities.csv и combat-scenarios.csv остаются историческими v0.2;
> identification-matrix.csv и identification-coverage.csv относятся к Deferred Identify Monster.


Статус: **Research**. Назначение: точное восстановление CSV и методики,
этапы R1/R2/R6 [ROADMAP](../ROADMAP.md#этапы-разработки).
Подтверждённый срез: commit `142a54e01da50d9faf258e38919e88dba9e8de29`, 2026-10-14.
Игровой код, ресурсы и сохранения не меняются; новый баланс не внедряется.

## Проверяемые входы

SHA256 оригинального MM8 dmonlist.bin: `c62c7f8af192e65f0db4d86d9157b9d5657434c1033967e2670fb6256775a4b5`.
Файл взят read-only из Git; если объект отсутствует в sparse checkout, Git может запросить сеть.
Доступ к GitHub Actions используется только для метаданных уже завершённой build #3, не запуска сборки.

| Активная таблица | SHA256 |
| --- | --- |
| monster_data.txt | `a32cb5a5db20edd7359977cc6791cc0e01465f22563526331d9c1a6af8303389` |
| monster_descriptors.txt | `7f198a01577115ef06e4ad3f10ca03a275b10c11b9dd6f0b568a06b823fa8f9f` |
| bolster_monsters.txt | `d640f02f9233100fccdcc60943916f002070498ad620d6be8d39a0fde7faa14a` |
| class_skills.txt | `ff66d338bb5a3ddb8d52830f921ccee12b8c863cdb4d5589e6a23c56b6143f1a` |
| race_skills.yml | `1c3958863848a9c45ba0d6a29a06e448bb7d3ebe76d1d792b10ad22f992875ee` |

## Процедура

Из корня репозитория извлечь оригинальный бинарник в `/tmp`:

```sh
git show HEAD:assets_dev/worlds/mm8/_legacy/bin_tables/dmonlist.bin > /tmp/openyamm-mm8-dmonlist.bin
```

Сохранить Python-блок ниже во временный `/tmp/openyamm_research.py`, выполнить
`python3 /tmp/openyamm_research.py` из корня репозитория. Только стандартная библиотека Python3.
Генератор перезаписывает четыре аналитических CSV в docs/research; вспомогательные Markdown
таблицы и JSON summary пишет в `/tmp`. Это не игровые таблицы. После изменения inputs
сверить hashes и вручную актуализировать выводы/таблицы Markdown; текущие дизайны автоматически
не перезаписываются. Никаких exe/build/test/CI/save операций.

Подсчёты имеют проверяемые ограничения: 198 оригинальных descriptor records, имена совпадают,
IDs уникальны; фиксированный damage и точный дискретный resistance distribution, прозрачные
допущения hit/skill/recovery. CSV матрица включает теоретические ранги и поле минимального
численного порога; class caps/teachers и золото не моделируются.

## Полный аналитический генератор

```python
from pathlib import Path
import csv, hashlib, json, math, re, statistics, struct
from collections import defaultdict

ROOT = Path.cwd()
OUT = ROOT / 'docs/research'
OUT.mkdir(parents=True, exist_ok=True)
(ROOT / 'docs/design').mkdir(parents=True, exist_ok=True)
TABLE = ROOT / 'assets_dev/engine/data_tables'
def rows(name):
    return [a for line in (TABLE/name).read_text().splitlines()
            if (a := line.split('\t'))[0].isdigit()]
stats, desc, bolster = rows('monster_data.txt'), rows('monster_descriptors.txt'), rows('bolster_monsters.txt')
dmap, bmap = {int(a[0]):a for a in desc}, {int(a[0]):a for a in bolster}
original = Path('/tmp/openyamm-mm8-dmonlist.bin').read_bytes()
count = struct.unpack_from('<I', original)[0]
assert count == 198 and len(original) == 4 + count*184
originals = {}
for i in range(count):
    offset = 4+i*184
    originals[i+1] = (original[offset+20:offset+52].split(b'\0')[0].decode('latin1'),
                       struct.unpack_from('<HHHh', original, offset))
def write_csv(name, data):
    with (OUT/name).open('w', newline='', encoding='utf-8') as f:
        w=csv.DictWriter(f, fieldnames=list(data[0]), lineterminator='\n'); w.writeheader(); w.writerows(data)
def table(headers, data):
    return '\n'.join(['| '+' | '.join(headers)+' |','| '+' | '.join(['---']*len(headers))+' |']+
                     ['| '+' | '.join(str(x) for x in r)+' |' for r in data])+'\n'
giants = {'Dragon','Crystal Dragon','Bone Dragon','Wimpy Dragon','Dragon Turtle','Cyclops',
          'Ogre Warrior','Ogre Mage','Ogre Pesant Male','Troll','Troll Pesant',
          'Earth Elemental','Water Elemental','Juggernaut'}
disputed = {'Crystal Dragon','Wimpy Dragon','Ogre Pesant Male','Ogre Mage','Troll','Troll Pesant',
            'Earth Elemental','Water Elemental','Juggernaut','Minotaur','Minotaur Pesant',
            'Thunder Lizard','Wyvern','Plane Guardian','Wimpy Plane Guardian','Fire Elemental',
            'Air Elemental','Efreeti','Centaur','Gorgon','Nightmare','Thunderbird','Phoenix',
            'Raven Man','Salamander'}
res_names = ['fire','air','water','earth','mind','spirit','body','light','dark','physical']
stat_names = ['monster_id','name','picture','level','full_hp','armor_class','experience','treasure',
              'blood','fly','move','ai_type','hostility','speed','recovery','preferences_raw',
              'special_attack_raw','attack1_type','attack1_damage','attack1_missile','attack2_chance',
              'attack2_type','attack2_damage','attack2_missile','spell1_chance','spell1_raw',
              'spell2_chance','spell2_raw'] + [n+'_resistance_raw' for n in res_names]+['special_column_raw']
desc_names = ['descriptor_id','internal_name','height','radius','movement_speed','to_hit_radius',
              'tint_color','attack_sound_id','death_sound_id','wince_sound_id','aware_sound_id',
              'sprite_standing','sprite_walking','sprite_melee','sprite_ranged','sprite_hit',
              'sprite_dying','sprite_dead','sprite_bored','inspect_y_offset','inspect_attack_chance',
              'inspect_fidget_when_moving']
bolster_names = ['bolster_id','note','creature_type','extra_types','creed','gender','style',
                 'preferred_magic','no_bounty','new_ranged_attacks','new_spells','size_affects_hp',
                 'replicate','new_summons','summon_id','extra_points','max_hp_boost_percent']
flag_tokens = ['undead','dragon','swimmer','immobile','peasant','noarena','ogre','elemental',
               'titan','nocorpse','demon','elf']
all_data=[]
for r in stats:
    id=int(r[0]); d=dmap.get(id, []); b=bmap.get(id, [])
    family=re.sub(r' [ABC]$', '', r[2].strip())
    variant=r[2].strip()[-1] if re.search(r' [ABC]$', r[2].strip()) else ''
    item=dict(zip(stat_names, [v.strip().strip('"') for v in r]))
    item.update({k:(d[j] if j<len(d) else '') for j,k in enumerate(desc_names)})
    item.update({'bolster_'+k:(b[j] if j<len(b) else '') for j,k in enumerate(bolster_names)})
    item['family']=family; item['variant']=variant
    item['scope']='MM8-original-descriptor' if id<=count else 'merged-other-or-custom'
    item['proposed_key']=f'mm8:monster:{id}' if id<=count else f'merged:monster:{id}'
    tokens={re.sub('[^a-z0-9]','',v.lower()) for v in ((b[2:3]+b[3].split(',')) if len(b)>3 else [])}
    item['runtime_kind_flags']='|'.join(t for t in flag_tokens if t in tokens)
    item['runtime_kind_flags_mask']=sum(1<<i for i,t in enumerate(flag_tokens) if t in tokens)
    for n in res_names:
        raw=item[n+'_resistance_raw']; item[n+'_resistance']=200 if raw.lower() in ['imm','immune'] else int(raw or 0)
    item['immune_damage_types']='|'.join(n for n in res_names if item[n+'_resistance']>=200)
    item['category_proposed']= 'GIANT' if (family in giants or re.search('dragon$|hydra|titan|golem|behemoth',family,re.I)) else 'NORMAL'
    item['category_review']='required' if family in disputed or id>count else 'provisional'
    item['classification_basis']='family+descriptor; visual review pending'
    item['original_height']=item['original_radius']=item['original_movement_speed']=item['original_to_hit_radius']=''
    if id<=count:
        name,geo=originals[id]; assert name.lower()==item['internal_name'].lower()
        for k,v in zip(['original_height','original_radius','original_movement_speed','original_to_hit_radius'],geo): item[k]=v
        # Keep original and mounted dimensions separately; merged data may differ.
    all_data.append(item)
assert len({x['monster_id'] for x in all_data})==len(all_data)
write_csv('monster-stats.csv',all_data)
mm8=[x for x in all_data if x['scope']=='MM8-original-descriptor']
assert [int(x['monster_id']) for x in mm8]==list(range(1,199))
families=defaultdict(list)
for x in mm8: families[x['family']].append(x)
def ints(xs,key): return [int(x[key]) for x in xs]
def corr(xs,a,b):
    x,y=ints(xs,a),ints(xs,b); mx,my=statistics.mean(x),statistics.mean(y)
    return sum((i-mx)*(j-my) for i,j in zip(x,y))/math.sqrt(sum((i-mx)**2 for i in x)*sum((j-my)**2 for j in y))
summary={'count_mm8':len(mm8),'count_merged':len(all_data),'families_mm8':len(families),
         'level_min':min(ints(mm8,'level')),'level_max':max(ints(mm8,'level')),
         'hp_min':min(ints(mm8,'full_hp')),'hp_max':max(ints(mm8,'full_hp')),'hp_median':statistics.median(ints(mm8,'full_hp')),
         'giant':sum(x['category_proposed']=='GIANT' for x in mm8),
         'correlations':{a+'_'+b:corr(mm8,a,b) for a,b in [('level','full_hp'),('height','level'),('height','full_hp'),('radius','level'),('radius','full_hp')]},
         'original_sha256':hashlib.sha256(original).hexdigest(),
         'source_hashes':{n:hashlib.sha256((TABLE/n).read_bytes()).hexdigest() for n in ['monster_data.txt','monster_descriptors.txt','bolster_monsters.txt','class_skills.txt','race_skills.yml']}}
print(json.dumps(summary,ensure_ascii=False,indent=2))
family_rows=[]; class_rows=[]
for family,xs in families.items():
    family_rows.append([family,','.join(x['monster_id'] for x in xs),'/'.join(x['level'] for x in xs),
                        '/'.join(x['full_hp'] for x in xs),'/'.join(x['armor_class'] for x in xs),
                        '/'.join(x['physical_resistance_raw'] for x in xs),
                        '/'.join(x['height']+'×'+x['radius'] for x in xs)])
    class_rows.append([family,','.join(x['monster_id']+':'+x['variant'] for x in xs),
                       '/'.join(x['height']+'×'+x['radius'] for x in xs),xs[0]['category_proposed'],
                       'спорно; нужен визуальный обзор' if family in disputed else 'семейство и геометрия; предварительно'])
(Path('/tmp')/'openyamm_family_tables.md').write_text(table(['Семейство','ID A/B/C','Level A/B/C','HP A/B/C','AC A/B/C','Phys A/B/C','Height×Radius A/B/C'],family_rows))
(Path('/tmp')/'openyamm_classification_tables.md').write_text(table(['Семейство','ID:вариант','Height×Radius A/B/C','Категория','Основание'],class_rows))
tops=''
for key in ['level','full_hp']:
    tops+='\n## Топ-20 по '+key+'\n\n'+table(['ID','Имя','Level','HP'],[[x['monster_id'],x['name'],x['level'],x['full_hp']] for x in sorted(mm8,key=lambda x:(-int(x[key]),int(x['monster_id'])))[:20]])
(Path('/tmp')/'openyamm_tops.md').write_text(tops)
matrix=[]
for x in mm8:
    for level in [10,20,30,35,50]:
        for skill in [5,10,15,20]:
            for mastery,mult in [('Normal',1),('Expert',2),('Master',3),('Grandmaster',4)]:
                power=level+mult*skill; difficulty=max(0,int(x['level'])-level)
                matrix.append({'monster_id':x['monster_id'],'name':x['name'],'monster_level':x['level'],
                               'player_level':level,'skill_level':skill,'mastery':mastery,
                               'power':power,'difficulty':difficulty,'success':int(power>=difficulty),
                               'mastery_numeric_requirement_met':int(skill>={'Normal':1,'Expert':4,'Master':7,'Grandmaster':10}[mastery])})
write_csv('identification-matrix.csv',matrix)
thresholds=[]
for level in [10,20,30,35,50]:
    for skill in [5,10,15,20]:
        thresholds.append([level,skill]+[str(2*level+m*skill)+' ('+str(sum(int(x['level'])<=2*level+m*skill for x in mm8))+'/198)' for m in range(1,5)])
(Path('/tmp')/'openyamm_identification_summary.md').write_text(table(['Player level','I','N:предел (доступно)','E','M','GM'],thresholds))
def p0(s,v,i,rank,known=True):
    a,b,c={'Normal':(.30,0,0),'Expert':(.45,.14,.07),'Master':(.55,.16,.08),'Grandmaster':(.65,.18,.09)}[rank]
    return a*s+b*v+c*i*known
def kl(player,monster):
    delta=player-monster
    return min(1.4,1+delta/60) if delta>=0 else 30/(30+1.5*abs(delta))
def chance(s,v,i,rank,pl,ml,giant=False,known=True): return min(15,p0(s,v,i,rank,known)*kl(pl,ml)*(.25 if giant else 1))/100
prob=[]
for s in [5,10,15,20]:
    for v,i in [(0,0),(5,5),(10,10),(15,10),(20,20)]:
        for rank in ['Normal','Expert','Master','Grandmaster']:
            for delta in [-60,-40,-20,0,20,40,60]:
                for giant in [False,True]:
                    prob.append({'S':s,'V':v,'I':i,'rank':rank,'delta':delta,'known':1,'category':'GIANT-below-half' if giant else 'NORMAL',
                                 'P0_percent':round(p0(s,v,i,rank),6),'KL':round(kl(100+delta,100),6),
                                 'P_percent':round(100*chance(s,v,i,rank,100+delta,100,giant),6)})
write_csv('death-strike-probabilities.csv',prob)
def cost(n):return n*(n+1)//2-1
# Levels and raw combat parameters are explicit scenarios, not measured characters.
# Axe, Armsmaster, Death Strike, Perception, Identify Monster; normal Minotaur assumes user-confirmed E Identify.
builds=[(10,5,4,4,4,1,'Normal'),(20,7,7,5,5,4,'Expert'),
        (35,10,10,7,5,5,'Master'),(50,12,10,10,7,7,'Grandmaster')]
build_rows=[]
for pl,a,arm,s,v,i,rank in builds:
    budget=sum(5+new//10 for new in range(2,pl+1)); spent=sum(cost(x) for x in [a,arm,s,v,i])
    build_rows.append([pl,f'{a}/{arm}/{s}/{v}/{i}',rank,budget,spent,budget-spent])
(Path('/tmp')/'openyamm_builds.md').write_text(table(['Level','Axe/Armsmaster/S/V/I','Death rank (условно)','Очки от уровней','Цена пяти навыков','Остаток'],build_rows))
def damage_distribution(raw,res):
    if res>=200:return {0:1.0}
    q=max(0,res)/(max(0,res)+30); result=defaultdict(float)
    for halvings in range(5): result[raw//(2**halvings)]+=q**halvings*(1-q) if halvings<4 else q**4
    return result
def expected_attacks(hp,raw,res,hit,p,giant):
    distribution=damage_distribution(raw,res); e=[0.0]*(hp+1)
    for h in range(1,hp+1):
        denominator=hit; numerator=1.0
        for dmg,w in distribution.items():
            nh=max(0,h-dmg); deathp=p if nh>0 and (not giant or 2*nh<hp) else 0
            if nh==h: denominator-=hit*w*(1-deathp)
            elif nh>0:numerator+=hit*w*(1-deathp)*e[nh]
        e[h]=numerator/denominator if denominator>1e-12 else math.inf
    return e[hp]
combat=[]
for pl,a,arm,s,v,i,rank in builds:
    for x in mm8:
        hp,ac,ml,res=[int(x[k]) for k in ['full_hp','armor_class','level','physical_resistance']]
        # Equal pre-modifier damage benchmark isolates specialty effects; not full item/stat character simulation.
        base={10:20,20:35,35:55,50:75}[pl]
        known=pl+2*i>=max(0,ml-pl); giant=x['category_proposed']=='GIANT'
        for mode in ['Minotaur-one-hand','Minotaur-Lord-two-hand','Minotaur-Lord-one-hand','Knight','Champion','Champion-reinvest']:
            for haste in [False,True]:
                mino=mode.startswith('Minotaur'); gmA=mode=='Minotaur-Lord-two-hand' and a>=10
                gmOne=mode=='Minotaur-Lord-one-hand' and a>=10
                combatArm=arm
                if mode=='Champion-reinvest':
                    budgetFive=sum(cost(t) for t in [a,arm,s,v,i])
                    combatArm=max(n for n in range(1,40) if cost(a)+cost(n)<=budgetFive)
                gmArm=mode.startswith('Champion') and combatArm>=10
                atk=10+2*a+(2*combatArm if gmArm else combatArm)
                hit=max(0,(2*atk+14)/(ac+2*atk+30))
                # Lord GM Axe requires race cap; Knight has M arms; Champion GM arms doubles contribution.
                raw=base+((2*combatArm if gmArm else combatArm) if combatArm>=7 else 0)
                if gmA:raw*=2
                ticks=max(30,(90 if not mino else 100)-a-(2*combatArm if gmArm else combatArm)-5-(25 if haste else 0)-(a if gmOne else 0))
                seconds=ticks/60
                p=chance(s,v,i,rank,pl,ml,giant,known) if mino else 0
                no_knowledge=expected_attacks(hp,raw,res,hit,0,giant)
                # Proposed flat bonus separate from GM Axe multiplication, before resistance, once per attack.
                flat=i if mino and known and i>=4 else 0
                raw_plus=raw+flat
                no_ds=expected_attacks(hp,raw_plus,res,hit,0,giant)
                with_ds=expected_attacks(hp,raw_plus,res,hit,p,giant)
                combat.append({'player_level':pl,'scenario':mode,'haste':int(haste),'monster_id':x['monster_id'],
                               'monster_name':x['name'],'monster_level':ml,'hp':hp,'ac':ac,'physical_resistance':res,
                               'category':x['category_proposed'],'build_skill_cost':sum(cost(t) for t in [a,arm,s,v,i]) if mino else cost(a)+cost(combatArm),
                               'armsmaster_level':combatArm,
                               'known':int(known),'raw_physical_damage':raw,'flat_identify_damage':flat,
                               'attack_bonus_assumed':atk,'hit_probability':round(hit,8),'recovery_ticks':ticks,
                               'recovery_seconds':seconds,'eligible_death_probability':p,
                               'expected_attacks_base':no_knowledge,'expected_attacks_identify':no_ds,'expected_attacks_death':with_ds,
                               'expected_seconds_base':no_knowledge*seconds,'expected_seconds_identify':no_ds*seconds,
                               'expected_seconds_death':with_ds*seconds,'time_reduction_death_percent':100*(1-with_ds/no_ds)})
write_csv('combat-scenarios.csv',combat)
# Extra families belong to the merged table, not native MM8.
extra=[x for x in all_data if int(x['monster_id'])>198 and re.search('hydra|titan|golem',x['family'],re.I)]
(Path('/tmp')/'openyamm_extra.md').write_text(table(['ID','Имя','Семейство','Level','HP','Height×Radius','Категория'],
    [[x['monster_id'],x['name'],x['family'],x['level'],x['full_hp'],x['height']+'×'+x['radius'],x['category_proposed']] for x in extra]))
# Original descriptions derive only from real row values; no imported lore asserted.
lore=[]
for family,xs in families.items():
    immune=sorted({n for x in xs for n in res_names if x[n+'_resistance']>=200})
    low=sorted(res_names,key=lambda n:(sum(x[n+'_resistance'] for x in xs),n))[:2]
    specials=sorted({x['special_attack_raw'] for x in xs}-{ '0','' })
    lore.append([family,'происхождение/место: не подтверждено',
                 ', '.join(sorted({x['ai_type'] for x in xs})),
                 ' / '.join(x['attack1_type']+':'+x['attack1_damage'] for x in xs),
                 ', '.join(specials) or 'нет табличной спецатаки',
                 ', '.join(immune) or 'нет иммунитета ≥200',
                 'сравнить '+', '.join(low)+'; меньшее сопротивление не означает дополнительный урон'])
(Path('/tmp')/'openyamm_lore.md').write_text(table(['Семейство','Лор','AI A/B/C','Атака A/B/C','Особенности','Иммунитеты','Совет по таблице'],lore))
(Path('/tmp')/'openyamm_research_summary.json').write_text(json.dumps(summary,indent=2))
```

## Проверки и открытые вопросы

Локально проверены размеры CSV, уникальность/покрытие ID, имена оригинальных дескрипторов,
формулы вероятности, половина HP для GIANT, детерминированная матрица идентификации,
монотонность эффекта DS в модели, source hashes, относительные ссылки и whitespace diff.
Это проверка аналитических артефактов, не игровые тесты новых навыков.
Документы используют реальные paths и symbol names из
[MonsterTable](../../game/tables/MonsterTable.cpp),
[GameMechanics](../../game/gameplay/GameMechanics.cpp),
[Party](../../game/party/Party.cpp),
[SaveGame](../../game/maps/SaveGame.cpp).
Открытые ограничения: [монстры](monster-stats.md#открытые-вопросы),
[баланс](combat-balance.md#открытые-вопросы-и-обязательная-следующая-проверка).

В существующем `.gitignore` `/docs/*` исключает новые каталоги docs/research и docs/design.
Менять .gitignore в этом этапе запрещено ограничением «только Markdown/CSV». Для включения
новых документов в будущий commit требуется явный `git add -f` выбранных файлов.
Новый бинарник MM8 и временный генератор не добавлять в commit.


## Финальная кампанийная ревизия (2026-10-15)

После Python-блока выше обязательно выполнить `python3 docs/research/final-review.py`.
Это второй аналитический проход: он расширяет identification-matrix.csv до 19800 строк (добавляет I1),
пишет mm8-campaign-occurrences.csv, identification-coverage.csv, monster-correlations.csv и
final-review-inputs.csv. Базовый блок намеренно сохранён для воспроизведения исходных расчётов;
без второго прохода получится прежняя матрица 15840 строк.
Сцены читаются из Git, нужны 61 основной scene blob MM8 в локальном object store.
Недостающие blobs следует предварительно прочитать через git show при разрешённой сети;
сам аналитический проход не должен изменять assets или запускать игру/сборку.
Источник и ограничения выборки184: [final-review](final-review.md).
Проверка воспроизводимости: выполнить оба прохода, сохранить SHA256 всех CSV, повторить
и сравнить SHA256; различий быть не должно. Сравнивать формулу знания с её эквивалентным
порогом `ML <= 2*PL+m*I`; формула речевой реакции `m*I+10` остаётся отдельной.
