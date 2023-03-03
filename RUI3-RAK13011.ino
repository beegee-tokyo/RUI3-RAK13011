#include <Arduino.h>
#include <CayenneLPP.h>
#include "ArrayQueue.h"

//******************************************************************//
// RAK13011 INT1_PIN
//******************************************************************//
// Slot A      WB_IO1
// Slot B      WB_IO2 ( not recommended, pin conflict with IO2)
// Slot C      WB_IO3
// Slot D      WB_IO5
// Slot E      WB_IO4
// Slot F      WB_IO6
//******************************************************************//

#define RAK13011_SLOT 'A'

/** Interrupt pin, depends on slot */
// Slot A
#if RAK13011_SLOT == 'A'
#pragma message "Slot A"
uint32_t SW_INT_PIN = WB_IO1;
// Slot B
#elif RAK13011_SLOT == 'B'
#pragma message "Slot B"
uint32_t SW_INT_PIN = WB_IO2;
// Slot C
#elif RAK13011_SLOT == 'C'
#pragma message "Slot C"
uint32_t SW_INT_PIN = WB_IO3;
// Slot D
#elif RAK13011_SLOT == 'D'
#pragma message "Slot D"
uint32_t SW_INT_PIN = WB_IO5;
// Slot E
#elif RAK13011_SLOT == 'E'
#pragma message "Slot E"
uint32_t SW_INT_PIN = WB_IO4;
// Slot F
#elif RAK13011_SLOT == 'F'
#pragma message "Slot F"
uint32_t SW_INT_PIN = WB_IO6;
#endif

ArrayQueue Fifo;

volatile int switch_status = 0;

volatile bool handler_available = true;

/** Flag if TX is active */
volatile bool tx_active = false;

/** Initialization results */
bool ret;

// Forward declarations
bool init_rak13011(void);
void handle_rak13011(void *);
bool init_irq_at(void);
bool init_status_at(void);
void switch_int_handler(void);
void switch_bounce_check(void *);

#define LPP_CHANNEL_BATT 1	  // Base Board
#define LPP_CHANNEL_SWITCH 48 // RAK13011

/** LoRaWAN packet */
CayenneLPP g_solution_data(255);

/** fPort to send packages */
uint8_t set_fPort = 2;
/** Packet is confirmed/unconfirmed (Set with AT commands) */
bool g_confirmed_mode = false;
/** If confirmed packet, number or retries (Set with AT commands) */
uint8_t g_confirmed_retry = 0;
/** Data rate  (Set with AT commands) */
uint8_t g_data_rate = 3;
/** Frequent data sending time */
uint32_t g_send_repeat_time = 30000;

/**
 * @brief IRQ handler, shows status on WisBlock LED's
 * Triggers the main handler through a timer
 *
 */
void switch_int_handler(void)
{
	Serial.println("Interrupt, start bounce check");
	switch_status = digitalRead(SW_INT_PIN);
	api.system.timer.start(RAK_TIMER_2, 50, NULL);
}

/**
 * @brief Check if status of switch is stable or if a bounce was triggered
 *
 */
void switch_bounce_check(void *)
{
	Serial.println("Bounce check");
	// Serial.println("Interrupt");
	int new_switch_status = digitalRead(SW_INT_PIN);
	if (new_switch_status != switch_status)
	{
		Serial.println("Bounce detected");
		return;
	}
	if (switch_status == LOW)
	{
		digitalWrite(LED_GREEN, HIGH);
		digitalWrite(LED_BLUE, LOW);
		if (!Fifo.enQueue(false))
		{
			Serial.println("FiFo full");
			return;
		}
	}
	else
	{
		digitalWrite(LED_GREEN, LOW);
		digitalWrite(LED_BLUE, HIGH);
		if (!Fifo.enQueue(true))
		{
			Serial.println("FiFo full");
			return;
		}
		// enable_interrupts();
	}

	Serial.println("Added event to queue");
	// Check if the handler is still active
	if (handler_available)
	{
		Serial.println("Start event handler");
		handler_available = false;
		handle_rak13011(NULL);
	}
}

