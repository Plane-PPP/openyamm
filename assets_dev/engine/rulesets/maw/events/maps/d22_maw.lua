-- MAW content of this map (devdocs/maw/MAPS_CONTENT.md): loaded after the world's events, for MAW games only.

-- During a dimension-map run the clerics fight, the secret button shows and door 452 opens, without Dyson Leyland or
-- QBit 28 (§3.3). Outside a run the base events stay, Dyson included (§3.5).
local baseLoadEvent = evt.map[5]
ReplaceMapEvent(5, nil, function()
    if not evt.RulesetInfo().dimensionRun then
        baseLoadEvent()
        return
    end

    evt.SetMonGroupBit(39, MonsterBits.Hostile, 1) -- actor group 39: spawn Cleric (human male) A
    evt.SetMonGroupBit(40, MonsterBits.Hostile, 1) -- actor group 40: spawn Cleric (human male) A
    evt.SetFacetBit(5, FacetBits.IsSecret, 1)
end)
-- Replacing an event drops it from the load list; 5 is d22's load event.
AppendMapOnLoadEvent(5)

local baseDoorEvent = evt.map[452]
ReplaceMapEvent(452, nil, function()
    if not evt.RulesetInfo().dimensionRun then
        baseDoorEvent()
        return
    end

    evt.SetDoorState(1, DoorAction.Open)
end)
