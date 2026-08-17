local DEFAULTS = {
	noise_threshold = 4000,
	band_start = 2,
	band_end = 8,
	min_energy = 60000,
	max_energy = 200000,
	frist_frame_threshold = 60000,
}

local _M = {

	noise_threshold = DEFAULTS.noise_threshold,
	band_start = DEFAULTS.band_start,
    band_end = DEFAULTS.band_end,
    min_energy = DEFAULTS.min_energy,
    max_energy = DEFAULTS.max_energy,
    frist_frame_threshold = DEFAULTS.frist_frame_threshold,


    --Speech level:
    smoothed = 0,
    level = 0,

}

local generic = require("generic")
local Map = generic.map


local CALIB_STATE_MENU          = 0
local CALIB_STATE_NOISE         = 1
local CALIB_STATE_TALK          = 2
local CALIB_STATE_REVIEW        = 3
local CALIB_STATE_TEST          = 4
local CALIB_STATE_ADJ_FRIST     = 5
local CALIB_STATE_ADJ_NOISE     = 6
local CALIB_STATE_ADJ_MIN       = 7
local CALIB_STATE_ADJ_MAX       = 8
local CALIB_STATE_ADJ_BANDSTART = 9
local CALIB_STATE_ADJ_BANDEND   = 10


local ENVELOPE_ATTACK_ALPHA  = 0.5
local ENVELOPE_RELEASE_ALPHA = 0.02

-- How far above a band's own noise floor it must sit (envelope-wise)
-- before that band is considered "active speech" this frame.
local NOISE_ACTIVE_MULT   = 1.8
local NOISE_ACTIVE_MARGIN = 500

-- How fast the detected [bandStart, bandEnd] range is allowed to drift.
-- Lower = more resistant to a stray/transient noisy bin, but slower to
-- track a genuine shift in the speaker's voice.
local BAND_ADAPT_ALPHA = 0.15

-- Overall energy (summed across the current band range) must exceed
-- this multiple of the noise energy in that same range before we
-- consider the user to be "speaking" (used to gate min/max tracking).
local ACTIVE_ENERGY_MULT = 2.0

-- Step sizes used by the quick "adjust a single value" screens.
local NOISE_STEP  = 500
local ENERGY_STEP = 5000
local FRIST_STEP = 500


local SCREEN_WIDTH = 128

local function bandX(i, count)
	return math.floor((i - 1) * (SCREEN_WIDTH / count))
end

local function bandBarWidth(count)
	local step = SCREEN_WIDTH / count
	local w = math.floor(step) - 1
	if w < 1 then w = 1 end
	return w
end

function _M.resetDefaults()



	_M.noise_threshold       = DEFAULTS.noise_threshold
	_M.band_start            = DEFAULTS.band_start
	_M.band_end              = DEFAULTS.band_end
	_M.min_energy            = DEFAULTS.min_energy
	_M.max_energy            = DEFAULTS.max_energy
	_M.frist_frame_threshold = DEFAULTS.frist_frame_threshold

	local cfg = configloader.Get()
	if not cfg.fft or not cfg.fft.enabled then
		_M.noise_threshold 			= cfg.noise_threshold or _M.noise_threshold
		_M.band_start 				= cfg.speech_band_start or _M.band_start
		_M.band_end 				= cfg.speech_band_end or _M.band_end
		_M.min_energy 				= cfg.speech_min_energy or _M.min_energy
		_M.band_end 				= cfg.speech_max_energy or _M.max_energy
		_M.frist_frame_threshold 	= cfg.speech_frist_frame_threshold or _M.frist_frame_threshold
	end

	dictSet("fft_noise_threshold", tostring(_M.noise_threshold))
	dictSet("fft_speech_band_start", tostring(_M.band_start))
	dictSet("fft_speech_band_end", tostring(_M.band_end))
	dictSet("fft_speech_min_energy", tostring(_M.min_energy))
	dictSet("fft_speech_max_energy", tostring(_M.max_energy))
	dictSet("fft_speech_frist_frame_threshold", tostring(_M.frist_frame_threshold))
	dictSave()

	setNoiseThreshold(_M.noise_threshold)

	-- Reset speech-level smoothing so any in-progress reading starts clean.
	_M.smoothed = 0
	_M.level = 0
end

