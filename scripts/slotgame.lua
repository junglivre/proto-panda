local input = require("input")
local boop = require("boop")
local menu = require("menu")

local COLOR_WHITE = color565(255, 255, 255)
local COLOR_GREEN = color565(0  , 255, 0)
local COLOR_RED   = color565(255, 0  , 0)
local SPRITE_SIZE = 20
local SCREEN_WIDTH = 64
local SCREEN_HEIGHT = 32
local Y_OFFSET = (SCREEN_HEIGHT - SPRITE_SIZE) / 2 -- Center vertically
local REEL_X = { 0, 22, 44 }

-- Timing tuning
local SPIN_BASE_SPEED = 80  -- ms per symbol advance while spinning at full speed (bigger = slower)
local SPIN_MAX_SPEED = 260  -- ms per symbol advance right before a reel stops 
local DECEL_WINDOW = 650    -- ms before a reel's stop time over which it eases from base -> max speed
local REEL_STOP_TIMES = { 1600, 2300, 3000 } -- when each reel locks in, ms after spin start (staggered)

-- Sound constants
local TONE_SPIN_START = 440   -- A4
local TONE_REEL_STOP = 660   -- E5
local TONE_WIN = 880         -- A5
local TONE_LOSE = 330        -- E4
local TONE_TICK = 220        -- A3

local function clamp(v, lo, hi)
	if v < lo then return lo end
	if v > hi then return hi end
	return v
end

local _M = {
	VERSION_REQUIRED = "3.3.4",
	shouldStop = false,
	
	reel = { 0, 0, 0 },
	reelNext = { 0, 0, 0 },
	reelFinal = { 0, 0, 0 },
	reelLastUpdate = { 0, 0, 0 },
	reelScrollOffset = { 0, 0, 0 },
	reelStopped = { true, true, true },

	spinning = false,
	spinStartTime = 0,
	result = 0,
	showResult = false,
	resultDisplayTime = 0,
	flashCount = 0,
	flashTimer = 0,
	borderColor = COLOR_WHITE,
	
	-- Track which reels have stopped to play sounds only once
	reelStoppedPlayed = { false, false, false },
}


local function drawReel(spr, x, current, nextSym, t)
	if t < SPRITE_SIZE then
		spr:CropSprite(0, 0, SPRITE_SIZE, SPRITE_SIZE - t, false, false)
		spr:SetFrameId(current)
		spr:SetPosition(x, Y_OFFSET + t)
		spr:Draw()
	end
	if t > 0 then
		spr:CropSprite(0, SPRITE_SIZE - t, SPRITE_SIZE, t, false, false)
		spr:SetFrameId(nextSym)
		spr:SetPosition(x, Y_OFFSET)
		spr:Draw()
	end
end

-- Helper function to play tones with error handling
local function playTone(freq, duration)
	if freq and duration then
		pcall(toneDuration, freq, duration)
	end
end

function _M.onSetup()
	setPanelManaged(false)
	math.randomseed(millis())

	if not _M.sprites then
		_M.sprites = Sprite()
		_M.sprites:SetTransparencyColor(0)
		_M.sprites:LoadFromPng("/scripts/sprites/bell.png") -- bell 0
		_M.sprites:LoadFromPng("/scripts/sprites/seven.png") -- seven 1
		_M.sprites:LoadFromPng("/scripts/sprites/cherry.png") -- cherry 2
		_M.sprites:LoadFromPng("/scripts/sprites/coconut.png") -- coconut 3
		_M.sprites:LoadFromPng("/scripts/sprites/bar.png") -- coconut 4
		_M.sprites:LoadFromPng("/scripts/sprites/orange.png") -- coconut 5
		_M.sprites:LoadFromPng("/scripts/sprites/pear.png") -- coconut 6
		_M.maxSprite = _M.sprites:LoadFromPng("/scripts/sprites/watermelon.png") -- watermelon 7
		_M.sprites:CropSprite(0, 0, SPRITE_SIZE, SPRITE_SIZE, false, false)
	end

	for i = 1, 3 do
		_M.reel[i] = math.random(0, _M.maxSprite)
		_M.reelNext[i] = _M.reel[i]
		_M.reelFinal[i] = _M.reel[i]
		_M.reelScrollOffset[i] = 0
		_M.reelStopped[i] = true
		_M.reelStoppedPlayed[i] = false
	end


	_M.spinning = false
	_M.showResult = false
	_M.flashCount = 0
	_M.borderColor = COLOR_WHITE
end

local function pickFinalSymbols(maxSprite, isWin)
	local a, b, c
	if isWin then
		local sym = math.random(0, maxSprite)
		a, b, c = sym, sym, sym
	else
		repeat
			a = math.random(0, maxSprite)
			b = math.random(0, maxSprite)
			c = math.random(0, maxSprite)
		until a ~= b or b ~= c or a ~= c
	end
	return a, b, c
end

