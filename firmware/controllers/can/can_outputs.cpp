#include "pch.h"
#include "can_outputs.h"
#include "can_msg_tx.h"
#include "output_channel_reader.h"
#include "can_rx_custom.h"
#include "logic_outputs.h"
#include "debounce.h"

constexpr size_t kTriggeredCanSlots = 8;
static bool triggeredCanTriggerPrev[kTriggeredCanSlots] = {};
static ButtonDebounce triggeredCanDebounce[kTriggeredCanSlots] = {
	ButtonDebounce("triggered_can0"),
	ButtonDebounce("triggered_can1"),
	ButtonDebounce("triggered_can2"),
	ButtonDebounce("triggered_can3"),
	ButtonDebounce("triggered_can4"),
	ButtonDebounce("triggered_can5"),
	ButtonDebounce("triggered_can6"),
	ButtonDebounce("triggered_can7"),
};
static switch_input_pin_e triggeredCanPrevPin[kTriggeredCanSlots] = {};
static pin_input_mode_e triggeredCanPrevMode[kTriggeredCanSlots] = {};

static constexpr efitimems_t kTriggeredCanDebounceMs = 150;

static void processTriggeredCanTriggers();

static bool isLogicOutputTriggerPin(switch_input_pin_e pin) {
#ifdef Gpio_LOGIC_OUTPUT_1
	const auto value = static_cast<uint32_t>(pin);
	return value >= Gpio_LOGIC_OUTPUT_1 && value <= Gpio_LOGIC_OUTPUT_10;
#else
	(void)pin;
	return false;
#endif
}

static size_t triggerPinToLogicIndex(switch_input_pin_e pin) {
#ifdef Gpio_LOGIC_OUTPUT_1
	return static_cast<size_t>(static_cast<uint32_t>(pin) - Gpio_LOGIC_OUTPUT_1);
#else
	(void)pin;
	return 0;
#endif
}

static inline CI rateToCi(can_outputs_rate_e r) {
	switch ((unsigned)r) {
		case 0: return CI::_1000ms;
		case 1: return CI::_500ms;
		case 2: return CI::_250ms;
		case 3: return CI::_200ms;
		case 4: return CI::_100ms;
		case 5: return CI::_50ms;
		case 6: return CI::_20ms;
		case 7: return CI::_10ms;
		case 8: return CI::_5ms;
		default: return CI::_100ms;
	}
}

static inline uint8_t  clampU8 (int32_t v) { return (v < 0) ? 0u   : (v > 255)   ? 255u   : (uint8_t)v; }
static inline uint16_t clampU16(int64_t v) { return (v < 0) ? 0u   : (v > 65535) ? 65535u : (uint16_t)v; }

static inline int64_t roundAwayFromZero(float f) {
	return (f >= 0.0f) ? (int64_t)(f + 0.5f) : (int64_t)(f - 0.5f);
}

static bool shouldSendCanOutput(const CanOutput& co) {
	const auto mode = co.enableMode;

	switch (mode) {
		case CAN_OUTPUT_DISABLED:
			return false;

		case CAN_OUTPUT_ENABLED:
			return true;

		case CAN_OUTPUT_ENABLED_VIA_LOGIC: {
			const uint8_t logicIndex = static_cast<uint8_t>(co.logicOutput);
			if (logicIndex >= 10) {
				return false;
			}

			return isLogicOutputActive(logicIndex);
		}

		default:
			return false;
	}
}

// Common logic for filling CAN frame bytes from CanOutput configuration
template <typename OutputConfig>
static void fillCanOutputBytes(CanTxMessage& msg, const OutputConfig& co, uint8_t dlc) {
	for (uint8_t k = 0; k < dlc; ++k) {
		msg[k] = 0;
	}

	for (uint8_t b = 0; b < dlc; ) {
		const auto& bc = co.byte[b];

		bool isVar = false;
		bool is16 = false;

		switch (bc.type) {
			case CanOutput_Fixed8:
				break;

			case CanOutput_Fixed16:
				is16 = true;
				break;

			case CanOutput_Var8:
				isVar = true;
				break;

			case CanOutput_Var16:
				isVar = true;
				is16 = true;
				break;

			default:
				break;
		}
		
		uint16_t mul = bc.multiplier;
		uint16_t div = bc.divider;
		if (mul == 0) mul = 1;
		if (div == 0) div = 1;
		const int16_t off = bc.offset;

		// Degrade 16-bit to 8-bit if it doesn't fit
		// (either byte 8, or insufficient DLC space)
		if (is16 && ((b == 7) || ((uint8_t)(b + 1) >= dlc))) {
			is16 = false;
		}

		int64_t scaled = 0;
		if (isVar) {
			float v = 0.0f;
			if (auto m = readOutputChannel(bc.var); m.Valid) {
				v = m.Value;
			}
			const float f = ((v * (float)mul) / (float)div) + (float)off;
			scaled = roundAwayFromZero(f);
		} else {
			const uint32_t baseFixed = bc.fixed;
			scaled = ((int64_t)baseFixed * (int64_t)mul) / (int64_t)div;
			scaled += (int64_t)off;
		}

		if (!is16) {
			msg[b] = clampU8((int32_t)scaled);
			b += 1;
		} else {
			const uint16_t v16 = clampU16(scaled);
			if (co.isBigEndian) {
				msg[b]     = (uint8_t)((v16 >> 8) & 0xFF);
				msg[b + 1] = (uint8_t)(v16 & 0xFF);
			} else {
				msg[b]     = (uint8_t)(v16 & 0xFF);
				msg[b + 1] = (uint8_t)((v16 >> 8) & 0xFF);
			}
			b += 2;
		}
	}
}