function _M.loadCalibration()
	_M.noise_threshold 			= tonumber(dictGet("fft_noise_threshold")) or _M.noise_threshold 
	_M.band_start 				= tonumber(dictGet("fft_speech_band_start")) or _M.band_start
	_M.band_end 				= tonumber(dictGet("fft_speech_band_end")) or _M.band_end
	_M.min_energy 				= tonumber(dictGet("fft_speech_min_energy")) or _M.min_energy
	_M.max_energy 				= tonumber(dictGet("fft_speech_max_energy")) or _M.max_energy
	_M.frist_frame_threshold 	= tonumber(dictGet("fft_speech_frist_frame_threshold")) or _M.frist_frame_threshold
	
	-- Ensure frist_frame_threshold is not less than min_energy
	if _M.frist_frame_threshold < _M.min_energy then
		_M.min_energy = _M.frist_frame_threshold
	end
	log("Mic calibration loaded")
	log("fft_noise_threshold: ".._M.noise_threshold)
	log("fft_speech_band_start: ".._M.band_start)
	log("fft_speech_band_end: ".._M.band_end)
	log("fft_speech_min_energy: ".._M.min_energy)
	log("fft_speech_max_energy: ".._M.max_energy)
	log("fft_speech_frist_frame_threshold: ".._M.frist_frame_threshold)
	setNoiseThreshold(_M.noise_threshold)
end

function _M.getSpeechLevel(dt, attack, release, maxLevels)
	local energy = 0
	_M.band_end = math.min(_M.band_count, _M.band_end)
    for b = _M.band_start, _M.band_end do
        energy = energy + getBandValueFft(b)
    end


    local tau = (energy > _M.smoothed) and (attack or 0.05) or (release or 0.2)
    local alpha = 1 - math.exp(-dt / tau)
    _M.smoothed = _M.smoothed * (1 - alpha) + energy * alpha

    if _M.smoothed > _M.frist_frame_threshold then
        local norm = (_M.smoothed - _M.min_energy) / (_M.max_energy - _M.min_energy)
        if norm < 0 then norm = 0 end
        if norm > 1 then norm = 1 end
        local level = math.floor(norm * maxLevels)
        if level >= maxLevels then
        	level = maxLevels-1
       	end
        if level < 1 then 
        	level = 1 
       	end
       	_M.level = level
    else
    	_M.level = 0
    end
    return _M.level
end



local function anyDirectionPressed()
	return input.readButtonStatus(BUTTON_UP) == BUTTON_PRESSED
		or input.readButtonStatus(BUTTON_DOWN) == BUTTON_PRESSED
		or input.readButtonStatus(BUTTON_LEFT) == BUTTON_PRESSED
		or input.readButtonStatus(BUTTON_RIGHT) == BUTTON_PRESSED
end

-- Peak follower: snaps up fast, decays slowly. Good for tracking a
-- recent "ceiling" (max energy / per-band envelope).
local function updateEnvelope(prev, v)
	local alpha = (v > prev) and ENVELOPE_ATTACK_ALPHA or ENVELOPE_RELEASE_ALPHA
	return prev * (1 - alpha) + v * alpha
end

-- Floor follower: snaps down fast, rises slowly. Good for tracking a
-- recent "floor" (min sustained energy) without being permanently
-- dragged down by a single quiet frame.
local function updateFloor(prev, v)
	local alpha = (v < prev) and ENVELOPE_ATTACK_ALPHA or ENVELOPE_RELEASE_ALPHA
	return prev * (1 - alpha) + v * alpha
end

local INSTRUCTIONS = {
	[CALIB_STATE_NOISE] = {
		title = "STEP 1/2 - NOISE",
		lines = { "Stay silent.", "Let the mic read", "background noise." },
	},
	[CALIB_STATE_TALK] = {
		title = "STEP 2/2 - TALK",
		lines = { "Say: ", "ba ba ba ba ba", "at normal volume" },
	},
}

local function drawInstructions(info)
	oledSetCursor(0, 0)
	oledDrawText(info.title)

	local y = 16
	for _, line in ipairs(info.lines) do
		oledSetCursor(0, y)
		oledDrawText(line)
		y = y + 10
	end

	oledSetCursor(0, 54)
	oledDrawText("Press CONFIRM")
end

-- Shared "N boxes, filled up to `level`" renderer used by the live
-- speech-level test screen and by the quick min/max/band adjust screens.
local function drawLevelBoxes(level, maxLevels, boxSize, y)
	maxLevels = maxLevels or 5
	boxSize = boxSize or 22
	y = y or 6

	local gap = math.floor(boxSize / 4)
	if gap < 2 then gap = 2 end

	local totalWidth = boxSize * maxLevels + gap * (maxLevels - 1)
	local startX = math.floor((128 - totalWidth) / 2)

	for i = 0, maxLevels - 1 do
		local x = startX + i * (boxSize + gap)
		local isOn = level >= i

		oledDrawRect(x, y, boxSize, boxSize, 1)
		if isOn then
			oledDrawFilledRect(x + 3, y + 3, boxSize - 6, boxSize - 6, 1)
		end

		local label = tostring(i)
		local textColor = isOn and 0 or 1
		oledSetCursor(x + math.floor(boxSize / 2) - 3, y + math.floor(boxSize / 2) - 4)
		oledSetTextColor(textColor)
		oledDrawText(label)
	end
	oledSetTextColor(1)
