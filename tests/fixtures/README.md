# Minotaur legacy save fixture

`minotaur-v84.oysav` is a synthetic save written by the production v84 serializer
(before the v85 change; serializer source matches commit69bed40). It contains a Minotaur Lord
with Perception and no LethalStrike entry. It contains no user save or personal data.

LethalStrikeTests loads it through the v85 reader, hydrates Party, and verifies that the innate
Normal1 skill is added without losing Perception. Separate tests cover preserved existing progress,
repeated hydration and persistence of the outdoor combat-level cache in v85.
