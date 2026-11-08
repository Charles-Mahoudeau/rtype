-- The server's entry (`scripts.server`): the modules that run the match. A module that several of them require runs once.

require("@scripts/server/obstacles")
require("@scripts/server/players")
require("@scripts/server/ships")
require("@scripts/server/lasers")