end

-- ---------------------------------------------------------------------
-- Calibration mode entry points (one per menu option)
-- ---------------------------------------------------------------------

local function startFullCalibration()
	setNoiseThreshold(1)

	_M.maxBandInput = 100
	_M.state = CALIB_STATE_NOISE

	_M.avgNoise = 0
	_M.noiseBandValues = {}

	-- Adaptive band-range tracking (float "smoothed" positions + the
	-- rounded integer range actually used for energy sums).
	_M.bandStartSmooth = nil
	_M.bandEndSmooth = nil
	_M.bandStart = nil
	_M.bandEnd = nil
	_M.talkEnvelope = {}

	-- Live energy tracking while the user talks.
	_M.sumEnergyEnvelope = 0
	_M.floorEnergy = nil
	_M.ceilEnergy = nil
	_M.isSpeaking = false

	_M.result = nil
	_M.showingInstructions = true
end

local function startAdjustFrist()
	_M.state = CALIB_STATE_ADJ_FRIST
	_M.showingInstructions = false
	_M.smoothed = 0
end

local function startAdjustNoise()
	_M.state = CALIB_STATE_ADJ_NOISE
	_M.showingInstructions = false
	_M.maxBandInput = math.max(_M.noise_threshold * 2, 100)
end

local function startAdjustMin()
	_M.state = CALIB_STATE_ADJ_MIN
	_M.showingInstructions = false
	_M.smoothed = 0
end

local function startAdjustMax()
	_M.state = CALIB_STATE_ADJ_MAX
	_M.showingInstructions = false
	_M.smoothed = 0
end

local function startAdjustBandEnd()
	_M.state = CALIB_STATE_ADJ_BANDEND
	_M.showingInstructions = false
	_M.smoothed = 0
end

local function startAdjustBandStart()
	_M.state = CALIB_STATE_ADJ_BANDSTART
	_M.showingInstructions = false
	_M.smoothed = 0
end

-- ---------------------------------------------------------------------
-- Pre-calibration menu (built with the shared ui lib, same widget used
-- by the settings/scripts menus)
-- ---------------------------------------------------------------------

