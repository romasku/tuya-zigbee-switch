# Frequently Asked Questions

<details open>
<summary>
<b> Troubleshooting </b>
</summary>
<p></p>

Before reporting any issues:
1. Stop Z2M   
2. Update coordinator  
  Just restart it if there's no update
3. Re-download the custom [converters/](../zigbee2mqtt/converters/)
4. Restart Z2M 
5. **Interview device `i`** 
6. **Reconfigure device `🗘`**
7. Boost signal if possible

Resetting the device may also help  
⤷ Spam any switch / Hold reset button / Remove from Z2M

</details>

<details>
<summary>
How do the Action modes work?
</summary>
<p></p>

Detailed answer and discussion here: [Switch mode #238](https://github.com/romasku/tuya-zigbee-switch/discussions/238)
</details>

<details>
<summary>
What is force OTA index?
</summary>
<p></p>

**It shows *Update available* constantly**  
- Allows updating without incrementing the version number
- Allows reinstalling the same version  

Use it when:
- switching between Router/End-device firmware
- switching branches
- testing changes in the source code

The firmware image is the same in normal and force index.
</details>

<details>
<summary>
Will my other devices still receive updates?
</summary>
<p></p>

**Yes**. The custom index is appended to the official index.  
- Supported devices receive custom updates
- Not-supported devices receive official updates
</details>

<details>
<summary>
Is it possible to revert to stock firmware?
</summary>
<p></p>

Reverting to stock firmware is only possible via **wired flashing.**  
It additionally requires a **memory dump of the original device.**  
*Some backups are available in [`bin/_factory/`](../bin/_factory/)*

Follow the same steps outlined in [flashing/](./flashing/), as you would for custom firmware.

</details>

<details>
<summary>
How to recover an unresponsive device?
</summary>
<p></p>

If your device does not join Z2M, and does not respond to commands or button-presses, the only way to bring it back is [flashing/](./flashing/)

</details>
