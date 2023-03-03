/**
 * @file custom_at.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Custom AT commands
 * @version 0.1
 * @date 2023-01-31
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <Arduino.h>

// The GPIO for the external interrupt
extern uint32_t SW_INT_PIN;
/** Regions as text array */
char *regions_list[] = {"EU433", "CN470", "RU864", "IN865", "EU868", "US915", "AU915", "KR920", "AS923", "AS923-2", "AS923-3", "AS923-4"};
/** Network modes as text array*/
char *nwm_list[] = {"P2P", "LoRaWAN", "FSK"};
/** Names of GPIO port numbers */
char* gpio_name[] = {"WB_IO1", "WB_IO2", "WB_IO3", "WB_IO4", "WB_IO5", "WB_IO6"};
/** GPIO ports */
uint32_t gpio_ports[] = {WB_IO1, WB_IO2, WB_IO3, WB_IO4, WB_IO5, WB_IO6};

// IRQ handler
void switch_int_handler(void);

int change_irq_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		Serial.print(cmd);
		int wb_io_num = 0;
		switch (SW_INT_PIN)
		{
		case WB_IO1:
			wb_io_num = 0;
			break;
		case WB_IO2:
			wb_io_num = 1;
			break;
		case WB_IO3:
			wb_io_num = 2;
			break;
		case WB_IO4:
			wb_io_num = 3;
			break;
		case WB_IO5:
			wb_io_num = 4;
			break;
		case WB_IO6:
			wb_io_num = 5;
			break;
		}
		Serial.printf("=%ld %s\r\n", wb_io_num + 1, gpio_name[wb_io_num]);
	}
	else if (param->argc == 1)
	{
		Serial.printf("param->argv[0] >> %s\n", param->argv[0]);
		for (int i = 0; i < strlen(param->argv[0]); i++)
		{
			if (!isdigit(*(param->argv[0] + i)))
			{
				Serial.printf("%d is no digit\n", i);
				return AT_PARAM_ERROR;
			}
		}

		uint32_t new_irq_pin = strtoul(param->argv[0], NULL, 10);

		Serial.printf("New GPIO %ld\n", new_irq_pin);

		if ((new_irq_pin < 1) || (new_irq_pin > 6))
		{
			return AT_PARAM_ERROR;
		}

		detachInterrupt(SW_INT_PIN);
		SW_INT_PIN = gpio_ports[new_irq_pin - 1];
		Serial.printf("New GPIO %ld %ld\n", new_irq_pin, gpio_ports[new_irq_pin - 1]);
		pinMode(SW_INT_PIN, INPUT);
		attachInterrupt(SW_INT_PIN, switch_int_handler, CHANGE);
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

int status_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	String value_str = "";
	int nw_mode = 0;
	int region_set = 0;
	uint8_t key_eui[16] = {0}; // efadff29c77b4829acf71e1a6e76f713

	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		Serial.println("Device Status:");
		value_str = api.system.hwModel.get();
		value_str.toUpperCase();
		Serial.printf("Module: %s\r\n", value_str.c_str());
		Serial.printf("Version: %s\r\n", api.system.firmwareVer.get().c_str());

		nw_mode = api.lorawan.nwm.get();
		Serial.printf("Network mode %s\r\n", nwm_list[nw_mode]);
		if (nw_mode == 1)
		{
			Serial.printf("Network %s\r\n", api.lorawan.njs.get() ? "joined" : "not joined");
			region_set = api.lorawan.band.get();
			Serial.printf("Region: %d\r\n", region_set);
			Serial.printf("Region: %s\r\n", regions_list[region_set]);
			if (api.lorawan.njm.get())
			{
				Serial.println("OTAA mode");
				api.lorawan.deui.get(key_eui, 8);
				Serial.printf("DevEUI = %02X%02X%02X%02X%02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
							  key_eui[4], key_eui[5], key_eui[6], key_eui[7]);
				api.lorawan.appeui.get(key_eui, 8);
				Serial.printf("AppEUI = %02X%02X%02X%02X%02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
							  key_eui[4], key_eui[5], key_eui[6], key_eui[7]);
				api.lorawan.appkey.get(key_eui, 16);
				Serial.printf("AppKey = %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
							  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
							  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
							  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
			}
			else
			{
				Serial.println("ABP mode");
				api.lorawan.appskey.get(key_eui, 16);
				Serial.printf("AppsKey = %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
							  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
							  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
							  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
				api.lorawan.nwkskey.get(key_eui, 16);
				Serial.printf("NwsKey = %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
							  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
							  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
							  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
				api.lorawan.daddr.set(key_eui, 4);
				Serial.printf("DevAddr = %02X%02X%02X%02X\r\n",
							  key_eui[0], key_eui[1], key_eui[2], key_eui[3]);
			}
		}
		else if (nw_mode == 0)
		{
			Serial.printf("Frequency = %d\r\n", api.lorawan.pfreq.get());
			Serial.printf("SF = %d\r\n", api.lorawan.psf.get());
			Serial.printf("BW = %d\r\n", api.lorawan.pbw.get());
			Serial.printf("CR = %d\r\n", api.lorawan.pcr.get());
			Serial.printf("Preamble length = %d\r\n", api.lorawan.ppl.get());
			Serial.printf("TX power = %d\r\n", api.lorawan.ptp.get());
		}
		else
		{
			Serial.printf("Frequency = %d\r\n", api.lorawan.pfreq.get());
			Serial.printf("Bitrate = %d\r\n", api.lorawan.pbr.get());
			Serial.printf("Deviaton = %d\r\n", api.lorawan.pfdev.get());
		}
	}
	else
	{
		return AT_PARAM_ERROR;
	}
	return AT_OK;
}

bool init_irq_at(void)
{
	return api.system.atMode.add((char *)"IRQ",
								 (char *)" Get/Set GPIO for interrupt input: 1 = WB_IO1 (PB15) 2 = WB_IO2 (PA8) 3 = WB_IO3 (PB12) 4 = WB_IO4 (PB2) 5 - WB_IO5 (PA15) 6 = WB_IO6 (PA9)",
								 (char *)"IRQ",
								 change_irq_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

bool init_status_at(void)
{
	return api.system.atMode.add((char *)"STATUS",
								 (char *)"Get device information",
								 (char *)"STATUS", status_handler,
								 RAK_ATCMD_PERM_READ);
}
