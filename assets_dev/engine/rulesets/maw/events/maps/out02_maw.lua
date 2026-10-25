-- MAW content of this map (devdocs/maw/MAPS_CONTENT.md): loaded after the world's events, for MAW games only.

-- The teleporter (§2.2): the pedestal and orb of ../../maps/out02_zz_maw.scene.yml open the dimensional travel
-- destinations, without waiting for the Cross Continents quest (DEVIATIONS 49).
RegisterEvent(1777, "Dimensional Travel", function()
    evt.OpenDimensionDoor(true)
end)
