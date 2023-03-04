// /**
//  * @file module_handler.cpp
//  * @author Bernd Giesecke (bernd@giesecke.tk)
//  * @brief Find and handle WisBlock sensor modules
//  * @version 0.1
//  * @date 2022-04-10
//  *
//  * @copyright Copyright (c) 2022
//  *
//  */
// #include <Arduino.h>
// #include "main.h"

// /** Mean Sea Level Pressure */
// float mean_sea_level_press = 1013.25;

// /**
//  * @brief Initialize modules
//  *
//  */
// void init_modules(void)
// {
// 	Wire.begin();
// 	// Some modules support only 100kHz
// 	Wire.setClock(100000);

// 	if (!init_rak13011())
// 	{
// 		MYLOG("MODS", "Could not initialize RAK13011");
// 	}
// 	else
// 	{
// 		Serial.println("+EVT:RAK13011 OK");
// 	}
// }

// /**
//  * @brief Read values from the found modules
//  *
//  */
// void get_sensor_values(void)
// {

// }