/**
 * @brief Callback after packet was received
 *
 * @param data Structure with the received data
 */
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	Serial.printf("RX, port %d, DR %d, RSSI %d, SNR %d\n", data->Port, data->RxDatarate, data->Rssi, data->Snr);
	for (int i = 0; i < data->BufferSize; i++)
	{
		Serial.printf("%02X", data->Buffer[i]);
	}
	Serial.print("\r\n");
}

/**
 * @brief Callback after TX is finished
 *
 * @param status TX status
 */
void sendCallback(int32_t status)
{
	tx_active = false;
	Serial.printf("TX status %d\n", status);
}

/**
 * @brief Callback after join request cycle
 *
 * @param status Join result
 */
void joinCallback(int32_t status)
{
	if (status != 0)
	{
		if (!(ret = api.lorawan.join()))
		{
			Serial.println("LoRaWan OTAA - join fail!\n");
		}
	}
	else
	{
		bool result_set = api.lorawan.dr.set(g_data_rate);
	}
}

void setup()
{
	g_confirmed_mode = api.lorawan.cfm.get();

	g_confirmed_retry = api.lorawan.rety.get();

	g_data_rate = api.lorawan.dr.get();

	// Setup the callbacks for joined and send finished
	api.lorawan.registerRecvCallback(receiveCallback);
	api.lorawan.registerSendCallback(sendCallback);
	api.lorawan.registerJoinCallback(joinCallback);

	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, HIGH);

	// pinMode(WB_IO2, OUTPUT);
	// digitalWrite(WB_IO2, HIGH);

	// Start Serial
	Serial.begin(115200);
	// For RAK3172 just wait a little bit for the USB to be ready
	delay(5000);

	Serial.println("RAKwireless RUI3 Node");
	Serial.println("------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.println("------------------------------------------------------");

	// Add custom AT
	if (!init_irq_at())
	{
		Serial.println("Failed to add IRQ change AT command");
	}
	if (!init_status_at())
	{
		Serial.println("Failed to add status AT command");
	}

	// Create a timers for handling the events
	api.system.timer.create(RAK_TIMER_2, switch_bounce_check, RAK_TIMER_ONESHOT);
	api.system.timer.create(RAK_TIMER_3, handle_rak13011, RAK_TIMER_ONESHOT);

	Serial.printf("Initialize Interrupt on GPIO %d\n", SW_INT_PIN);
	pinMode(SW_INT_PIN, INPUT);
	attachInterrupt(SW_INT_PIN, switch_int_handler, CHANGE);
	Serial.println("Interrupt Initialized ");

	if (digitalRead(SW_INT_PIN) == LOW)
	{
		digitalWrite(LED_GREEN, HIGH);
		digitalWrite(LED_BLUE, LOW);
	}
	else
	{
		digitalWrite(LED_GREEN, LOW);
		digitalWrite(LED_BLUE, HIGH);
	}
}

/**
 * @brief This example is complete timer
 * driven. The loop() does nothing than
 * sleep.
 *
 */
void loop()
{
	api.system.sleep.all();
}

void handle_rak13011(void *)
{
	if (!tx_active)
	{
		// Reset automatic interval sending if active
		if (g_send_repeat_time != 0)
		{
			// Restart a timer
			api.system.timer.stop(RAK_TIMER_0);
			api.system.timer.start(RAK_TIMER_0, g_send_repeat_time, NULL);
		}

		// Clear payload
		g_solution_data.reset();

		noInterrupts();
		g_solution_data.addPresence(LPP_CHANNEL_SWITCH, !Fifo.deQueue() ? 0 : 1);
		interrupts();

		// Add battery voltage
		g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());

		// Send the packet
		Serial.printf("Send packet with size %d on port %d\n", g_solution_data.getSize(), set_fPort);

		// Send the packet
		if (api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), set_fPort)) // , g_confirmed_mode, g_confirmed_retry))
		{
			tx_active = true;
			Serial.println("Packet enqueued");
		}
		else
		{
			Serial.println("Send failed");
		}
	}
	else
	{
		Serial.println("TX still active");
		// Fifo.deQueue();
	}

	if (!Fifo.isEmpty())
	{
		// Event queue is not empty. Trigger next packet in 5 seconds
		api.system.timer.start(RAK_TIMER_3, 5000, NULL);
	}
	else
	{
		handler_available = true;
		Serial.println("Queue is empty");
	}
}
