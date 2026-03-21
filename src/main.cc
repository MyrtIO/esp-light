#include <MyrtIO/device/device.h>
#include "controllers/controllers.h"
#include "platforms/platforms.h"

io::Device lightDevice("MyrtLight");

void setup() {
	// Enable MQTT logging
	Serial.begin(115200);
	auto mqttController = IO_INJECT_INSTANCE(MQTTController);
	io::Logger::setOutput(&Serial);

	lightDevice.setup()
		// Add platforms
		.registerPlatform(IO_INJECT_UNIT(IWiFiPlatform))
		.registerPlatform(IO_INJECT_UNIT(ILightPlatform))
		// Add controllers
		.registerController(IO_INJECT_UNIT(OTAController))
		.registerController(IO_INJECT_UNIT(BootController))
		.registerController(IO_INJECT_UNIT(MQTTController));

	lightDevice.log().print("initialized");
}

void loop() {
	lightDevice.loop();
}
