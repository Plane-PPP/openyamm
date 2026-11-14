#!/bin/bash
# The test selections of the Minotaur Windows workflow (.github/workflows/minotaur-windows.yml), kept in one place so
# the same filters run in CI and on a development machine before a push:
#
#   tools/minotaur_ci_tests.sh build/tests                 # every group, as CI runs them
#   tools/minotaur_ci_tests.sh build/tests roguelike mm9   # the named groups
#
# MEASURE, when set, is the command each group runs under (CI: tools/measure_windows_build.py --phase); the group's
# name is appended to it as the phase. The test programs are <tests>/openyamm_unit_tests[.exe] and
# openyamm_runtime_tests[.exe]. Exit status: the first failing group's.
set -u

tests="${1:?usage: minotaur_ci_tests.sh <tests directory> [group ...]}"
shift
groups=("$@")
if [ ${#groups[@]} -eq 0 ]; then
    groups=(gameplay world medusa-importer updater upstream-ai upstream-indoor roguelike mm9 mm9-ai)
fi

suffix=""
[ -f "$tests/openyamm_unit_tests.exe" ] && suffix=".exe"
unit="$tests/openyamm_unit_tests$suffix"
runtime="$tests/openyamm_runtime_tests$suffix"

run() {
    local phase="$1"
    shift
    echo "== $phase"
    if [ -n "${MEASURE:-}" ]; then
        # shellcheck disable=SC2086
        $MEASURE "$phase" -- "$@"
    else
        "$@"
    fi
}

status=0
for group in "${groups[@]}"; do
    case "$group" in
        gameplay)
            # MM9's and Chedian's cases are the mm9 group; Bloodaxe is not an Axe mechanic test.
            run gameplay-tests "$unit" --test-case='*alchemy*,*resistance*,*lethal*,*minotaur*,*axe*,*levitate*,*lava*,Reagent stacks*,Inventory hotfix*,Bottle stacks*,Inspect *,*potion*,*bag*,technology skills are appended*,*summary*,*summaries*,*history*,item search*,*steal*,*creation*,*quick item*,*party item*,*teacher*,*trainer*,*completion*,*basebar*,*carried*,*world completion*,resistance and monster effect*,*lich immunit*,*Lich*immun*,*condition immun*,undead and vampires*' \
                --test-case-exclude='MM9*,*MM9 RUDE*,*Chedian*,*Bloodaxe*,MM8 lava terrain is drawn by the liquid shader' ;;
        world)
            run world-tests "$runtime" --test-case='Lethal Strike*,Inventory hotfix*,*steal*' ;;
        medusa-importer)
            # Two upstream cases need a case-sensitive file system (files differing only in case side by side);
            # upstream runs its unit tests on Linux and macOS only, and NTFS cannot hold them.
            run medusa-importer-tests "$unit" --source-file='*ProjectMedusaTests.cpp,*CustomPortraitTests.cpp,*AssetFileSystemTests.cpp' \
                --test-case-exclude='AssetFileSystem reads aliased package files case-insensitively,AssetFileSystem rejects ambiguous case fallback while preserving exact names' ;;
        updater)
            run updater-engine-tests "$unit" --source-file='*UpdaterTests.cpp' ;;
        upstream-ai)
            run upstream-ai-tests "$unit" --source-file='*GameplayActorAiSystemTests.cpp,*ActorPathRuntimeTests.cpp,*GameplayBolsterRuntimeTests.cpp,*CharacterSheetRegressionTests.cpp' \
                --test-case-exclude='*MM9*' ;;
        upstream-indoor)
            run upstream-indoor-tests "$runtime" --source-file='*IndoorActorFlyingRuntimeTests.cpp,*IndoorActorDetectionRuntimeTests.cpp,*IndoorActorFactionRuntimeTests.cpp' ;;
        roguelike)
            run roguelike-tests "$unit" --source-file='*DungeonRunTests.cpp,*RoguelikeRulesTests.cpp' ;;
        mm9)
            # Needs assets_dev/worlds/mm9 (the full 2.3 release ships it).
            run mm9-tests "$unit" --test-case='MM9*,*MM9 RUDE*,*Chedian*' ;;
        mm9-ai)
            run mm9-ai-tests "$unit" --source-file='*GameplayActorAiSystemTests.cpp,*ActorPathRuntimeTests.cpp,*GameplayBolsterRuntimeTests.cpp,*CharacterSheetRegressionTests.cpp' \
                --test-case='*MM9*' ;;
        *)
            echo "Unknown test group: $group" >&2
            exit 2 ;;
    esac
    result=$?
    if [ $result -ne 0 ]; then
        echo "== $group failed with $result" >&2
        [ $status -eq 0 ] && status=$result
    fi
done
exit $status
