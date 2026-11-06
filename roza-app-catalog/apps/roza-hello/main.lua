-- Hello ROZA : premier exemple executif du runtime embarque Lua 5.4.
local seconds = 0
local last_tick = 0
local launches = 0

function setup()
  launches = launches + 1
end

function loop()
  local now = roza.millis()
  if now - last_tick >= 1000 then
    seconds = seconds + 1
    last_tick = now
  end
end

function draw()
  roza.clear(0x0000)
  roza.text("HELLO ROZA", 8, 8, 0x07E0)
  roza.text("Application Lua active", 8, 26, 0xFFFF)
  roza.rect(4, 45, 232, 43, 0x07E0)
  roza.text("Temps : " .. tostring(seconds) .. " s", 12, 55, 0xFFFF)
  roza.text("Lancer : OK / Entree", 8, 96, 0xFFE0)
  roza.text("Retour : touche retour", 8, 114, 0x7BEF)
end

function on_key(key)
  if key == "ENTER" then
    seconds = 0
    last_tick = roza.millis()
  end
end