-- The ONLY place _M.quit is set. Reachable either by selecting "Exit"
-- from the menu, or via LEFT/BACK on the menu (the ui lib's built-in
-- "back" handling, wired up below as this menu's onQuit).
local function exitCalibration()
	_M.quit = true
	_M.calibrating = false
end

-- Switches into the menu state and resets its selection/scroll position.
local function goToMenu()
	_M.state = CALIB_STATE_MENU
	_M.menu.onEnter()
end

function _M.onEnter()
	_M.quit = false
	_M.showingInstructions = false
	goToMenu()
	return true
end

function _M.CalibrateDraw(dt)
	_M.calibrating = false
	oledClearScreen()

	if _M.state == CALIB_STATE_MENU then
		_M.menu.draw()
		return
	end

	if _M.showingInstructions and INSTRUCTIONS[_M.state] then
		drawInstructions(INSTRUCTIONS[_M.state])
		oledDisplay()
		return
	end

	if _M.state == CALIB_STATE_NOISE then
		local count = getBandCountFft()
		local avg = 0
		local barWidth = bandBarWidth(count)
		for i = 1, count do
			local v = getBandValueFft(i)
			_M.maxBandInput = math.max(v, _M.maxBandInput)
			avg = avg + v

			_M.noiseBandValues[i] = updateEnvelope(_M.noiseBandValues[i] or 0, v)

			local mapped = Map(v, 0, _M.maxBandInput * 1.1, 0, 64)
			oledDrawFilledRect(bandX(i, count), 0, barWidth, math.ceil(mapped), 1)
		end
		avg = avg / count

		_M.avgNoise = _M.avgNoise * 0.8 + avg * 0.2
		oledDrawFastHLine(0, Map(_M.avgNoise * 1.1, 0, _M.maxBandInput * 1.1, 0, 64), 128, 1)
		oledSetCursor(0, 40)
		oledDrawText("Stay quiet...")
		oledSetCursor(0, 49)
		oledDrawText("Noise: " .. (_M.avgNoise * 1.1))

	elseif _M.state == CALIB_STATE_TALK then
		local count = getBandCountFft()
		local barWidth = bandBarWidth(count)
		local bandValues = {}
		local bandActive = {}

		for i = 1, count do
			local v = getBandValueFft(i)
			bandValues[i] = v
			_M.maxBandInput = math.max(v, _M.maxBandInput)
			_M.talkEnvelope[i] = updateEnvelope(_M.talkEnvelope[i] or 0, v)

			local mapped = Map(v, 0, _M.maxBandInput * 1.1, 0, 64)
			oledDrawFilledRect(bandX(i, count), 0, barWidth, math.ceil(mapped), 1)

			local bandThreshold = (_M.noiseBandValues[i] or 0) * NOISE_ACTIVE_MULT + NOISE_ACTIVE_MARGIN
			bandActive[i] = _M.talkEnvelope[i] > bandThreshold
		end

		-- Find the largest contiguous run of currently-active bands.
		-- Using "largest contiguous run" (rather than "any active band")
		-- means a lone stray bin (e.g. a burst of high-frequency noise)
		-- doesn't get treated as part of the speech band.
		local bestStart, bestEnd, bestLen = nil, nil, 0
		local curStart = nil
		for i = 1, count do
			if bandActive[i] then
				if not curStart then curStart = i end
				local len = i - curStart + 1
				if len > bestLen then
					bestLen = len
					bestStart = curStart
					bestEnd = i
				end
			else
				curStart = nil
			end
		end

		if bestStart then
			_M.bandStartSmooth = _M.bandStartSmooth
				and (_M.bandStartSmooth + BAND_ADAPT_ALPHA * (bestStart - _M.bandStartSmooth))
				or bestStart
			_M.bandEndSmooth = _M.bandEndSmooth
				and (_M.bandEndSmooth + BAND_ADAPT_ALPHA * (bestEnd - _M.bandEndSmooth))
				or bestEnd

			_M.bandStart = math.floor(_M.bandStartSmooth + 0.5)
			_M.bandEnd = math.floor(_M.bandEndSmooth + 0.5)
		end

		if _M.bandStart then
			oledDrawFastVLine(bandX(_M.bandStart, count), 0, 64, 1)
			oledDrawFastVLine(bandX(_M.bandEnd, count), 0, 64, 1)

			local sumEnergy = 0
			local noiseEnergy = 0
			for i = _M.bandStart, _M.bandEnd do
				sumEnergy = sumEnergy + (bandValues[i] or 0)
				noiseEnergy = noiseEnergy + (_M.noiseBandValues[i] or 0)
			end

			-- Sticky peak-follower used ONLY as the "are they still
			-- talking" gate. It stays high through the brief natural
			-- pauses between syllables so the gate doesn't flap open
			-- and shut on every "ba" -- but precisely BECAUSE it's
			-- sticky/peaky, it must never be fed into the floor
			-- tracker below (a floor-of-a-peak-signal just hugs the
			-- ceiling, which is why min/max were nearly equal).
			_M.sumEnergyEnvelope = updateEnvelope(_M.sumEnergyEnvelope, sumEnergy)

			local speakThreshold = noiseEnergy * ACTIVE_ENERGY_MULT
			_M.isSpeaking = _M.sumEnergyEnvelope > speakThreshold

			if _M.isSpeaking then
				-- Feed the RAW per-frame energy (not the sticky
				-- envelope) into the floor/ceiling trackers, so they
				-- actually see the true quiet dips and loud peaks
				-- that occur while the user is talking.
				_M.ceilEnergy = _M.ceilEnergy
					and updateEnvelope(_M.ceilEnergy, sumEnergy)
					or sumEnergy
				_M.floorEnergy = _M.floorEnergy
					and updateFloor(_M.floorEnergy, sumEnergy)
					or sumEnergy
			end
		end

		oledSetCursor(0, 40)
		oledDrawText("Bands: " .. (_M.bandStart or "-") .. " | " .. (_M.bandEnd or "-"))
		oledSetCursor(0, 49)
		oledDrawText("min:" .. math.floor(_M.floorEnergy or 0) .. " max:" .. math.floor(_M.ceilEnergy or 0))

	elseif _M.state == CALIB_STATE_REVIEW then
		local r = _M.result
		oledSetCursor(0, 0)
		oledDrawText("band_start: " .. r.band_start)
		oledSetCursor(0, 12)
		oledDrawText("band_end: " .. r.band_end)
		oledSetCursor(0, 22)
		oledDrawText("threshold: " .. math.floor(r.frist_frame_threshold))
		oledSetCursor(0, 32)
		oledDrawText("min: " .. math.floor(r.min_energy) )
		oledSetCursor(0, 42)
		oledDrawText("max: " .. math.floor(r.max_energy))
		oledSetCursor(0, 54)
		oledDrawText("CONFIRM to test")

	elseif _M.state == CALIB_STATE_TEST then
		_M.calibrating = true
		_M.testLevel = _M.getSpeechLevel(dt, nil, nil, 5)
		drawLevelBoxes(_M.testLevel, 5, 22, 6)

		oledSetCursor(0, 44)
		oledDrawText("Talk to test level")
		oledSetCursor(0, 54)
		oledDrawText("CONFIRM=save DIR=redo")

	elseif _M.state == CALIB_STATE_ADJ_FRIST then
		local count = getBandCountFft()
		local barWidth = bandBarWidth(count)
		
		-- Use max_energy as the max for the Map function
		local maxVal = _M.max_energy * 1.2
		
		for i = 1, count do
			local v = getBandValueFft(i)
			local mapped = Map(v, 0, maxVal, 0, 28)
			oledDrawFilledRect(bandX(i, count), 0, barWidth, math.ceil(mapped), 1)
		end
		
		-- Show the current frame threshold as a horizontal line
		local thresholdY = Map(_M.frist_frame_threshold, 0, maxVal, 0, 28)
		oledDrawFastHLine(0, thresholdY, 128, 1)
		_M.calibrating = true
		local level = _M.getSpeechLevel(dt, nil, nil, 5)
		drawLevelBoxes(level, 5, 12, 30)

		oledDrawFastVLine(bandX(_M.band_start, count), 0, 28, 1)
		oledDrawFastVLine(bandX(_M.band_end, count), 0, 28, 1)

		oledSetCursor(0, 44)
		oledDrawText("Frame thresh: " .. math.floor(_M.frist_frame_threshold))
		oledSetCursor(0, 54)
		oledDrawText("UP/DN +-500 CONFIRM=save")

	elseif _M.state == CALIB_STATE_ADJ_NOISE then
		local count = getBandCountFft()
		local barWidth = bandBarWidth(count)
		
		-- Use max_energy as the max for the Map function
		local maxVal = _M.max_energy * 1.2
		
		for i = 1, count do
			local v = getBandValueFft(i)
			local mapped = Map(v, 0, maxVal, 0, 64)
			oledDrawFilledRect(bandX(i, count), 0, barWidth, math.ceil(mapped), 1)
		end
		
		local thresholdY = Map(_M.noise_threshold, 0, maxVal, 0, 64)
		oledDrawFastHLine(0, thresholdY, 128, 1)

		oledSetCursor(0, 40)
		oledDrawText("Noise: " .. math.floor(_M.noise_threshold))
		oledSetCursor(0, 49)
		oledDrawText("UP/DN +-500 CONFIRM")

	elseif _M.state == CALIB_STATE_ADJ_MIN then
		local count = getBandCountFft()
		local barWidth = bandBarWidth(count)
		
		-- Use max_energy as the max for the Map function
		local maxVal = _M.max_energy * 1.2
		
		for i = 1, count do
			local v = getBandValueFft(i)
			local mapped = Map(v, 0, maxVal, 0, 28)
			oledDrawFilledRect(bandX(i, count), 0, barWidth, math.ceil(mapped), 1)
		end

		oledDrawFastVLine(bandX(_M.band_start, count), 0, 28, 1)
		oledDrawFastVLine(bandX(_M.band_end, count), 0, 28, 1)
		
		-- Show min_energy as a horizontal line
		local minY = Map(_M.min_energy, 0, maxVal, 0, 28)
		oledDrawFastHLine(0, minY, 128, 1)
		
		-- Show frist_frame_threshold as another horizontal line if it's different
		if _M.frist_frame_threshold ~= _M.min_energy then
			local fristY = Map(_M.frist_frame_threshold, 0, maxVal, 0, 28)
			oledDrawFastHLine(0, fristY, 128, 0) -- Draw inverted line for threshold
		end
		_M.calibrating = true
		local level = _M.getSpeechLevel(dt, nil, nil, 5)
		drawLevelBoxes(level, 5, 12, 30)

		oledSetCursor(0, 44)
		oledDrawText("Min energy: " .. math.floor(_M.min_energy))
		oledSetCursor(0, 54)
		oledDrawText("UP/DN +-5000 CONFIRM=save")

	elseif _M.state == CALIB_STATE_ADJ_MAX then
		local count = getBandCountFft()
		local barWidth = bandBarWidth(count)
		
		-- Use max_energy as the max for the Map function
		local maxVal = _M.max_energy * 1.2
		
		for i = 1, count do
			local v = getBandValueFft(i)
			local mapped = Map(v, 0, maxVal, 0, 28)
			oledDrawFilledRect(bandX(i, count), 0, barWidth, math.ceil(mapped), 1)
		end
		
		-- Show max_energy as a horizontal line
		local maxY = Map(_M.max_energy, 0, maxVal, 0, 28)
		oledDrawFastHLine(0, maxY, 128, 1)
		_M.calibrating = true
		local level = _M.getSpeechLevel(dt, nil, nil, 5)
		drawLevelBoxes(level, 5, 12, 30)

		oledDrawFastVLine(bandX(_M.band_start, count), 0, 28, 1)
		oledDrawFastVLine(bandX(_M.band_end, count), 0, 28, 1)

		oledSetCursor(0, 44)
		oledDrawText("Max energy: " .. math.floor(_M.max_energy))
		oledSetCursor(0, 54)
		oledDrawText("UP/DN +-5000 CONFIRM=save")

	elseif _M.state == CALIB_STATE_ADJ_BANDEND then
		local count = getBandCountFft()
		local barWidth = bandBarWidth(count)
		
		-- Use max_energy as the max for the Map function
		local maxVal = _M.max_energy * 1.2
		
		for i = 1, count do
			local v = getBandValueFft(i)
			local mapped = Map(v, 0, maxVal, 0, 28)
			oledDrawFilledRect(bandX(i, count), 0, barWidth, math.ceil(mapped), 1)
		end
		local minEn = Map(_M.min_energy, 0, maxVal, 0, 28)
		oledDrawFastHLine(0, minEn, 128, 1)

		oledDrawFastVLine(bandX(_M.band_start, count), 0, 28, 1)
		oledDrawFastVLine(bandX(_M.band_end, count), 0, 28, 1)
		_M.calibrating = true
		local level = _M.getSpeechLevel(dt, nil, nil, 5)
		drawLevelBoxes(level, 5, 12, 30)

		oledSetCursor(0, 44)
		oledDrawText("Band end: " .. _M.band_end)
		oledSetCursor(0, 54)
		oledDrawText("UP/DN +-1 CONFIRM=save")
	elseif _M.state == CALIB_STATE_ADJ_BANDSTART then
		local count = getBandCountFft()
		local barWidth = bandBarWidth(count)
		
		-- Use max_energy as the max for the Map function
		local maxVal = _M.max_energy * 1.2
		
		for i = 1, count do
			local v = getBandValueFft(i)
			local mapped = Map(v, 0, maxVal, 0, 28)
			oledDrawFilledRect(bandX(i, count), 0, barWidth, math.ceil(mapped), 1)
		end

		local minEn = Map(_M.min_energy, 0, maxVal, 0, 28)
		oledDrawFastHLine(0, minEn, 128, 1)

		oledDrawFastVLine(bandX(_M.band_start, count), 0, 28, 1)
		oledDrawFastVLine(bandX(_M.band_end, count), 0, 28, 1)
		_M.calibrating = true
		local level = _M.getSpeechLevel(dt, nil, nil, 5)
		drawLevelBoxes(level, 5, 12, 30)

		oledSetCursor(0, 44)
		oledDrawText("Band start: " .. _M.band_start)
		oledSetCursor(0, 54)
		oledDrawText("UP/DN +-1 CONFIRM=save")
	end

	oledDisplay()
end

local function finalizeCalibration()
	local bandStart, bandEnd = _M.bandStart, _M.bandEnd

	local noiseEnergy = 0
	for i = bandStart, bandEnd do
		noiseEnergy = noiseEnergy + (_M.noiseBandValues[i] or 0)
	end

	local minEnergy = _M.floorEnergy or 0
	local maxEnergy = _M.ceilEnergy or (minEnergy * 1.5)

	local threshold = noiseEnergy * 1.5
	if threshold >= minEnergy then
		threshold = minEnergy * 0.5
	end

	if maxEnergy <= minEnergy then
		maxEnergy = minEnergy * 1.5
	end

	_M.result = {
		band_start = bandStart,
		band_end = bandEnd,
		frist_frame_threshold = threshold,
		min_energy = minEnergy,
		max_energy = maxEnergy,
	}

	return _M.result
end

local function saveCalibrationBackup()
	_M.savedCalibration = {
		noise_threshold        = _M.noise_threshold,
		band_start             = _M.band_start,
		band_end               = _M.band_end,
		min_energy             = _M.min_energy,
		max_energy             = _M.max_energy,
		frist_frame_threshold  = _M.frist_frame_threshold,
	}
end

local function applyCalibrationResult()
	local r = _M.result

	_M.noise_threshold       = _M.noiseThreshold
	_M.band_start            = r.band_start
	_M.band_end              = r.band_end
	_M.min_energy            = r.min_energy
	_M.max_energy            = r.max_energy
	_M.frist_frame_threshold = r.frist_frame_threshold

	setNoiseThreshold(_M.noise_threshold)

	_M.smoothed = 0
	_M.level = 0
end

local function restoreCalibrationBackup()
	local b = _M.savedCalibration
	if not b then return end

	_M.noise_threshold       = b.noise_threshold
	_M.band_start             = b.band_start
	_M.band_end               = b.band_end
	_M.min_energy             = b.min_energy
	_M.max_energy             = b.max_energy
	_M.frist_frame_threshold  = b.frist_frame_threshold

	setNoiseThreshold(_M.noise_threshold)
end

function _M.Calibrate(dt)
	if _M.state == CALIB_STATE_MENU then
		_M.menu.handle(dt)
		return
	end

	if _M.showingInstructions then
		if input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			_M.showingInstructions = false
		end
		if _M.state ~= CALIB_STATE_NOISE and anyDirectionPressed() then
			startFullCalibration()
		end
		return
	end

	if _M.state == CALIB_STATE_NOISE then
		if input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			_M.state = CALIB_STATE_TALK
			_M.showingInstructions = true
			_M.noiseThreshold = _M.avgNoise * 1.1
			setNoiseThreshold(_M.noiseThreshold)
			_M.maxBandInput = _M.noiseThreshold * 8
		end
		if anyDirectionPressed() then
			_M.maxBandInput = 100
		end

	elseif _M.state == CALIB_STATE_TALK then
		if input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			if _M.bandStart and _M.floorEnergy then
				finalizeCalibration()
				_M.state = CALIB_STATE_REVIEW
			end
		end
		if anyDirectionPressed() then
			startFullCalibration()
		end

	elseif _M.state == CALIB_STATE_REVIEW then
		if input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			saveCalibrationBackup()
			applyCalibrationResult()
			_M.state = CALIB_STATE_TEST
		end
		if anyDirectionPressed() then
			startFullCalibration()
		end

	elseif _M.state == CALIB_STATE_TEST then
		if input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			dictSet("fft_noise_threshold", tostring(_M.noiseThreshold))
			dictSet("fft_speech_min_energy", tostring(_M.result.min_energy))
			dictSet("fft_speech_max_energy", tostring(_M.result.max_energy))
			dictSet("fft_speech_band_start", tostring(_M.result.band_start))
			dictSet("fft_speech_band_end", tostring(_M.result.band_end))
			dictSet("fft_speech_frist_frame_threshold", tostring(_M.result.frist_frame_threshold))
			_M.result = nil
			dictSave()
			_M.loadCalibration()
			goToMenu()
		end
		if anyDirectionPressed() then
			restoreCalibrationBackup()
			startFullCalibration()
		end

	elseif _M.state == CALIB_STATE_ADJ_FRIST then
		if input.readButtonStatus(BUTTON_UP) == BUTTON_JUST_PRESSED then
			_M.frist_frame_threshold = _M.frist_frame_threshold + FRIST_STEP
		end
		if input.readButtonStatus(BUTTON_DOWN) == BUTTON_JUST_PRESSED then
			_M.frist_frame_threshold = math.max(0, _M.frist_frame_threshold - FRIST_STEP)
		end
		
		-- Ensure min_energy is not less than frist_frame_threshold
		if _M.min_energy < _M.frist_frame_threshold then
			_M.min_energy = _M.frist_frame_threshold
		end

		if input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			dictSet("fft_speech_frist_frame_threshold", tostring(_M.frist_frame_threshold))
			dictSet("fft_speech_min_energy", tostring(_M.min_energy))
			dictSave()
			goToMenu()
		end
		if input.readButtonStatus(BUTTON_LEFT) == BUTTON_JUST_PRESSED then			
			_M.loadCalibration()
			goToMenu()
		end

	elseif _M.state == CALIB_STATE_ADJ_NOISE then
		if input.readButtonStatus(BUTTON_UP) == BUTTON_JUST_PRESSED then
			_M.noise_threshold = _M.noise_threshold + NOISE_STEP
		end
		if input.readButtonStatus(BUTTON_DOWN) == BUTTON_JUST_PRESSED then
			_M.noise_threshold = math.max(0, _M.noise_threshold - NOISE_STEP)
		end
		setNoiseThreshold(_M.noise_threshold)

		if input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			dictSet("fft_noise_threshold", tostring(_M.noise_threshold))
			dictSave()
			goToMenu()
		end
		if input.readButtonStatus(BUTTON_LEFT) == BUTTON_JUST_PRESSED then
			_M.loadCalibration()
			goToMenu()
		end

	elseif _M.state == CALIB_STATE_ADJ_MIN then
		if input.readButtonStatus(BUTTON_UP) == BUTTON_JUST_PRESSED then
			_M.min_energy = _M.min_energy + ENERGY_STEP
		end
		if input.readButtonStatus(BUTTON_DOWN) == BUTTON_JUST_PRESSED then
			_M.min_energy = math.max(0, _M.min_energy - ENERGY_STEP)
		end
		
		-- Ensure min_energy is not less than frist_frame_threshold
		if _M.min_energy < _M.frist_frame_threshold then
			_M.min_energy = _M.frist_frame_threshold
		end

		if input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			dictSet("fft_speech_min_energy", tostring(_M.min_energy))
			dictSave()
			goToMenu()
		end
		if input.readButtonStatus(BUTTON_LEFT) == BUTTON_JUST_PRESSED then
			_M.loadCalibration()
			goToMenu()
		end

	elseif _M.state == CALIB_STATE_ADJ_MAX then
		if input.readButtonStatus(BUTTON_UP) == BUTTON_JUST_PRESSED then
			_M.max_energy = _M.max_energy + ENERGY_STEP
		end
		if input.readButtonStatus(BUTTON_DOWN) == BUTTON_JUST_PRESSED then
			_M.max_energy = math.max(_M.min_energy + ENERGY_STEP, _M.max_energy - ENERGY_STEP)
		end

		if input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			dictSet("fft_speech_max_energy", tostring(_M.max_energy))
			dictSave()
			goToMenu()
		end
		if input.readButtonStatus(BUTTON_LEFT) == BUTTON_JUST_PRESSED then
			_M.loadCalibration()
			goToMenu()
		end

	elseif _M.state == CALIB_STATE_ADJ_BANDEND then
		local maxBand = getBandCountFft()

		if input.readButtonStatus(BUTTON_UP) == BUTTON_JUST_PRESSED then
			_M.band_end = math.min(_M.band_end + 1, maxBand)
		end
		if input.readButtonStatus(BUTTON_DOWN) == BUTTON_JUST_PRESSED then
			_M.band_end = math.max(_M.band_end - 1, _M.band_start + 1)
		end

		if input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			dictSet("fft_speech_band_end", tostring(_M.band_end))
			dictSave()
			goToMenu()
		end
		if input.readButtonStatus(BUTTON_LEFT) == BUTTON_JUST_PRESSED then
			_M.loadCalibration()
			goToMenu()
		end
	elseif _M.state == CALIB_STATE_ADJ_BANDSTART then
		local maxBand = getBandCountFft()


		if input.readButtonStatus(BUTTON_UP) == BUTTON_JUST_PRESSED then
			_M.band_start = math.min(_M.band_start+1, _M.band_end-1)
		end
		if input.readButtonStatus(BUTTON_DOWN) == BUTTON_JUST_PRESSED then
			_M.band_start = math.max(_M.band_start-1, 1)
		end

		if input.readButtonStatus(BUTTON_CONFIRM) == BUTTON_JUST_PRESSED then
			dictSet("fft_speech_band_start", tostring(_M.band_start))
			dictSave()
			goToMenu()
		end
		if input.readButtonStatus(BUTTON_LEFT) == BUTTON_JUST_PRESSED then
			_M.loadCalibration()
			goToMenu()
		end
	end
end



function _M.load()
	local cfg = configloader.Get()
	if not cfg.fft or not cfg.fft.enabled then
		generic.displaySplashMessage("Starting:\nNo fft configued")
		delay(200)
		return
	end
	local ui = require("ui")
	local menu = ui.generateUi("Mic Calibration", nil, exitCalibration)
	menu.addElement(function() return "Full calibration" end, startFullCalibration)
	menu.addElement(function() return "Adjust frame thresh" end, startAdjustFrist)
	menu.addElement(function() return "Adjust noise thresh" end, startAdjustNoise)
	menu.addElement(function() return "Adjust min energy" end, startAdjustMin)
	menu.addElement(function() return "Adjust max energy" end, startAdjustMax)
	menu.addElement(function() return "Adjust band start" end, startAdjustBandStart)
	menu.addElement(function() return "Adjust band end" end, startAdjustBandEnd)
	menu.addElement(function() return "Exit" end, exitCalibration)
	_M.menu = menu

	local cfg = cfg.fft

	if not tonumber(cfg.gpio) then
		error("GPIO for fft must be a number.")
	end

	cfg.gpio = tonumber(cfg.gpio)

	if cfg.gpio <= 0 or cfg.gpio >= 11 then
		error("GPIO for fft must be channel1, that means gpipo between 1 and 10")
	end

	cfg.samples = cfg.samples or 512
	cfg.sampling_frequency = cfg.sampling_frequency or 44100


	_M.noise_threshold = cfg.noise_threshold or _M.noise_threshold
	_M.band_start 	= cfg.speech_band_start or _M.band_start
	_M.band_end 	= cfg.speech_band_end or _M.band_end
	_M.min_energy 	= cfg.speech_min_energy or _M.min_energy
	_M.band_end 	= cfg.speech_max_energy or _M.max_energy
	_M.frist_frame_threshold 	= cfg.speech_frist_frame_threshold or _M.frist_frame_threshold


	_M.loadCalibration()
	cfg.band_count = cfg.band_count or 16
	_M.band_count = cfg.band_count

	fft.enabled = beginFft(cfg.gpio, cfg.samples, cfg.sampling_frequency, _M.noise_threshold, cfg.band_count)

	

	if not fft.enabled then
		generic.displaySplashMessage("FFT FAILED")
		delay(2000)
	end

	startFft()
end


return _M