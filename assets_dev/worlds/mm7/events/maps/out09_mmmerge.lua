-- MMMerge supplement: Evenmorn obelisk treasure and Dimension Door trigger.

AppendMapEvent(6, function()
    MM7.OpenDimensionDoor()
end)

RegisterMapOnLoadEvent(65009, "MMMerge Evenmorn obelisk treasure", function()
    MM7.UpdateEvenmornObeliskTreasure()
end)

-- Every half hour, as MMerge's Timer(SetTrViz, const.Hour/2).
RegisterMapTimerEvent(65011, 1800, function()
    MM7.UpdateEvenmornObeliskTreasure()
end, "MMMerge Evenmorn obelisk treasure clock")

RegisterMapTimerEvent(65010, 180, function()
    MM7.OpenEvenmornDimensionDoorIfNear()
end, "MMMerge Evenmorn Dimension Door proximity")
