#include "pch.h"
#include "defaults.h"
#include "board_overrides.h"

Gpio getCommsLedPin() {
    return Gpio::Unassigned;
}
Gpio getRunningLedPin() {
    return Gpio::Unassigned;
}
Gpio getWarningLedPin() {
    return Gpio::Unassigned;
}

static void KRC_M52_2_boardDefaultConfiguration() {

	//Injector output
	engineConfiguration->injectionPinMode = OM_DEFAULT;
	engineConfiguration->injectionPins[0] = Gpio::C6;
	engineConfiguration->injectionPins[1] = Gpio::D10;
	engineConfiguration->injectionPins[2] = Gpio::D14;
	engineConfiguration->injectionPins[3] = Gpio::D12;
	engineConfiguration->injectionPins[4] = Gpio::D15;
	engineConfiguration->injectionPins[5] = Gpio::D13;

        //Ignition output
	engineConfiguration->ignitionPinMode = OM_DEFAULT;
	engineConfiguration->ignitionPins[0] = Gpio::E2;
	engineConfiguration->ignitionPins[1] = Gpio::E3;
	engineConfiguration->ignitionPins[2] = Gpio::C13;
	engineConfiguration->ignitionPins[3] = Gpio::E6;
	engineConfiguration->ignitionPins[4] = Gpio::E4;
	engineConfiguration->ignitionPins[5] = Gpio::E5;

	//Digital out
	engineConfiguration->fuelPumpPin = Gpio::E11;
	engineConfiguration->tachOutputPin = Gpio::D7;

	//Analog
	engineConfiguration->iat.adcChannel = EFI_ADC_0;
	engineConfiguration->clt.adcChannel = EFI_ADC_1;
	engineConfiguration->tps1_1AdcChannel = EFI_ADC_2;
	engineConfiguration->map.sensor.hwChannel = EFI_ADC_3;
	engineConfiguration->vbattAdcChannel = EFI_ADC_4;
	engineConfiguration->afr.hwChannel = EFI_ADC_8;

	engineConfiguration->map.sensor.type = MT_MPXH6400;

	//Trigger 
	engineConfiguration->triggerInputPins[0] = Gpio::D3;
	engineConfiguration->camInputs[0] = Gpio::D4;

        // Idle configuration
        engineConfiguration->useStepperIdle = false;
        engineConfiguration->isDoubleSolenoidIdle = true;
        engineConfiguration->idle.solenoidPin = Gpio::B9;
        engineConfiguration->secondSolenoidPin = Gpio::B7;

	//Bias resistor
	engineConfiguration->clt.config.bias_resistor = 2490;
	engineConfiguration->iat.config.bias_resistor = 2490;

	//Analog calc
        engineConfiguration->analogInputDividerCoefficient = 1.52f;
        engineConfiguration->vbattDividerCoeff = (7.55f / 1.0f);
        engineConfiguration->adcVcc = 3.24f;

        //SD card
	engineConfiguration->isSdCardEnabled = true;
	engineConfiguration->is_enabled_spi_3 = true;
	engineConfiguration->sdCardSpiDevice = SPI_DEVICE_3;
	engineConfiguration->spi3mosiPin = Gpio::C10;
	engineConfiguration->spi3misoPin = Gpio::C11;
	engineConfiguration->spi3sckPin = Gpio::C12;
	engineConfiguration->sdCardCsPin = Gpio::D2;
	engineConfiguration->sdCardLogFrequency = 20;

	//CAN bus overwrites
	engineConfiguration->canTxPin = Gpio::D1;
	engineConfiguration->canRxPin = Gpio::D0;
        engineConfiguration->canWriteEnabled = true;
	engineConfiguration->canReadEnabled = true;
	engineConfiguration->canSleepPeriodMs = 50;
	engineConfiguration->canBaudRate = B500KBPS;

	//Engine configuration
	engineConfiguration->cylindersCount = 6;
	engineConfiguration->firingOrder = FO_1_5_3_6_2_4;
	engineConfiguration->ignitionMode = IM_INDIVIDUAL_COILS;
	engineConfiguration->crankingInjectionMode = IM_SIMULTANEOUS;
	engineConfiguration->injectionMode = IM_SEQUENTIAL;

        //Knock 
	engineConfiguration->enableSoftwareKnock = false;
	

}

void setup_custom_board_overrides() {

	custom_board_DefaultConfiguration = KRC_M52_2_boardDefaultConfiguration;
}