void canOutputsSend(CanCycle cycle) {
	for (size_t i = 0; i < efi::size(engineConfiguration->canOutputs); i++) {
		const CanOutput& co = engineConfiguration->canOutputs[i];
		if (!shouldSendCanOutput(co)) {
			continue;
		}
		if (!cycle.isInterval(rateToCi(co.rate))) {
			continue;
		}

		const uint32_t canId = parseCanId(co.id);
		// Skip if CAN ID is 0 (empty/disabled)
		if (canId == 0) {
			continue;
		}
		const bool     ext = IS_EXT_RANGE_ID(canId);
		const uint8_t  dlc = (uint8_t)co.dlc + 1u;
		if (dlc > 8) {
			continue;
		}
		const uint8_t bus = co.isSecondaryCAN ? 1u : 0u;

		{
			CanTxMessage msg(CanCategory::NBC, canId, dlc, bus, ext);
			fillCanOutputBytes(msg, co, dlc);
		}
	}

	processTriggeredCanTriggers();
}

void sendTriggeredCanMessage(size_t index) {
	if (index >= kTriggeredCanSlots) {
		return;
	}

	const auto& co = engineConfiguration->triggeredCanOutput[index];

	const uint32_t canId = parseCanId(co.id);
	// Skip if CAN ID is 0 (empty/disabled)
	if (canId == 0) {
		return;
	}
	const bool     ext = IS_EXT_RANGE_ID(canId);
	const uint8_t  dlc = (uint8_t)co.dlc + 1u;
	if (dlc > 8) {
		return;
	}
	const uint8_t bus = co.isSecondaryCAN ? 1u : 0u;

	{
		CanTxMessage msg(CanCategory::NBC, canId, dlc, bus, ext);
		fillCanOutputBytes(msg, co, dlc);
	}
}

static void processTriggeredCanTriggers() {
	for (size_t i = 0; i < kTriggeredCanSlots; i++) {
		const auto& slot = engineConfiguration->triggeredCanOutput[i];
		const auto trigger = slot.triggeredCanOutputTrigger;
		const auto mode = slot.triggeredCanOutputTriggerPinMode;

		const bool isLogic = isLogicOutputTriggerPin(trigger);

		if (!isLogic && !isBrainPinValid(static_cast<brain_pin_e>(trigger))) {
			triggeredCanTriggerPrev[i] = false;
			triggeredCanPrevPin[i] = trigger;
			triggeredCanPrevMode[i] = mode;
			continue;
		}

		bool active = false;

		const bool prevWasLogic = isLogicOutputTriggerPin(triggeredCanPrevPin[i]);
		bool sourceChanged = (trigger != triggeredCanPrevPin[i]) || (isLogic != prevWasLogic);
		if (!isLogic && (mode != triggeredCanPrevMode[i])) {
			sourceChanged = true;
		}

		if (isLogic) {
			const size_t logicIndex = triggerPinToLogicIndex(trigger);
			if (logicIndex < 10) {
				active = isLogicOutputActive(logicIndex);
			}
		} else {
			if ((trigger != triggeredCanPrevPin[i]) || (mode != triggeredCanPrevMode[i]) || prevWasLogic) {
				triggeredCanDebounce[i].init(kTriggeredCanDebounceMs,
					engineConfiguration->triggeredCanOutput[i].triggeredCanOutputTrigger,
					engineConfiguration->triggeredCanOutput[i].triggeredCanOutputTriggerPinMode);
			}

			active = triggeredCanDebounce[i].readPinState();
		}

		bool prev = triggeredCanTriggerPrev[i];
		if (sourceChanged) {
			prev = active;
			triggeredCanTriggerPrev[i] = active;
		}

		triggeredCanPrevPin[i] = trigger;
		triggeredCanPrevMode[i] = mode;

		const auto edge = slot.triggeredCanOutputEdge;

		bool shouldSend = false;
		switch (edge) {
			case TRIGGER_CAN_EDGE_FALLING:
				shouldSend = prev && !active;
				break;

			case TRIGGER_CAN_EDGE_BOTH:
				shouldSend = prev != active;
				break;

			case TRIGGER_CAN_EDGE_RISING:
			default:
				shouldSend = active && !prev;
				break;
		}

		if (shouldSend) {
			sendTriggeredCanMessage(i);
		}

		triggeredCanTriggerPrev[i] = active;
	}
}