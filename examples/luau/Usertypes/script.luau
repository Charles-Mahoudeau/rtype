-- Creating objects of a C++ type from Luau
-- `Player` is a C++ type (see main.cpp). `Player.new` is its constructor.
local player = Player.new("Ada", 100)

-- Members are read and written like fields: `name` is read-only, `health` is read-write.
print(`{player.name} has {player.health} health`)
player.health = 80

-- Methods are called with `:`. `damage` returns the remaining health.
print(`after 30 damage: {player:damage(30)}`)

-- `tostring` and `typeof` come from the C++ type.
print(tostring(player), typeof(player))

-- Two players are equal when the C++ `eq` says so (here, when their names are the same).
print("same player:", player == Player.new("Ada", 1), "other player:", player == Player.new("Bob", 1))

-- A wrong use raises a Lua error, which `pcall` catches.
print("unknown member:", select(2, pcall(function() return player.mana end)))
print("read-only member:", select(2, pcall(function() player.name = "Eve" end)))
print("wrong argument:", select(2, pcall(function() return player:damage("a lot") end)))

-- Members known only at runtime
-- `Settings` has no fixed members: any name can be read once it is written (see `index` and `newIndex` in main.cpp).
local settings = Settings.new()
settings.volume = 0.5
print(`volume is {settings.volume}`)
print("missing setting:", select(2, pcall(function() return settings.brightness end)))

-- Objects created by C++
-- `boss` was made by C++ and stored in a global before this script ran.
print(`the boss is {boss.name}`)

-- C++ calls this function with a `Player` it holds, and reads the player back afterwards.
function heal(target: Player, amount: number)
	target:damage(-amount)
end
