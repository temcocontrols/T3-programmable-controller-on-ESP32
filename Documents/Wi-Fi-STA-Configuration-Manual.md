# Wi-Fi STA Configuration Manual

This manual explains how to configure the controller to connect in STA mode (Station mode) to your existing Wi-Fi network.

## 1. What is STA mode?

STA mode means the device acts as a client and connects to an existing Wi-Fi router or access point.

- The controller does not create its own network for normal operation in STA mode.
- It joins your home or office Wi-Fi network.
- After successful connection, you can access the device over the local network.

## 2. What you need before starting

Make sure you have:

- The controller powered on.
- A phone, laptop, or tablet with Wi-Fi.
- The Wi-Fi network name (SSID).
- The Wi-Fi password.
- A web browser such as Chrome, Edge, or Safari.

Important notes:

- Use a 2.4 GHz Wi-Fi network if possible.
- The password should be entered exactly as it is on the router.
- If your network uses WPA2/WPA3 security, the password is usually required.

## 3. How the device enters Wi-Fi setup mode

If the controller does not have valid Wi-Fi credentials saved, or if it cannot connect to the saved network, it automatically starts a temporary setup access point.

Default setup hotspot details:

- SSID: T3_Admin
- Password: T3_Admin

This hotspot is used only for configuration.

## 4. How to connect and configure Wi-Fi

### Step 1: Connect your phone or laptop to the controller setup hotspot

1. Open the Wi-Fi settings on your phone or laptop.
2. Search for the network named T3_Admin.
3. Connect to it using the password T3_Admin.

You should now be connected to the controller's temporary setup network.

### Step 2: Open the configuration page

Open a browser and visit one of the following:

- http://192.168.4.1
- http://tstat.local

If the page does not open immediately, wait a few seconds and try again.

### Step 3: Select your home Wi-Fi network

On the configuration page:

1. Click the Scan Wi-Fi button.
2. Wait for the list of nearby networks to appear.
3. Select your Wi-Fi network from the list.
4. Enter the Wi-Fi password in the password field.
5. Click Save & Connect.

### Step 4: Wait for the device to connect

After saving:

- The controller will reboot.
- It will try to connect to the Wi-Fi network you selected.
- Wait about 30 to 60 seconds.

### Step 5: Disconnect from the setup hotspot

Once the device connects successfully:

1. Disconnect your phone or laptop from T3_Admin.
2. Reconnect to your normal home or office Wi-Fi network.
3. Open the controller using its IP address or hostname if available.

## 5. Where to connect from

Use the following connection points depending on the situation:

- During initial setup: connect to the temporary hotspot T3_Admin.
- After successful STA connection: connect to your normal Wi-Fi network and access the controller from its local IP address.
- If your network supports mDNS, you may also try the hostname tstat.local.

## 6. Recommended Wi-Fi setup conditions

For best results:

- Place the controller near the router during initial setup.
- Make sure the Wi-Fi signal is strong.
- Avoid using a hidden SSID unless you know the exact name.
- Use a simple password that does not contain unsupported characters.

## 7. Troubleshooting

### Problem: I cannot see the T3_Admin network

Try the following:

- Make sure the controller is powered on.
- Wait a few seconds after startup.
- Restart the controller.
- Check whether the setup hotspot is active.

### Problem: The configuration page does not open

Try the following:

- Connect again to T3_Admin.
- Use http://192.168.4.1 directly.
- Ensure your device is connected to the controller hotspot and not your home Wi-Fi.

### Problem: The device cannot connect to the router

Try the following:

- Confirm the SSID name is correct.
- Confirm the password is correct.
- Use a 2.4 GHz Wi-Fi network.
- Move the controller closer to the router.
- Restart the controller and try again.

### Problem: The device connects but I cannot reach it later

Try the following:

- Check the router's DHCP client list.
- Confirm the device received an IP address.
- Try using the assigned IP address in the browser.

## 8. Summary

To configure Wi-Fi in STA mode:

1. Connect to the temporary hotspot T3_Admin.
2. Open http://192.168.4.1 or http://tstat.local.
3. Scan for your Wi-Fi network.
4. Enter your SSID and password.
5. Save and wait for the device to connect.

Once connected, the controller will operate as a client on your Wi-Fi network.
