void i2cTest() {
	I2C i2c(0, 100000);

	while(1) {
		uint8_t pressureData[3];
		uint8_t readPressureCmd = 0xF1;
		int16_t pressure = 0;


		if (i2c.transaction(0x40, &readPressureCmd, 1, pressureData, 3)) {
			pressure = (pressureData[0] << 8) | pressureData[1];
			itm.print("Pressure data: ");
			itm.print(std::to_string(pressure));
		}
		else {
			itm.print("Error reading pressure.\r\n");
		}
		Sleep(1000);
	}
}

