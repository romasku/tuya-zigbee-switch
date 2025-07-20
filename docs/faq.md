# Frequently Asked Questions

<details>
<summary>
How do the Action modes work?
</summary>
  
Detailed answer and discussion here: [Switch mode #54](https://github.com/romasku/tuya-zigbee-switch/issues/54#issuecomment-3006002960)
</details>

<details>
<summary>
What is FORCE OTA index/image?
</summary>

**Normal** images **do not** allow flashing the same version (for example v1.0.17).  
So, using the **normal** index, Z2M will update your device to the latest version and say that you have the latest version already installed.  
You will only receive updates if the version number is incremented (in the Makefile).

**Force** images **do** allow flashing the same version.  
So, using the force index, Z2M will update your device to the latest version and still say there is an update available.  
Z2M will **always** say there is an update available, even if there is no difference between your version and the remote version.  

Use the **force** image when:
- switching between Router/End-device firmware
- testing your changes in the source code

After flashing a **force** image, you can simply change the index back to normal and restart Z2M. No downgrade or other action necessary. You will receive updates correctly.
</details>

<details>
<summary>
Will I still receive updates for my other devices?
</summary>

Yes, the custom index you provide to Z2M does not replace the original index. It 'appends', so you receive custom firmware for the supported devices and stock firmware for the other devices.
</details>

<details>
<summary>
Is it possible to revert to stock firmware?
</summary>

Partially. Reverting to the stock firmware is only possible via wired flashing using UART.  
Additionally, you will need a dump of the original stock firmware. Dumps are available for **some** devices in the `bin/` directory.

To flash the stock firmware, follow the same steps outlined in the [flashing via wire guide](./docs/flashing_via_wire.md), just as you would for custom firmware.

</details>