function _M.onLoop(dt)
	clearPanelBuffer()
	oledClearScreen()

	local isBooping = false
	local isBooped = false
	if menu.has_boop then
		isBooped, isBooping = boop.isBoopedCheck(dt)
	end


	if not _M.spinning and (isBooping or input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED) then
		_M.spinning = true
		_M.spinStartTime = millis()
		_M.showResult = false
		_M.flashCount = 0
		_M.borderColor = COLOR_WHITE
		
		-- Reset the played flags
		for i = 1, 3 do
			_M.reelStoppedPlayed[i] = false
		end

		_M.result = (math.random(0, 1000) <= 200) and 1 or 0
		local f1, f2, f3 = pickFinalSymbols(_M.maxSprite, _M.result == 1)
		_M.reelFinal[1], _M.reelFinal[2], _M.reelFinal[3] = f1, f2, f3

		for i = 1, 3 do
			_M.reelStopped[i] = false
			_M.reelLastUpdate[i] = _M.spinStartTime
			_M.reelNext[i] = math.random(0, _M.maxSprite)
			_M.reelScrollOffset[i] = 0
		end
		
		-- Play spin start sound
		playTone(TONE_SPIN_START, 80)
	end

	--spiwiwiwiwiwiwiwiwi
	if _M.spinning then
		local currentTime = millis()
		local allStopped = true

		for i = 1, 3 do
			if not _M.reelStopped[i] then
				local elapsedSinceStart = currentTime - _M.spinStartTime
				local timeToStop = REEL_STOP_TIMES[i] - elapsedSinceStart

				if timeToStop <= 0 then
					_M.reel[i] = _M.reelFinal[i]
					_M.reelNext[i] = _M.reelFinal[i]
					_M.reelScrollOffset[i] = 0
					_M.reelStopped[i] = true
					
					-- Play stop sound for each reel
					if not _M.reelStoppedPlayed[i] then
						_M.reelStoppedPlayed[i] = true
						playTone(TONE_REEL_STOP, 60)
					end
				else
					allStopped = false

					local speedSlowing = timeToStop <= DECEL_WINDOW
					local speed = SPIN_BASE_SPEED
					if speedSlowing then
						local frac = timeToStop / DECEL_WINDOW -- 1 -> 0 as we approach the stop
						local eased = (1 - frac) * (1 - frac)   -- ease toward slow
						speed = SPIN_BASE_SPEED + (SPIN_MAX_SPEED - SPIN_BASE_SPEED) * eased
					end

					local tickElapsed = currentTime - _M.reelLastUpdate[i]
					if tickElapsed >= speed then
						_M.reelLastUpdate[i] = _M.reelLastUpdate[i] + speed
						tickElapsed = currentTime - _M.reelLastUpdate[i]

						_M.reel[i] = _M.reelNext[i]

						if speedSlowing then
							_M.reelNext[i] = _M.reelFinal[i]
						else
							_M.reelNext[i] = math.random(0, _M.maxSprite)
						end
						
						-- Play tick sound during spin (but not too often)
						if not speedSlowing and math.random(1, 3) == 1 then
							playTone(TONE_TICK, 20)
						end
					end

					_M.reelScrollOffset[i] = clamp(math.floor((tickElapsed / speed) * SPRITE_SIZE), 0, SPRITE_SIZE)
				end
			end
		end

		if allStopped then
			_M.spinning = false
			_M.showResult = true
			_M.resultDisplayTime = millis()
			_M.flashCount = 0
			_M.flashTimer = millis()

			if _M.result == 1 then
				_M.borderColor = COLOR_GREEN
				-- Play win sound (ascending tone)
				playTone(TONE_WIN, 200)
				-- Add a second tone for extra celebration
				playTone(TONE_WIN * 1.25, 150)
			else
				_M.borderColor = COLOR_RED
				-- Play lose sound
				playTone(TONE_LOSE, 300)
			end
		end
	end

	if _M.showResult then
		local currentTime = millis()
		if currentTime - _M.flashTimer >= 150 then
			_M.flashTimer = currentTime
			_M.flashCount = _M.flashCount + 1
			if _M.flashCount % 2 == 0 then
				if _M.result == 1 then
					_M.borderColor = COLOR_GREEN
				else
					_M.borderColor = COLOR_RED
				end
			else
				_M.borderColor = COLOR_WHITE
			end

			if _M.flashCount >= 10 then 
				_M.showResult = false
				_M.borderColor = COLOR_WHITE
			end
		end
	end

	local spr = _M.sprites
	for i = 1, 3 do
		local t = (_M.spinning and not _M.reelStopped[i]) and _M.reelScrollOffset[i] or 0
		drawReel(spr, REEL_X[i], _M.reel[i], _M.reelNext[i], t)
	end

	drawPanelRect(0, 0, SCREEN_WIDTH , SCREEN_HEIGHT , _M.borderColor)
	drawPanelRect(64, 0, SCREEN_WIDTH , SCREEN_HEIGHT , _M.borderColor) --Draw on the other side

	flipPanelBuffer()
	oledFaceToScreen(0, 0, 2)
	oledDisplay()

	-- Exit condition
	if input.readButtonStatus(BUTTON_BACK) == BUTTON_JUST_PRESSED or
	   (input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_PRESSED and
	    input.readButtonStatus(BUTTON_LEFT) == BUTTON_PRESSED and
	    input.readButtonStatus(BUTTON_RIGHT) == BUTTON_PRESSED and
	    input.readButtonStatus(BUTTON_UP) == BUTTON_PRESSED and
	    input.readButtonStatus(BUTTON_DOWN) == BUTTON_PRESSED) then
		_M.shouldStop = true
		return true
	end
end

function _M.onClose()
	collectgarbage()
end

return _M