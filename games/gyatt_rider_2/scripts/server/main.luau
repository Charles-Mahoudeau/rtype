-- The server's entry (`scripts.server`): the gameplay. The riders join and move, the traffic comes, the riders shoot,
-- the hits are resolved, the match runs and the score counts.

require("@scripts/server/players")
require("@scripts/server/riders")
require("@scripts/server/traffic")
require("@scripts/server/shots")
require("@scripts/server/shield")
require("@scripts/server/boosts")
require("@scripts/server/hits")
require("@scripts/server/match")
require("@scripts/server/ready")
require("@scripts/server/score")
